#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define POOL_SIZE 4
#define MAX_REQUESTS 10000

typedef struct {
    int id;
} Request;

/* Fila FCFS: requisições entram em ordem R1..RN e saem sempre pela cabeça. */
Request queue[MAX_REQUESTS];
int request_count;
int queue_head;
int in_service[POOL_SIZE]; /* id da requisição em atendimento por worker (0 = livre) */
int worker_ids[POOL_SIZE];

/* Base de dados compartilhada (recurso crítico). */
int account_a = 1000;
int account_b = 0;
int completed;
int safe_mode;

pthread_mutex_t queue_mutex;   /* protege fila, in_service e completed */
pthread_mutex_t account_mutex; /* usado no modo race (acessos isolados) */
sem_t turn[MAX_REQUESTS];      /* modo safe: turn[k] libera a k-ésima requisição */

/* O handler só altera uma flag (async-signal-safe); a impressão fica fora dele. */
volatile sig_atomic_t debug_requested;

void handle_debug(int sig) {
    /* Reinstala o handler: em algumas implementações de signal() (semântica
       System V, p.ex. gcc -std=c99) o tratamento volta ao padrão após o 1o sinal,
       e o 2o SIGUSR1 encerraria o programa. signal() é async-signal-safe. */
    signal(sig, handle_debug);
    debug_requested = 1;
}

void show_queue(void) {
    int i;

    pthread_mutex_lock(&queue_mutex);
    printf("[DEBUG] Em atendimento:");
    for (i = 0; i < POOL_SIZE; ++i)
        if (in_service[i]) printf(" R%d(w%d)", in_service[i], i + 1);
    printf("\n[DEBUG] Fila FCFS aguardando (%d):", request_count - queue_head);
    for (i = queue_head; i < request_count; ++i) printf(" R%d", queue[i].id);
    printf("\n[DEBUG] Concluidas: %d de %d\n", completed, request_count);
    pthread_mutex_unlock(&queue_mutex);
}

void wait_turn(sem_t *s) {
    /* sem_wait pode falhar com EINTR se um sinal interromper a thread. */
    while (sem_wait(s) == -1 && errno == EINTR) {
    }
}

void *worker(void *arg) {
    /* Pthreads entrega o argumento como void*: voltamos para int*. */
    int worker_id = *((int *)arg);

    for (;;) {
        Request request;
        int old_a;
        int old_b;

        /* Retirar sempre a cabeça da fila mantém a ordem FCFS. */
        pthread_mutex_lock(&queue_mutex);
        if (queue_head == request_count) {
            pthread_mutex_unlock(&queue_mutex);
            return NULL;
        }
        request = queue[queue_head++];
        in_service[worker_id - 1] = request.id;
        pthread_mutex_unlock(&queue_mutex);

        if (safe_mode) {
            /* Espera a vez desta requisição (FCFS); só uma por vez na seção crítica. */
            wait_turn(&turn[request.id - 1]);
            old_a = account_a;
            old_b = account_b;
            sleep(1); /* tempo de atendimento simulado, dentro da seção crítica */
            account_a = old_a - 1;
            account_b = old_b + 1;
            if (request.id < request_count) sem_post(&turn[request.id]); /* libera a próxima */
        } else {
            /* Sem seção crítica completa: outra thread lê os mesmos saldos durante o sleep. */
            pthread_mutex_lock(&account_mutex);
            old_a = account_a;
            old_b = account_b;
            pthread_mutex_unlock(&account_mutex);

            sleep(1);

            pthread_mutex_lock(&account_mutex);
            account_a = old_a - 1;
            account_b = old_b + 1;
            pthread_mutex_unlock(&account_mutex);
        }

        pthread_mutex_lock(&queue_mutex);
        ++completed;
        in_service[worker_id - 1] = 0;
        printf("Worker %d processou R%d\n", worker_id, request.id);
        pthread_mutex_unlock(&queue_mutex);
    }
}

int main(int argc, char **argv) {
    pthread_t workers[POOL_SIZE];
    int created = 0;
    int i;
    int input;
    char *end;
    long n;

    if (argc != 3 || (strcmp(argv[1], "race") != 0 && strcmp(argv[1], "safe") != 0)) {
        fprintf(stderr, "Uso: %s race|safe N\n", argv[0]);
        return 1;
    }

    n = strtol(argv[2], &end, 10);
    if (*argv[2] == '\0' || *end != '\0' || n < 1 || n > MAX_REQUESTS) {
        fprintf(stderr, "N deve ser um inteiro entre 1 e %d.\n", MAX_REQUESTS);
        return 1;
    }
    request_count = (int)n;
    safe_mode = strcmp(argv[1], "safe") == 0;

    pthread_mutex_init(&queue_mutex, NULL);
    pthread_mutex_init(&account_mutex, NULL);

    /* R1, R2, ... entram na fila nesta ordem, antes do início do atendimento. */
    for (i = 0; i < request_count; ++i) {
        queue[i].id = i + 1;
        sem_init(&turn[i], 0, i == 0 ? 1 : 0); /* só R1 começa liberada */
    }

    if (signal(SIGUSR1, handle_debug) == SIG_ERR) {
        perror("signal");
        return 1;
    }

    printf("PIX: %d requisicoes para a mesma base de contas; pool=%d; modo=%s\n", request_count, POOL_SIZE,
           safe_mode ? "protegido (FCFS)" : "sem secao critica completa");
    printf("Fila inicial FCFS: R1 ... R%d. Digite d + Enter para depurar; q + Enter para encerrar a entrada.\n",
           request_count);

    for (i = 0; i < POOL_SIZE; ++i) {
        worker_ids[i] = i + 1;
        if (pthread_create(&workers[i], NULL, worker, &worker_ids[i]) != 0) {
            fprintf(stderr, "Erro ao criar a thread %d.\n", i + 1);
            break;
        }
        ++created;
    }

    while (created > 0 && (input = getchar()) != EOF) {
        if (input == 'd' || input == 'D') {
            /* Envia SIGUSR1 a esta thread (main); o handler apenas marca a flag. */
            if (pthread_kill(pthread_self(), SIGUSR1) != 0) {
                perror("pthread_kill");
                break;
            }
            if (debug_requested) {
                debug_requested = 0;
                show_queue();
            }
        } else if (input == 'q' || input == 'Q') {
            break;
        }
    }

    for (i = 0; i < created; ++i) pthread_join(workers[i], NULL);

    printf("Resultado: conta A=%d, conta B=%d, transferencias atendidas=%d, esperadas=%d\n", account_a, account_b,
           completed, request_count);
    if (account_b != request_count)
        printf("Divergencia: saldo recebido mostra %d transferencia(s) perdida(s).\n", request_count - account_b);

    for (i = 0; i < request_count; ++i) sem_destroy(&turn[i]);
    pthread_mutex_destroy(&queue_mutex);
    pthread_mutex_destroy(&account_mutex);
    return account_b == request_count ? 0 : 2;
}
