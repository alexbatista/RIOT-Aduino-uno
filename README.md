# Sistemas de Tempo Real
## Universidade Federal de Alagoas

### Descrição do projeto
 Este repositório é um clone do [repositório oficial do RIOT OS]() com a adição de um projeto de exemplo no qual enviam-se dados para a Cloud (ThingSpeak) através de um módulo GPRS SIM800L pela UART1  do Arduino MEGA.


### Dependências do projeto

Para compilar o código e gerar o executável, será necessário ter o [AVRDUDE](http://www.nongnu.org/avrdude/) instalado.

[Tutorial de instalação Ubuntu](http://ubuntuhandbook.org/index.php/2014/09/install-avrdude-6-1-ubuntu-1404/)

[Tutorial de instalação Windows](http://www.ladyada.net/learn/avr/setup-win.html)

Para este exemplo, será necessário criar uma conta no [ThingSpeak](https://thingspeak.com/) para visualizar o dado enviado na Cloud.

### Hardwares Utilizados e suas conexões

Para execução do projeto, será necessário:
- Arduino MEGA2560
- Módulo GPRS SIM800L
- *Push Button*
- Resistor *Pull-up* 10k

O esquema de conexão dos pinos está disposta da seguinte forma:

Na imagem, o bloco do Arduino MEGA2560 está exibindo apenas os pinos utilizados na conexão. O pino D2 é o pino 2 digital do Arduino.

![](/img/CircuitoArduino-SIM800L.png)

| SIM800L       | ARDUINO MEGA2560 |
| ------------- |:-------------:   |
| RX | TX1   |
| TX      | RX1         |

Neste projeto, foi utilizada uma fonte externa para alimentação do módulo SIM800L. A tensão de alimentação pode estar entre 3.4V e 4.4V.

Além disso, foi utilizado um botão e um resistor *Pull-up* para gerar a interrupção externa.

#### Observações Importantes

- Verifique se o GND está comum ao SIM800L e o Arduino Mega, do contrário os dados não serão enviados.

- É recomendável que a fonte de alimentação externa para o SIM800L seja capaz de fornecer 2A, visto que este é o valor de pico para transmissão de dados que o módulo exige.


### Como executar

Para executar o projeto, vá para o diretório *examples/sim800-app*, e execute o seguinte comando no terminal:

```s
make BOARD=arduino-mega2560 flash
```

Ao final do processo de gravação, abra o *Serial Monitor* para o *debug* das etapas.


O código aguarda uma interrupção externa (neste caso, a interrupção é o pressionar do botão), e quando esta ocorre, o sistema gera um valor aleatório de 0 a 99 e inicia uma série de etapas para o envio deste valor para o [ThingSpeak](https://thingspeak.com/).

A sequência de comandos AT utilizados são específicos para o módulo SIM800L.
