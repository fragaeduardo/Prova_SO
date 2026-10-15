# Roteiro de apresentação — A01

## 1. Abertura

### Mostrar

- [`pix.c`, linhas 8–25](pix.c): tamanho do pool, estrutura `Request`, fila, saldos compartilhados e mutexes.

### Falar

> “Esta é uma simulação simplificada de transferências entre duas contas. Os pedidos ficam numa fila e são atendidos por um pool com quatro threads trabalhadoras. Cada pedido transfere uma unidade da conta A para a conta B.”

## 2. Parâmetros da simulação

| Parâmetro | Valor no código | Como explicar |
|---|---:|---|
| Número de pools | 1 | Há um único pool de workers. |
| Workers do pool | 4 | `POOL_SIZE` define quatro threads que retiram e atendem os pedidos. |
| `N` | Informado ao executar | É o número de pedidos de transferência, não o número de threads criadas. |
| Pedidos máximos | 10.000 | Limite técnico definido por `MAX_REQUESTS`. |
| Saldo inicial | A = 1000; B = 0 | Valores simples para comparar o resultado esperado. |
| Valor transferido | 1 por pedido | Mantém a simulação fácil de acompanhar. |
| Tempo simulado | 1 segundo | A pausa ajuda a observar a intercalação das threads; não representa o tempo real de um PIX. |

### Falar

> “Escolhi quatro workers para que seja fácil ver a diferença entre a capacidade de atendimento e os pedidos que ficam esperando. O valor de N pode mudar em cada execução. Como cada pedido transfere uma unidade, depois de N transferências corretas, B deve terminar com N.”

## 3. Preparação e início do programa

### Mostrar

- [`pix.c`, linhas 97–113](pix.c): escolha do modo e leitura de N.
- [`pix.c`, linhas 115–120](pix.c): inicialização dos mutexes e preenchimento da fila.
- [`pix.c`, linhas 131–137](pix.c): criação dos quatro workers.

### Falar

> “Ao iniciar o programa, escolho o modo `race` ou `safe` e informo N. O programa inicializa os mutexes, coloca os pedidos na fila em ordem e cria os quatro workers. Cada worker atende pedidos até a fila acabar.”

## 4. Item 1 — Condição de corrida

### Mostrar

- [`pix.c`, linhas 20–21](pix.c): saldos compartilhados `account_a` e `account_b`.
- [`pix.c`, linhas 73–87](pix.c): modo `race`, com leitura e gravação separadas.
- [`pix.c`, linhas 156–158](pix.c): resultado final dos saldos.

### Falar

> “As contas são o recurso compartilhado. No modo `race`, cada thread lê os saldos, espera e depois grava os valores calculados. Como outra thread pode ler os mesmos saldos antes da gravação, uma atualização pode sobrescrever a outra. A seção crítica é a transferência inteira, da leitura até a gravação.”

### Demonstração

- Execute `./pix race 1` e observe um pedido.
- Execute `./pix race 2` e compare o saldo de B com os dois pedidos esperados.
- Explique que a pausa simulada aumenta a chance de os workers lerem os mesmos valores antigos.

## 5. Item 2 — Proteção com mutex e fila FCFS

### Mostrar

- [`pix.c`, linha 8](pix.c): `POOL_SIZE` igual a 4.
- [`pix.c`, linhas 54–61](pix.c): retirada do primeiro pedido da fila sob `queue_mutex`.
- [`pix.c`, linhas 63–72](pix.c): modo `safe`, com `account_mutex` protegendo a transferência inteira.
- [`pix.c`, linhas 89–93](pix.c): contador de transferências concluídas.

### Falar

> “A fila é atendida em ordem de chegada: cada worker retira o primeiro pedido disponível. Os quatro primeiros podem começar com os quatro workers; os seguintes esperam na fila. No modo `safe`, o mutex fica travado durante a leitura, o cálculo e a gravação dos saldos. Assim, outra thread espera a seção crítica ser liberada.”

### Demonstração

- Execute `./pix safe 6`.
- Digite `d` e Enter enquanto há pedidos aguardando para mostrar a fila.
- Compare o resultado final com seis transferências esperadas.

## 6. Item 3 — Depuração por sinal

### Mostrar

- [`pix.c`, linhas 27–32](pix.c): flag e handler do sinal.
- [`pix.c`, linhas 122–126](pix.c): associação de `SIGUSR1` ao handler.
- [`pix.c`, linhas 139–149](pix.c): tecla `d`, envio do sinal e chamada para exibir a fila.
- [`pix.c`, linhas 34–43](pix.c): função `show_queue`.

### Falar

> “Quando digito `d` e Enter, o programa envia `SIGUSR1`. O handler apenas marca que pedi a depuração. Em seguida, o fluxo normal do programa chama `show_queue`, que mostra os pedidos ainda aguardando.”

## 7. Encerramento

### Mostrar

- [`pix.c`, linhas 154–158](pix.c): espera pelos workers e impressão do resultado final.

### Falar

> “No encerramento, `pthread_join` espera os quatro workers terminarem. Depois, o programa mostra os saldos finais e compara quantas transferências foram concluídas com quantas eram esperadas.”

## 8. Limite importante da versão atual

O código atual coloca todos os pedidos na fila antes de iniciar os workers. Portanto, cada execução representa **um único lote inicial**. Ele ainda não representa 10 pedidos chegando juntos e, mais tarde, outro lote de 30. Também não cria N threads: cria quatro workers e os pedidos são itens da fila. Se o professor exigir dois lotes separados ou N threads reais, o programa precisa ser adaptado antes de apresentar essa parte como implementada.
