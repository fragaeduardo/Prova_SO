#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#define POOL_SIZE 4
#define MAX_REQUESTS 10000

typedef struct {
    int id;
} Request;

// Várias requisições disputam as mesmas duas contas; o pool tem quatro workers.
Request queue[MAX_REQUESTS];
int worker_ids[POOL_SIZE];
int request_count;
int queue_head;
int account_a = 1000;
int account_b = 0;
int completed;
int safe_mode;
pthread_mutex_t queue_mutex;
pthread_mutex_t account_mutex;

// O handler só muda esta flag; o programa imprime a fila fora do handler.
volatile sig_atomic_t debug_requested;

void handle_debug(int) {
    debug_requested = 1;
}

void show_queue(void) {
    int i;

    // A fila também é usada pelos workers, então lemos enqt ela tá protegida
    pthread_mutex_lock(&queue_mutex);
    printf("Fila FCFS aguardando (%d):", request_count - queue_head);
    for (i = queue_head; i < request_count; ++i) printf(" R%d", queue[i].id);
    puts("");
    pthread_mutex_unlock(&queue_mutex);
}

void *worker(void *arg) {
    // Pthreads entrega o argumento como void*: aqui voltamos para int*.
    int worker_id = *((int *)arg);

    for (;;) {
        Request request;
        int old_a;
        int old_b;

        // Retirar sempre o primeiro pedido mantém a ordem FCFS.
        pthread_mutex_lock(&queue_mutex);
        if (queue_head == request_count) {
            pthread_mutex_unlock(&queue_mutex);
            return NULL;
        }
        request = queue[queue_head++];
        pthread_mutex_unlock(&queue_mutex);

        if (safe_mode) {
            // Modo protegido: a transação inteira fica dentro da seção crítica.
            pthread_mutex_lock(&account_mutex);
            old_a = account_a;
            old_b = account_b;
            // Pausa simulada para representar o tempo de atendimento.
            sleep(1);
            account_a = old_a - 1;
            account_b = old_b + 1;
            pthread_mutex_unlock(&account_mutex);
        } else {
            // Modo sem proteção completa: outra thread pode ler os mesmos saldos aqui.
            pthread_mutex_lock(&account_mutex);
            old_a = account_a;
            old_b = account_b;
            pthread_mutex_unlock(&account_mutex);

            // Essa pausa deixa outras requisições lerem o mesmo saldo antes da gravação.
            sleep(1);

            pthread_mutex_lock(&account_mutex);
            account_a = old_a - 1;
            account_b = old_b + 1;
            pthread_mutex_unlock(&account_mutex);
        }

        // Vários workers podem terminar juntos, então protegemos também o contador.
        pthread_mutex_lock(&queue_mutex);
        ++completed;
        pthread_mutex_unlock(&queue_mutex);
        printf("Worker %d processou R%d\n", worker_id, request.id);
    }
}

int main(int argc, char **argv) {
    pthread_t workers[POOL_SIZE];
    int i;
    int input;

    if (argc != 3 || (strcmp(argv[1], "race") != 0 && strcmp(argv[1], "safe") != 0)) {
        fprintf(stderr, "Uso: %s race|safe N\n", argv[0]);
        return 1;
    }

    // atoi aceita "4abc" como 4; use strtol para validar rigorosamente.
    request_count = atoi(argv[2]);
    if (request_count < 1 || request_count > MAX_REQUESTS) {
        fprintf(stderr, "N deve estar entre 1 e %d.\n", MAX_REQUESTS);
        return 1;
    }
    safe_mode = strcmp(argv[1], "safe") == 0;

    // Inicializa os mutexes como no exemplo de Pthreads visto em aula.
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_mutex_init(&account_mutex, NULL);

    // R1, R2, ... entram na fila nesta ordem, antes do início do atendimento.
    for (i = 0; i < request_count; ++i) queue[i].id = i + 1;

    // A tecla d envia este sinal; o handler apenas marca que a fila deve ser exibida.
    if (signal(SIGUSR1, handle_debug) == SIG_ERR) {
        perror("signal");
        return 1;
    }

    printf("PIX: %d requisicoes para a mesma conta; pool=%d; modo=%s\n", request_count, POOL_SIZE, safe_mode ? "protegido" : "sem secao critica completa");
    printf("Fila inicial FCFS: R1 ... R%d. Digite d + Enter para depurar; q + Enter para encerrar entrada.\n", request_count);

    for (i = 0; i < POOL_SIZE; ++i) {
        worker_ids[i] = i + 1;
        if (pthread_create(&workers[i], NULL, worker, &worker_ids[i]) != 0) {
            fprintf(stderr, "Erro ao criar uma thread.\n");
            return 1;
        }
    }

    while ((input = getchar()) != EOF) {
        if (input == 'd' || input == 'D') {
            // O sinal é tratado agora; a impressão acontece neste fluxo normal.
            if (raise(SIGUSR1) != 0) {
                perror("raise");
                break;
            }
            if (debug_requested) {
                debug_requested = 0;
                show_queue();
            }

        } else if (input == 'q' || input == 'Q') break;
    }

    for (i = 0; i < POOL_SIZE; ++i) pthread_join(workers[i], NULL);

    // Esperar por todas as threads garante que os saldos finais já estão prontos.
    printf("Resultado: conta A=%d, conta B=%d, transferencias atendidas=%d, esperadas=%d\n", account_a, account_b, completed, request_count);
    if (account_b != request_count) printf("Divergencia: saldo recebido mostra %d transferencia(s) perdida(s).\n", request_count - account_b);

    return 0;
}
