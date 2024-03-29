# Manual de Referência do Sistema de Controle Do Forno LITel

O sistema de controle foi desenvolvido com um microcontrolador, um esp8266,
programado em c com uso do sdk da fabricante com sistema operacional [ESP8266
RTOS SDK](https://github.com/espressif/ESP8266_RTOS_SDK). Para o desenvolvimento
da placa de circuito impresso (PCB), foi utilizado o
[KiCad](https://www.kicad.org), um programa de Automação de Design Eletrônico
(EDA) open source.

A parte física pode ser dividida em algumas seções, que foram também separadas
para maior clareza no esquemático do circuito (kicad/projeto/projeto.kicad_sch):

# Divisão da parte física

## Sensor

O esp8266 possui apenas um conversor analógico - digital (adc), portanto foi
necessário o uso de um multiplexador analógico (CD4051) para permitir a leitura
de todos os sensores. Também, o adc suporta até 1V, enquanto o sensor pode
enviar até 1.5V. Um divisor de tensão foi utilizado para reduzir essa pela
metade, se encaixando com folga nos limites do microcontrolador.

O adc possui uma precisão de 10 bits, ou 1024 valores. Isso significa uma
precisão de 1/1024 = 0.977mV. O sensor utilizado (LM35) entrega 10mV/ºC, com
precisão típica entre 0.2ºC -> 2mV e 0.4ºC -> 4mV.

Os sensores são:
- S1: Central do fundo
- S2: Lateral direito
- S3: Lateral esquerdo
- S4: Porta
- S5: Principal (centro)
- S6: Ambiente

> No software, os sensores são indexados a partir de 0, não 1, logo o índice
será o descrito aqui menos 1.

## Status

Para visualização de alguns parâmetros diretamente na placa, três leds são
utilizados:

- Steady State: Liga quando a temperatura alcança um regime permanente;

- Wifi Status: Indica o status do modo estação do esp. Enquanto está tentando se
conectar, o led pisca. Se falhar / enquanto não estiver conectado devidamente,
fica desligado. Quando ligado e funcionando, fica ligado;

- Ligado: Liga diretamente na linha de 3.3V da placa, indicando quando o sistema
está energizado.

## Botões (Buttons)

Alguns botões são necessários para o adequado funcionamento, principalmente
para desenvolvimento.

- Reset: O botão para resetar o esp;
- Exit: Esse botão é ligado no pino de flash do esp, que deve ser mantido
em nível lógico baixo (pressionado) ao ligar para permitir o upload de novo
firmware.

## Detecção de Zero (Zero Detection)

Para a estratégia de modulação implementada, é necessário que o microcontrolador
saiba exatamente quando a tensão CA cruza 0V. Essa seção do circuito utiliza
um diodo zener e um optoacoplador (4N25) para gerar um pulso exatamente nesse
momento.

> O diodo zener foi utilizado para aumentar a largura do pulso, que não estava
sendo detectado de forma consistente em testes com apenas o optoacoplador.

## Driver da Carga (Load Driver)

Essa parte é a responsável por ativar a resistência da saída, através do uso
de um optoacoplador, que permite a condução entre o Gate e o terminal A2,
logo ativando a condução do triac, quando recebe um sinal de nível alto do
microcontrolador.

## Alimentação (Power)

O circuito é alimentado com 5V, que pode vir tanto do conversor USB-serial (se
estiver sendo utilizado) quanto de uma alimentação externa com um conector CC de
2.5mm. Os capacitores são utilizados para reduzir ruído da fonte e para garantir
que, caso o microcontrolador consuma uma corrente mais alta do que a fonte /
conversor são capazes de suprir por um curto período de tempo, o circuito ainda
funcione.

> Embora o esperado seja 5V, a fonte pode ser de até 10V, e os circuito ainda
funcionará adequadamente. Porém, vale notar que, se o circuito for alimentado
por uma fonte externa, ele **não deve ser alimentado pela tensão do conversor
USB-serial**. Nesse caso ainda deve se ligar o gnd do serial.

A alimentação CA é para a carga, e é protegida por um fusível (o instalado na
montagem foi de 12A).

## Serial

Aqui há apenas a conexão direta entre os pinos de comunicação serial do
microcontrolador com dois pinos (TX, RX), e a conexão da alimentação também em
dois pinos (5V, GND)

> Ao longo de todo o circuito, estão presentes diversos jumpers. Esses foram
usados apenas para facilitar a construção da PCB

# O Software

Para o projeto, foram desenvolvidos códigos para o microcontrolador (software
embarcado), para automatizar tarefas, e para interface com o usuário com uma
página web.

O código da página web foi desenvolvido em html, css e js, apenas como um
supervisório, e os detalhes de seu conteúdo podem ser vistos nos README.md e
também nas documentações e comentários dentro dos códigos.

O código para automatizar o envio dos arquivos web para o esp está em codigo/
helpers.

O software embarcado foi todo desenvolvido para ser concorrente, por meio do
sistema operacional FreeRTOS, embutido no
[SDK utilizado](https://github.com/espressif/ESP8266_RTOS_SDK), a esp-idf
(SDK da fabricante). Cada parte do sistema foi implementada em um arquivo .c
separado, e todas são inicializadas no main.c. Para os sistemas que dependem de
uma task, essas também são criadas no main.

> Para mais detalhes em qualquer código, basta abrir e ler os comentários e
documentações das funções. As documentações se encontram nos headers (.h) para
funções públicas, e nos source (.c) para as estáticas.

- main: O ponto de entrada do programa do microprocessador. Inicializa cada
parte individual do sistema, e cria as tasks para os que precisam de uma criada;

- wifi: Cuida da conexão WiFi do esp8266, tanto a inicialização do modo *Access
Point* quanto de tentar conectar a uma rede externa no modo *Station*;

- sensor_reader: Responsável por fazer a leitura periódica dos sensores e manter
seus valores acessíveis a outras partes do programa;

- serial_comm: Trata as entradas recebidas na porta serial, e ativa os devidos
comandos quando os seus padrões forem detectados;

- server: Implementa o servidor HTTP que serve como interface principal do
sistema;

- storage_spiffs: Faz a inicialização (mount) do sistema de arquivos usado
(SPIFFS). O sistema de arquivos é utilizado apenas para guardar os arquivos
web;

- helpers: Possui funções independentes que podem ser úteis em diversas partes
do programa, apenas para evitar reescrevê-las;

## controller

Essa seção é responsável por enviar à carga a potência desejada, e também por
periodicamente invocar controlador propriamente dito (que toma a decisão). O
algoritmo de modulação de potência utilizado foi um baseado no por detecção
de zero (*Zero Detection*). O mais comumente visto dessa técnica é permitir
a condução em X semiciclos, enquanto bloqueia a condução em Y. A condução do
dispositivo é sempre acionada quando  tensão cruza 0V, garantindo o mínimo
possível de perdas por chaveamento e também permitindo mais precisão na potência
controlada, se comparado ao método por PWM, por exemplo (considerando carga
atendida em CA acionada por um TRIAC). O que difere no método utilizado é a
distribuição dos semiciclos de condução e corte, que não mais são consecutivos
no período de funcionamento.

No método utilizado, a potência é representada por dois valores (X, Y), onde
X é a quantidade de semiciclos de condução, Y a quantidade de semiciclos em
corte, e X+Y o período total. Para determinar o que deve ser o semiciclo atual,
uma variável é inicializada em 0 e armazenada, chamada de contador de potência
(*power_counter*). Sempre que a borda entre dois semiciclos é percebida (V cruza
0V), o algoritmo define o tipo de semiciclo seguindo o seguinte pseudocódigo:

- Se **contador** > 0:  
    **contador** -= Y  
    **saída** = condução  
- Se **contador** \<= 0:  
    **contador** += x  
    **saída** = corte

Esse código mantém a relação de potência, mas torna a distribuição mais
homogênea. Vale notar que ao final do período (X+Y semiciclos), o contador de
potência é sempre 0. Isso permite uma troca de potência sem efeitos colaterais
de causados por um valor inicial desse contador, caso a mudança seja feita nesse
momento.

### Um pequeno exemplo parar ilustrar a modulação

Potência modulada para 60%, com período de 5 semiciclos:

X = 3, Y = 2

| semiciclo | contador | contador' | saída    |  
| --------- | -------- | --------- | -------- |
| 0         | 0        | 3         | corte    |
| 1         | 3        | 1         | condução |
| 2         | 1        | -1        | condução |
| 3         | -1       | 2         | corte    |
| 4         | 2        | 0         | condução |


## fuzzy_controller

Essa parte é a que de fato implementa o sistema de controle. Ela implementa,
como o nome sugere, um controlador de lógica difusa (Fuzzy Logic
Controller - FLC). O controlador foi baseado no de primeira ordem utilizado no
trabalho original de controle do forno
([TCC de Leonardo Serapião](https://www.ufjf.br/mecanica/files/2016/07/Trabalho-de-Conclus%c3%a3o-de-Curso-Leonardo-Ara%c3%bajo-Serapi%c3%a3o-Engenharia-Mec%c3%a2nica.pdf)).
Controladores difusos possuem bom desempenho para controle de sistemas não
lineares, e com possíveis perturbações externas, que é exatamente a situação do
controle de temperatura de um forno.

A lógica difusa funciona de forma parecida com uma lógica digital, porém com
saltos suaves entre variáveis. As variáveis difusa podem assumir valores entre
0 e 1, representando o nível de pertencimento do valor ao espaço da variável.
Esse valor será chamado de $\mu$. Em uma lógica binária, o valor seria 0 ou 1,
representando que pertence ao espaço ou não, não o quanto pertence.

### O controle

O controle se dá da seguinte maneira:

Primeiro, para cada entrada, o seu valor é codificado em algumas variáveis
fuzzy, através de um processo chamado de fuzzificação.

Essas variáveis difusas são então passadas por diversas regras, similares e
análogas a circuitos lógicos, que determinam novas variáveis difusas de saída em
função das de entrada. Cada regra mapeia um conjunto de variáveis de entrada a
uma única variável de saída, mas mais de uma regra pode usar a mesma variável de
saída, neste caso devem ser combinadas.

As variáveis de saída são transformadas em um único valor real (y), em um
processo chamado de defuzzificação.

### Entrada

Para a entrada, dois valores foram analisados: O erro e a variação da
temperatura. O erro é definido como $e[n] = T[n] - T_{set}$, onde $T_{set}$ é a
temperatura alvo. A variação foi pega diretamente da temperatura, e não variação
do erro, para evitar uma descontinuidade quando o set point é trocado. Em
qualquer momento diferente desse, as duas grandezas são equivalentes. Ademais, a
variação de temperatura é analisada ao longo do período de chamada do algoritmo
de controle (30 segundos).

> Na verdade, para facilitar os cálculos, todo o controlador foi implementado
com inteiros, logo a temperatura, variação e alvo são todos em termos da leitura
no ADC, e as variáveis difusas variam entre 0 e 256, em vez de de 0 a 1. A ideia
ainda é a mesma, o que muda é que essas questões devem ser levadas em conta em
algumas operações.

Para a variação da temperatura ($\Delta T$), 3 variáveis foram definidas: N
(negative), Z (zero), P (positive). Para o erro ($e_T$), foram definidas 5:
NL (negative large), NM (negative medium), NS (negative small), Z (zero) e P
(positive). Para ambas, a fuzzifição foi singleton com funções de participação
(*membership functions*) triangulares. Os limites podem vistos nos gráficos:

![Participação para variáveis de Delta T](/imgs/deltaT.svg "Participação para
variáveis de Delta T")

![Participação para variáveis de erro](/imgs/erro.svg "Participação para
variáveis de erro")

### Regras
A saída é composta de duas parcelas, uma que proporciona funções proporcional
e derivativa, a principal, e outra acumuladora de erro (integradora). Para esse
primeiro, as variáveis estabelecidas foram Z (zero), ST (estabilização), baixo
(L), médio (M) e alto (H).

- Se $e_T$ é P, Y é Z
- Se $\Delta_T$ é P, Y é Z

---

- Se $e_T$ é Z, Y é ST
- Se $e_T$ é NS e $\Delta_T$ é P, Y é ST

---

- Se $e_T$ é NM, Y é L
- Se $e_T$ é NS e $\Delta_T$ é Z, Y é L

---

- Se $e_T$ é NL, Y é M
- Se $e_T$ é NM e $\Delta_T$ é Z, Y é M

---

- Se $e_T$ é NL e $\Delta_T$ é Z, Y é H

---

Para o integrador, as variáveis são N (negativo), Z (zero) e P (positivo). A
saída das regras aplicadas é somada com a saída atual para encontrar o novo
valor. O acumulador só funciona a cada 4 ciclos de controle, visto que um
integrador rápido poderia inserir instabilidade no sistema.

- Se $\Delta_T$ é Z e $e_T$ é P, $Y_i$ é N

---

- Se $e_T$ é Z, $Y_i$ é Z
- Se $\Delta_T$ é N ou P, $Y_i$ é Z

---

- Se $\Delta_T$ é Z e $e_T$ é N, $Y_i$ é P

---

> As regras com mesma variável de saída foram combinadas com um OU

### Operações usadas
Foram consideradas as seguintes definições para os operadores:
- A OU B: O maior valor entre A e B;
- A E B: O menor valor entre A e B.

### Saída

A saída de cada parte é então calculada encontrando a centroide de cada
variável de saída, com pesos relativos aos seus valores, e centros considerados
nos pontos centrais das funções de participação, e somada à saída do último
instante. As funções de participação das variáveis de saída podem ser vistas
no gráfico:

![Participação para variáveis de saída da parte principal](imgs/saida.svg
"Participação para variáveis de saída da parte principal")

Para a participação de cada variável que não Z foi somado um valor de correção.
A perda de calor aumenta conforme a diferença entre a temperatura do forno e
externa aumenta. Esse valor ajuda a mitigar esse efeito. Ele é calculado em
tempo real usando a diferença entre o set point e a temperatura ambiente.

![Participação para variáveis de saída da parte integradora](imgs/saida_i.svg
"Participação para variáveis de saída da parte integradora")
