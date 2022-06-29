# Controle do forno do LITel

Esse repositório guarda o que foi desenvolvido para o controle do forno do Laboratório de Instrumentação e Telemetria da UFJF (LITel)

# Informações

O sistema funciona com aplicando um controle por lógica Fuzzy (FLC), com modulação de potência por detecção de zero, para manter a temperatura vista pelo sensor central do forno no valor especificado. Também pode funcionar por potência fixa.

O programa roda em um esp8266, programado com o framework micropython.

Para interação com o usuário existem duas interfaces de comunicação:
- WiFi
- Serial

# Comunicação WiFi

Para se comunicar com o controlador por wifi, basta estar na mesma rede. Para isso, deve-se conectar no ponto de acesso do esp ou estar na rede em que ele estiver conectado.

Uma vez conectado à mesma rede do esp, o programa pode ser controlado através das páginas acessíveis em **http://\<endereço ip\>** ou  **\<endereço ip\>:80**. O endereço depende de qual interface wifi está sendo utilizada.

## Ponto de Acesso (Access Point)

O esp está configurado para funcionar em modo "access point", permitindo que dispositivos se conectem a ele. As credenciais de acesso são:

> SSID: forno-litel
>
> Senha: 787Cu7kg

Nesse modo, o programa pode ser acessado pelo ip:

> ip: 192.168.4.1

## Modo Estação (station)

Nesse modo, o esp tentará se conectar à rede que estiver configurado para se conectar. A única forma de alterar essa configuração é pelas próprias páginas web.

> Ao mudar a configuração, ele automaticamente irá se desconectar da rede em que estava anteriormente. Logo a forma mais indicada de fazer essa alteração é acessando através do ponto de acesso.

O ip quando nesse modo depende do roteador (ou ponto de acesso no geral) em que está se conectando. Esse ip será informado na página de conexão e também pode ser descoberto pelo comando serial **!netinfo**.

## Páginas

### Página Principal

A página principal serve apenas como uma entrada à interface. Ela permite acesso às outras páginas do sistema, e também linka para esse repositório.

> Todas as outras páginas possuem também links para as outras páginas do sistema e para esse repositório.

### Conexão

Essa página mostra as informações de conexão (SSID, senha, ip), e permite mudar as credenciais da rede a ser acessada pelo modo estação.

> No caso do modo estação, a página também indica se o esp está devidamente conectado

### Controle

Essa é a página mais importante, e também a mais pesada. É normal que ela demore muitos segundos para carregar.

> Se a página estiver apresentando instabilidades, por favor reporte para mim ou crie um "issue" aqui no repositório

A página possui alguns componentes:

- Operação: 
    O forno pode funcionar por potência fixa ou por set point. Uma vez que o modo é selecionado, e o valor desejado inserido na caixa de texto, o botão enviar pode ser pressionado para enviar as informações para o microcontrolador. 

- Informações instantâneas:
    Para melhor entendimento, algumas informações são colocadas referentes ao funcionamento do forno no último instante recebido.
    - A potência enviada às resistências;
    - Se o sistema está em regime transitório ou alcançou o estacionário;
    - A temperatura no sensor principal (S5, o que fica no centro).

- Gráfico:
    Um gráfico temporal com os valores de set point (se estiver no modo de controle) e os valores lidos por todos os sensores. Cada conjunto pode ser apagado ou redesenhado para maior clareza. Também, vale notar que o gráfico somente mostra as últimas 600 amostras.

> O gráfico e as informações instantâneas são atualizados com 1 segundo. Dessa forma, o gráfico mostraria amostras dos últimos 10 minutos.


# Comunicação Serial

A interface serial funciona utilizando uma taxa (baud rate) de **115200 bits/s**, com 8 bits de dados e 1 de parada (stop bit).

A comunicação ocorre através de comandos. Para enviar um comando, deve-se enviar primeiro um caracter de início (!), então escrever o comando, seguido pelo argumento, caso exista, seguindo a formatação exigida. 

Os comandos implementados são:
- **!netinfo**: Devolve as informações de rede:
    - SSiD, senha e ip do modo ap;
    - Se estiver devidamente conectado no modo sta, SSID e IP desse modo;
    - Se não estiver, indica que o sta está desconectado;

- **!tempX**, onde X é o índice do sensor: Devolve a temperatura lida no sensor de índice X; 

- **!sensorX**, onde X é o índice do sensor: Parecido com o **temp**, mas retorna o inteiro entre 0 e 1023 que representa a leitura crua do adc (analog - digital converter) do esp;

- **!settXXX.X**, onde XXX.X é a temperatura em ºC, com três dígitos antes da vírgula, e um depois: Seta o set point do sistema de controle para o valor de temperatura passado. Para não alcançar a temperatura máxima de operação do sensor, o valor máximo que pode ser passado é 140ºC. Automaticamente ativa o modo de controle;

- **!setpXXX**, onde XXX é um inteiro representando a quantidade de semiciclos ligados: Seta a relação de potência a ser enviada para as resistências. O período da modulação de potência é de 120 semiciclos, logo o valor enviado deve estar entre 0 e 120 (inclusivo), onde 0 é carga desligada e 120 é carga em plena potência. Automaticamente ativa o modo de potência fixa;

> A formatação dos argumentos deve ser exatamente como descrita, ou o programa pode não responder adequadamente
