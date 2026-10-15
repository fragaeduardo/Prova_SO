# Roteiro de apresentação — A01

## 1. Contextualização e parâmetros

- “Esta é uma simulação de PIX entre duas contas compartilhadas. Temos um único pool com quatro threads trabalhadoras, chamadas workers. N é a quantidade de solicitações, não a quantidade de workers. Neste programa, N solicitações disputam uma única base de contas; isso não simula o mapeamento de threads de usuário para threads de kernel. Todos os pedidos entram juntos em um lote inicial, na ordem R1, R2 e assim por diante.”<br>
  Mostrar: [POOL_SIZE, linha 10](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:10>); [Request, linha 13](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:13>); [queue, linha 18](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:18>); [preenchimento da fila, linha 144](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:144>).

- “Cada pedido transfere uma unidade de A para B. As contas começam com A igual a 1000 e B igual a zero. Escolhi quatro workers para mostrar facilmente pedidos acima da capacidade do pool. O limite de entrada é dez mil solicitações, e cada atendimento tem uma pausa simulada de um segundo para conseguirmos observar a execução. Essa pausa não é o tempo real de um PIX. Se todas as transferências forem preservadas, B deve terminar igual a N.”<br>
  Mostrar: [MAX_REQUESTS, linha 11](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:11>); [saldos iniciais, linha 25](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:25>); [tempo simulado, linha 90](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:90>).

## 2. Preparação do programa

- “O programa é escrito em C. As bibliotecas fornecem entrada e saída, conversão dos argumentos, threads, sinais, pausas e semáforos. Na execução escolhemos race ou safe e informamos N. A conversão com strtol verifica se N é um inteiro válido. Depois inicializamos os mutexes, os pedidos e seus semáforos. Só o semáforo de R1 começa liberado. Se a inicialização de um semáforo falhar, o programa encerra com erro.”<br>
  Mostrar: [bibliotecas, linha 1](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:1>); [main, linha 119](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:119>); [validação de N, linha 132](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:132>); [inicialização dos mutexes, linha 140](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:140>); [inicialização dos semáforos, linha 147](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:147>).

- “Criamos quatro workers e passamos o identificador de cada um como argumento. Como Pthreads recebe um ponteiro genérico, o worker converte esse argumento de volta para acessar seu número. Se não conseguir criar um worker, o programa encerra com erro. Cada worker retira o primeiro pedido da fila, marca que está responsável por ele e repete o processo até não haver mais pedidos. O mutex da fila protege essa retirada e também o estado usado pela depuração.”<br>
  Mostrar: [criação dos workers, linha 163](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:163>); [argumento do worker, linha 68](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:68>); [retirada FIFO, linha 81](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:81>); [pedido atribuído ao worker, linha 82](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:82>).

## 3. Questão 1 — Condição de corrida

- “O recurso compartilhado são os saldos de A e B. A seção crítica é a transferência inteira: ler os saldos, calcular e gravar os novos valores. No modo race, os acessos isolados têm mutex, mas ele é solto entre leitura e gravação. Durante a pausa outra thread pode ler os mesmos valores antigos. As duas calculam a partir do mesmo saldo e uma atualização sobrescreve a outra, causando uma transferência perdida.”<br>
  Mostrar: [saldos compartilhados, linha 25](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:25>); [ramo race, linha 94](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:94>); [proteção isolada da leitura, linha 95](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:95>).

- “Primeiro executo um pedido. B termina em um, como esperado. Depois executo dois: nos testes, B terminou em um, embora os dois pedidos tenham sido processados. Assim, a perda já ficou evidente em N igual a dois. Isso acontece com o tempo simulado usado aqui; o escalonamento pode variar entre execuções, então não afirmo uma fórmula fixa para todos os resultados.”<br>
  Mostrar: no terminal, `gcc -std=c99 -Wall -Wextra -Wpedantic -pthread pix.c -o pix`; depois `./pix race 1` e `./pix race 2`. Digite `q` e Enter em cada execução para encerrar a entrada e aguardar o resultado. Compare o saldo B com o total esperado; no modo race, o código de saída 2 indica a divergência demonstrada.

## 4. Questão 2 — Proteção e FCFS

- “No modo safe, cada pedido espera no seu próprio semáforo. R1 começa liberado; os demais ficam bloqueados. Depois de transferir, atualizar o contador e imprimir sua conclusão, R1 libera R2; R2 libera R3, e assim por diante. Isso protege a transferência inteira e garante FCFS tanto no atendimento quanto na saída. Se a espera pelo semáforo for interrompida por um sinal, ela é repetida; qualquer outro erro encerra o programa sem fazer a transferência. Não dependemos da ordem em que um mutex acordaria as threads.”<br>
  Mostrar: [semáforos turn, linha 32](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:32>); [espera e repetição se interrompida por sinal, linha 56](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:56>); [transferência protegida, linha 85](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:85>); [contador e registro de conclusão, linha 109](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:109>); [liberação do próximo pedido, linha 115](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:115>).

- “O pool continua tendo quatro workers, mas as transferências na mesma base são serializadas. Até quatro pedidos ficam atribuídos aos workers, incluindo os que esperam sua vez; os restantes aguardam na fila. Com seis pedidos, maior que o pool, conseguimos observar R5 e R6 aguardando. A conclusão deve aparecer de R1 até R6, com A igual a 994 e B igual a seis. Como cada transferência protegida demora um segundo, essa execução leva aproximadamente seis segundos.”<br>
  Mostrar: execute `./pix safe 6`; logo após iniciar, digite `d` e Enter para observar os pedidos nos workers e os que ainda aguardam. Depois digite `q` e Enter e confira a sequência R1 a R6 e os saldos finais. Os números dos workers podem variar.

## 5. Questão 3 — Depuração por sinal

- “Quando digito d e Enter, a thread principal envia SIGUSR1 para si mesma com pthread_kill. O handler registra o pedido de depuração numa flag e reinstala o tratamento para permitir novos sinais. A impressão acontece depois, no fluxo normal do programa, evitando operações de impressão ou mutex dentro do handler.”<br>
  Mostrar: [flag de depuração, linha 35](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:35>); [handler, linha 37](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:37>); [registro do handler, linha 153](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:153>); [envio do sinal, linha 175](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:175>); [consulta da flag e exibição, linha 179](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:179>).

- “A depuração mostra os pedidos atribuídos aos workers, a fila FCFS aguardando e o total concluído. Em safe, um pedido listado em atendimento pode estar bloqueado esperando seu semáforo. O mutex da fila mantém a leitura desses dados consistente enquanto os workers alteram o estado. Como o terminal lê uma linha, usamos d mais Enter.”<br>
  Mostrar: [show_queue, linha 43](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:43>); [pedidos nos workers, linha 47](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:47>); [pedidos aguardando, linha 50](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:50>); [total concluído, linha 52](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:52>).

- “Para demonstrar que o sinal funciona repetidamente, executo safe com seis pedidos e digito d e Enter duas vezes durante o atendimento. Entre as consultas a fila diminui e o total concluído aumenta. Por fim, q encerra somente a leitura do teclado; os workers continuam até terminar os pedidos.”<br>
  Mostrar: `./pix safe 6`; digite `d` e Enter, aguarde cerca de dois segundos e repita. Depois digite `q` e Enter.

## 6. Encerramento

- “pthread_join espera os workers terminarem antes de lermos o resultado final. O programa mostra os saldos e a quantidade de pedidos processados, compara B com N e libera os semáforos e mutexes. É importante comparar o saldo também: um pedido pode ter sido processado no modo race sem sua atualização ter sido preservada.”<br>
  Mostrar: [espera pelos workers, linha 188](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:188>); [resultado final, linha 190](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:190>); [liberação dos recursos, linha 195](</home/eduardo/Área de trabalho/Sistemas Operacionais/prova01/pix.c:195>).
