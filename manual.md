# Manual de Referência do Sistema de Controle Do Forno LITel

O sistema de controle foi desenvolvido com um microcontrolador, um esp8266, programado em python com uso do framework [micropython](https://micropython.org/). Para o desenvolvimento da placa de circuito impresso (PCB), foi utilizado o [KiCad](https://www.kicad.org), um programa de Automação de Design Eletrônico (EDA) open source.

A parte física pode ser dividida em algumas seções, que foram também separadas para maior clareza no esquemático do circuito (kicad/projeto/projeto.kicad_sch):

## Divisão da parte física

### Sensor

O esp8266 possui apenas um conversor analógico - digital (adc), portanto foi necessário o uso de um multiplexador analógico (CD4051) para permitir a leitura de todos os sensores. Também, o adc suporta até 1V, enquanto o sensor pode enviar até 1.5V. Um divisor de tensão foi utilizado para reduzir essa pela metade, se encaixando com folga nos limites do microcontrolador.

O adc possui uma precisão de 10 bits, ou 1024 valores. Isso significa uma precisão de 1/1024 = 0.977mV. O sensor utilizado (LM35) entrega 10mV/ºC, com precisão típica entre 0.2ºC -> 2mV e 0.4ºC -> 4mV. 

### Status

Para visualização de alguns parâmetros diretamente na placa, três leds são utilizados:

- Steady State: Liga quando a temperatura alcança um regime permanente;

- Wifi Status: Indica o status do modo estação do esp. Enquanto está tentando se conectar, o led pisca. Se falhar / enquanto não estiver conectado devidamente, fica desligado. Quando ligado e funcionando, fica ligado;

- Ligado: Liga diretamente na linha de 3.3V da placa, indicando quando o sistema está energizado.

### Botões (Buttons)

Alguns botões são necessários para o adequado funcionamento, principalmente para desenvolvimento.

- Reset: O botão para resetar o esp;
- Exit: Sempre que se programa utilizando o micropython, é importante deixar uma forma de fazer com que o programa termine, e inicie o [REPL](https://docs.micropython.org/en/latest/esp8266/tutorial/repl.html), para que seja possível atualizar os programas. *Também, esse botão é ligado no pino de flash do esp, que deve ser mantido em nível lógico baixo (pressionado) ao ligar para permitir o upload de novo firmware*.

### Detecção de Zero (Zero Detection)

Para a estratégia de modulação implementada, é necessário que o microcontrolador saiba exatamente quando a tensão CA cruza 0V. Essa seção do circuito utiliza um diodo zener e um optoacoplador (4N25) para gerar um pulso exatamente nesse momento. 

> O diodo zener foi utilizado para aumentar a largura do pulso, que não estava sendo detectado de forma consistente em testes com apenas o optoacoplador.

### Driver da Carga (Load Driver)

Essa parte é a responsável por ativar a resistência da saída, através do uso de um optoacoplador, que permite a condução entre o Gate e o terminal A2, logo ativando a condução do triac, quando recebe um sinal de nível alto do microcontrolador.

### Alimentação (Power)

O circuito é alimentado com 5V, que pode vir tanto do conversor USB-serial (se estiver sendo utilizado) quanto de uma alimentação externa com um conector CC de 2.5mm. Os capacitores são utilizados para reduzir ruído da fonte e para garantir que, caso o microcontrolador consuma uma corrente mais alta do que a fonte / conversor são capazes de suprir por um curto período de tempo, o circuito ainda funcione.

> Embora o esperado seja 5V, a fonte pode ser de até 10V, e os circuito ainda funcionará adequadamente. Porém, vale notar que, se o circuito for alimentado por uma fonte externa, ele **não deve ser alimentado pela tensão do conversor USB-serial**. Nesse caso ainda deve se ligar o gnd do serial.

A alimentação CA é para a carga, e é protegida por um fusível (o instalado na montagem foi de 12A).

### Serial

Aqui há apenas a conexão direta entre os pinos de comunicação serial do microcontrolador com dois pinos (TX, RX), e a conexão da alimentação também em dois pinos (5V, GND)

> Ao longo de todo o circuito, estão presentes diversos jumpers. Esses foram usados apenas para facilitar a construção da PCB