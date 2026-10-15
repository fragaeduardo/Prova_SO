# PIX — Simulação de transferências concorrentes (A01)

Prova de Sistemas Operacionais que simula transferências do tipo PIX entre duas contas, atendidas por um **pool de 4 threads (workers)**. O programa demonstra:

1. **Condição de corrida** sobre um recurso compartilhado (modo `race`);
2. **Proteção da seção crítica** com atendimento em ordem de chegada, **FCFS** (modo `safe`);
3. **Depuração por sinal** (`SIGUSR1`) para inspecionar a fila durante a execução.

> O enunciado original está em [`A01.PDF`](A01.PDF) e o roteiro de apresentação em [`roteiro.md`](roteiro.md).

## Como funciona

- Existe um único pool com `POOL_SIZE = 4` workers (`pthread`).
- Cada execução recebe `N` requisições (`R1 … RN`), colocadas na fila **antes** de os workers começarem. `N` é o número de pedidos, não de threads.
- Cada requisição transfere **1 unidade** da conta A para a conta B (saldos iniciais: A = 1000, B = 0) e simula 1 segundo de atendimento com `sleep(1)`.
- Ao final, B deve valer `N`. Qualquer valor diferente indica transferências perdidas.

### Modo `race` (sem seção crítica completa)

O worker lê os saldos, espera 1 s e só então grava `saldo - 1` / `saldo + 1`. Como outro worker pode ter lido os mesmos saldos antigos durante a espera, as atualizações se sobrescrevem (*lost update*). Resultado típico com `N = 4`: `B = 1` em vez de `4`.

**A partir de que N o problema aparece?** Medido com `./pix race N` (o valor exato pode variar entre execuções):

| N | 1 | 2 | 3 | 4 | 5 | 8 | 20 |
|---|---:|---:|---:|---:|---:|---:|---:|
| B obtido (esperado = N) | 1 | 1 | 1 | 1 | 2 | 2 | 5 |

Com `N = 1` não há concorrência; já com `N = 2` há perda de transferência.

### Modo `safe` (protegido, FCFS)

Leitura, atraso e gravação ficam dentro de uma única seção crítica. A ordem é garantida por um semáforo por requisição (`turn[k]`): a requisição `Rk` só entra na seção crítica depois que `R(k-1)` terminou, então as transferências ocorrem estritamente em ordem de chegada. Resultado esperado: `B = N` e saída com código `0`.

### Depuração por sinal

Durante a execução, digite `d` + Enter. A thread principal envia `SIGUSR1` a si mesma; o handler só levanta uma flag (`sig_atomic_t`, *async-signal-safe*) e a impressão é feita fora do handler, em `show_queue()`, que mostra:

- requisições em atendimento (e qual worker as atende);
- requisições aguardando na fila FCFS;
- quantas já foram concluídas.

Digite `q` + Enter para parar de ler a entrada (o programa ainda espera os workers terminarem e imprime o resultado).

## Requisitos

- Linux (ou WSL) com `gcc` e suporte a POSIX threads/semáforos.
- `make` (opcional).

## Compilação

```bash
make
# ou, sem make:
gcc -Wall -Wextra -pthread -o pix pix.c
```

## Execução

```bash
./pix race|safe N     # N entre 1 e 10000
```

| Exemplo | O que mostra |
|---|---|
| `./pix race 1` | Um pedido: resultado correto, sem concorrência. |
| `./pix race 4` | Condição de corrida: `B` termina menor que `4`. |
| `./pix safe 6` | Seis pedidos em ordem FCFS; `B = 6`. Digite `d` + Enter durante a execução para ver a fila. |

Exemplo de saída (`./pix safe 4`):

```
PIX: 4 requisicoes para a mesma base de contas; pool=4; modo=protegido (FCFS)
Fila inicial FCFS: R1 ... R4. Digite d + Enter para depurar; q + Enter para encerrar a entrada.
Worker 1 processou R1
Worker 2 processou R2
Worker 3 processou R3
Worker 4 processou R4
Resultado: conta A=996, conta B=4, transferencias atendidas=4, esperadas=4
```

> **Nota:** no modo `safe` o tempo total é de aproximadamente `N` segundos, pois as transferências são serializadas (1 s cada). No modo `race`, cerca de `⌈N/4⌉` segundos.

### Códigos de saída

| Código | Significado |
|---:|---|
| `0` | Execução correta (`B == N`). |
| `1` | Argumentos inválidos ou erro de inicialização. |
| `2` | Divergência de saldo (transferências perdidas — esperado no modo `race`). |

## Estrutura do repositório

| Arquivo | Descrição |
|---|---|
| `pix.c` | Código-fonte da simulação. |
| `Makefile` | Compilação (`make`) e limpeza (`make clean`). |
| `A01.PDF` | Enunciado do trabalho. |
| `roteiro.md` | Roteiro de apresentação. |
