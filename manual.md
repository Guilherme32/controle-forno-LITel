# Manual de Referência do Sistema de Controle Do Forno LITel

O sistema de controle foi desenvolvido com um microcontrolador, um esp8266, programado em python com uso do framework [micropython](https://micropython.org/). Para o desenvolvimento da placa de circuito impresso (PCB), foi utilizado o [KiCad](https://www.kicad.org), um programa de Automação de Design Eletrônico (EDA) open source.

A parte física pode ser dividida em algumas seções, que foram também separadas para maior clareza no esquemático do circuito (kicad/projeto/projeto.kicad_sch):

# Divisão da parte física

## Sensor

O esp8266 possui apenas um conversor analógico - digital (adc), portanto foi necessário o uso de um multiplexador analógico (CD4051) para permitir a leitura de todos os sensores. Também, o adc suporta até 1V, enquanto o sensor pode enviar até 1.5V. Um divisor de tensão foi utilizado para reduzir essa pela metade, se encaixando com folga nos limites do microcontrolador.

O adc possui uma precisão de 10 bits, ou 1024 valores. Isso significa uma precisão de 1/1024 = 0.977mV. O sensor utilizado (LM35) entrega 10mV/ºC, com precisão típica entre 0.2ºC -> 2mV e 0.4ºC -> 4mV.

## Status

Para visualização de alguns parâmetros diretamente na placa, três leds são utilizados:

- Steady State: Liga quando a temperatura alcança um regime permanente;

- Wifi Status: Indica o status do modo estação do esp. Enquanto está tentando se conectar, o led pisca. Se falhar / enquanto não estiver conectado devidamente, fica desligado. Quando ligado e funcionando, fica ligado;

- Ligado: Liga diretamente na linha de 3.3V da placa, indicando quando o sistema está energizado.

## Botões (Buttons)

Alguns botões são necessários para o adequado funcionamento, principalmente para desenvolvimento.

- Reset: O botão para resetar o esp;
- Exit: Sempre que se programa utilizando o micropython, é importante deixar uma forma de fazer com que o programa termine, e inicie o [REPL](https://docs.micropython.org/en/latest/esp8266/tutorial/repl.html), para que seja possível atualizar os programas. *Também, esse botão é ligado no pino de flash do esp, que deve ser mantido em nível lógico baixo (pressionado) ao ligar para permitir o upload de novo firmware*.

## Detecção de Zero (Zero Detection)

Para a estratégia de modulação implementada, é necessário que o microcontrolador saiba exatamente quando a tensão CA cruza 0V. Essa seção do circuito utiliza um diodo zener e um optoacoplador (4N25) para gerar um pulso exatamente nesse momento.

> O diodo zener foi utilizado para aumentar a largura do pulso, que não estava sendo detectado de forma consistente em testes com apenas o optoacoplador.

## Driver da Carga (Load Driver)

Essa parte é a responsável por ativar a resistência da saída, através do uso de um optoacoplador, que permite a condução entre o Gate e o terminal A2, logo ativando a condução do triac, quando recebe um sinal de nível alto do microcontrolador.

## Alimentação (Power)

O circuito é alimentado com 5V, que pode vir tanto do conversor USB-serial (se estiver sendo utilizado) quanto de uma alimentação externa com um conector CC de 2.5mm. Os capacitores são utilizados para reduzir ruído da fonte e para garantir que, caso o microcontrolador consuma uma corrente mais alta do que a fonte / conversor são capazes de suprir por um curto período de tempo, o circuito ainda funcione.

> Embora o esperado seja 5V, a fonte pode ser de até 10V, e os circuito ainda funcionará adequadamente. Porém, vale notar que, se o circuito for alimentado por uma fonte externa, ele **não deve ser alimentado pela tensão do conversor USB-serial**. Nesse caso ainda deve se ligar o gnd do serial.

A alimentação CA é para a carga, e é protegida por um fusível (o instalado na montagem foi de 12A).

## Serial

Aqui há apenas a conexão direta entre os pinos de comunicação serial do microcontrolador com dois pinos (TX, RX), e a conexão da alimentação também em dois pinos (5V, GND)

> Ao longo de todo o circuito, estão presentes diversos jumpers. Esses foram usados apenas para facilitar a construção da PCB

# O Software

Para o projeto, foram desenvolvidos códigos para o microcontrolador (software embarcado), para automatizar tarefas, e para interface com o usuário com uma página web.

O código da página web foi desenvolvido em html, css e js, apenas como um supervisório, e os detalhes de seu conteúdo podem ser vistos nos README.md e também nas documentações e comentários dentro dos códigos.

O código para automatizar as tarefas está em tasks.py, na pasta dos códigos para o esp8266. Cada função é uma task que pode ser invocada pelo cmd com o comando (inv "task"), como parte da biblioteca **[invoke](https://www.pyinvoke.org)**. Nele estão funções referentes à interação com o sistema de arquivos do micropython.

> Vale notar que o código utiliza a porta serial que o meu computador utilizou no momento de desenvolvimento (COM3)

O software embarcado foi todo desenvolvido para ser concorrente, por meio da biblioteca uasyncio, a versão do micropython da asyncio. Cada parte do sistema foi dividido em um gerenciador, em um arquivo. Cada gerenciador possui sua corrotina assíncrona que deve ser iniciada e configurada no início do programa. Para as partes mais simples, deixarei uma breve explicação aqui. Para as mais complexas, dedicarei mais espaço aos detalhes.

> Para mais detalhes em qualquer código, basta abrir e ler os comentários e documentações das funções e classes.

- main.py: O ponto de entrada do programa do microprocessador. Cria os objetos responsáveis por cuidar de cada parte e também mantém o loop principal;

- interrupt_exit.py: Ao desenvolver um programa com o micropython, é sempre importante garantir uma forma de finalizar o código, logo iniciando a REPL por serial. Sem isso, é consideravelmente mais complicado de enviar novos arquivos para o microcontrolador;

- network_handler.py: Cuida da conexão WiFi do esp8266, tanto a inicialização do modo *Access Point* quanto de tentar conectar a uma rede externa no modo *Station*;

- sensor_reader.py: Define a classe responsável por fazer a leitura periódica de todos os sensores, manter salvos os valores de forma acessível até a próxima leitura e transformar esse valor em uma temperatura em ºC;

- serial_comm.py: Define a classe responsável por tratar as entradas recebidas na porta serial, e ativar os devidos comandos quando os seus padrões forem detectados;

- server.py: Implementa um servidor http extremamente leve e simples, apenas para permitir a transmissão dos arquivos da página web, e permitir a comunicação entre as páginas e o sistema. A interface web foi apresentada de forma mais extensa no README.md.

## controller.py

Esse script define a classe responsável por enviar à carga a potência desejada, e também por periodicamente invocar controlador propriamente dito (que toma a decisão). O algoritmo de modulação de potência utilizado foi um baseado no por detecção de zero (*Zero Detection*). O mais comumente visto dessa técnica é permitir a condução em X semiciclos, enquanto bloqueia a condução em Y. A condução do dispositivo é sempre acionada quando  tensão cruza 0V, garantindo o mínimo possível de perdas por chaveamento e também permitindo mais precisão na potência controlada, se comparado ao método por PWM, por exemplo (considerando carga atendida em CA acionada por um TRIAC). O que difere no método utilizado é a distribuição dos semiciclos de condução e corte, que não mais são consecutivos no período de funcionamento.

No método utilizado, a potência é representada por dois valores (X, Y), onde X é a quantidade de semiciclos de condução, Y a quantidade de semiciclos em corte, e X+Y o período total. Para determinar o que deve ser o semiciclo atual, uma variável é inicializada em 0 e armazenada, chamada de contador de potência (*power_counter*). Sempre que a borda entre dois semiciclos é percebida (V cruza 0V), o algoritmo define o tipo de semiciclo seguindo o seguinte pseudocódigo:

- Se **contador** > 0:  
    **contador** -= Y  
    **saída** = condução  
- Se **contador** \<= 0:  
    **contador** += x  
    **saída** = corte

Esse código mantém a relação de potência, mas torna a distribuição mais homogênea. Vale notar que ao final do período (X+Y semiciclos), o contador de potência é sempre 0. Isso permite uma troca de potência sem efeitos colaterais de causados por um valor inicial desse contador, caso a mudança seja feita nesse momento.

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


## fuzzy_controller.py

Esse script é o que de fato implementa o sistema de controle. Ele é, como o nome sugere, um controlador de lógica difusa (Fuzzy Logic Controller - FLC). O controlador foi baseado no de primeira ordem utilizado no trabalho original de controle do forno ([TCC de Leonardo Serapião](https://www.ufjf.br/mecanica/files/2016/07/Trabalho-de-Conclus%c3%a3o-de-Curso-Leonardo-Ara%c3%bajo-Serapi%c3%a3o-Engenharia-Mec%c3%a2nica.pdf)). Controladores difusos possuem bom desempenho para controle de sistemas não lineares, e com possíveis perturbações externas, que é exatamente a situação do controle de temperatura de um forno.

A lógica difusa funciona de forma parecida com uma lógica digital, porém com saltos suaves entre variáveis. As variáveis difusa podem assumir valores entre 0 e 1, representando o nível de pertencimento do valor ao espaço da variável. Esse valor será chamado de $\mu$. Em uma lógica binária, o valor seria 0 ou 1, representando que pertence ao espaço ou não, não o quanto pertence.

### O controle

O controle se dá da seguinte maneira:

Primeiro, para cada entrada, o seu valor é codificado em algumas variáveis fuzzy, através de um processo chamado de fuzzificação.

Essas variáveis difusas são então passadas por diversas regras, similares e análogas a circuitos lógicos, que determinam novas variáveis difusas de saída em função das de entrada. Cada regra mapeia um conjunto de variáveis de entrada a uma única variável de saída, mas mais de uma regra pode usar a mesma variável de saída, neste caso devem ser combinadas.

As variáveis de saída são transformadas em um único valor real (y), em um processo chamado de defuzzificação.

### Entrada

Para a entrada, dois valores foram analisados: O erro e a variação da temperatura. O erro é definido como $e[n] = T[n] - T_{set}$, onde $T_{set}$ é a temperatura alvo. A variação foi pega diretamente da temperatura, e não variação do erro, para evitar uma descontinuidade quanto o set point é trocado. Em qualquer momento diferente desse, as duas grandezas são equivalentes. Ademais, a variação de temperatura é analisada ao longo do período de chamada do algoritmo de controle (30 segundos).

> Na verdade, para facilitar os cálculos, todo o controlador foi implementado com inteiros, logo a temperatura, variação e alvo são todos em termos da leitura no ADC, e as variáveis difusas variam entre 0 e 256, em vez de de 0 a 1. A ideia ainda é a mesma, o que muda é que essas questões devem ser levadas em conta em algumas operações.

Para a variação da temperatura ($\Delta T$), 3 variáveis foram definidas: N (negative), Z (zero), P (positive). Para o erro ($e_T$), foram definidas 5: NL (negative large), NM (negative medium), NS (negative small), Z (zero) e P (positive). Para ambas, a fuzzifição foi singleton com funções de participação (*membership functions*) triangulares. Os limites podem vistos nos gráficos:

![Participação para variáveis de Delta T](/imgs/deltaT.svg "Participação para variáveis de Delta T")

![Participação para variáveis de erro](/imgs/erro.svg "Participação para variáveis de erro")

### Regras
Para a saída, as variáveis estabelecidas foram Z (zero), ST (estabilização), baixo (L),
médio (M) e alto (H).

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

> As regras com mesma variável de saída foram combinadas com um OU

### Operações usadas
Foram consideradas as seguintes definições para os operadores:
- A OU B: O maior valor entre A e B;
- A E B: O menor valor entre A e B.

### Saída

A saída é então calculada encontrando a centroide de cada variável de saída, com pesos relativos aos seus valores, e centros considerados nos pontos centrais das funções de participação, e somada à saída do último instante. As funções de participação das variáveis de saída podem ser vistas no gráfico:

![Participação para variáveis de saída](/imgs/saida.svg "Participação para variáveis de saída")
