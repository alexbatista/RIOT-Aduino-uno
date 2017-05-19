#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "xtimer.h"
#include "board.h"
#include "msg.h"
#include "thread.h"
#include "periph/uart.h"
#include "periph/gpio.h"

/*
  This example sends data through UART1 of the Arduino Mega to a SIM800L module
  connected to it. The data is sampled from the current system time.
  Using AT commands, an HTTP GET command is sent with the sampled value  to the
  ThingSpeak cloud.
*/

#define SIM_UART  UART_DEV(1)
#define BAUDRATE (9600U)
#define THREAD1_STACKSIZE   (THREAD_STACKSIZE_MAIN)
#define THREAD2_STACKSIZE   (THREAD_STACKSIZE_MAIN)
// #define THREAD3_STACKSIZE   (THREAD_STACKSIZE_MAIN)

static char thread1_stack[THREAD1_STACKSIZE];
static char thread2_stack[THREAD2_STACKSIZE];
// static char thread3_stack[THREAD3_STACKSIZE];

static kernel_pid_t thread1_pid;
static kernel_pid_t thread2_pid;
// static kernel_pid_t thread3_pid;
// static kernel_pid_t main_thread_pid;

// Data sent to SIM800L module for HTTP connection _start
uint8_t counter = 0;
char cRcv = 0;
const char *data[] = {"AT+SAPBR=1,1\r\n","AT+HTTPINIT\r\n","AT+HTTPPARA=\"CID\",1\r\n",
                    "AT+HTTPPARA=\"URL\",\"api.thingspeak.com/update?api_key=TGGXXSHNNX87LYX7&field1=%d\"\r\n",
"AT+HTTPACTION=0\r\n","AT+HTTPREAD\r\n","AT+HTTPTERM\r\n","AT+SAPBR=0,1\r\n"};

// Button and LED pins
gpio_t buttonpin = ARDUINO_PIN_2;
gpio_t ledpin = ARDUINO_PIN_13;
// Struct for getting current system time
xtimer_ticks32_t time_struct;
// Data that will be sent in the URL as a field value to ThingSpeak channel
uint8_t field_value;

// ISR that handles a button interrupt - it sends a message to Thread1
void isr_button(void *arg){
  (void)arg;
  msg_t msg;
  // thread_wakeup(thread1_pid);
  msg_send(&msg,thread1_pid);
  return;
}

// Thread that sends a data to ThingSpeak using SIM800L module
void *thread_handler(void *arg){
  (void)arg;
  msg_t msg;
  while (1) {
    msg_receive(&msg);
    if (counter == 3) {
      char data4[100];
      // Gets current system time
      time_struct = xtimer_now();
      field_value = time_struct.ticks32 % 100;
      snprintf(data4, sizeof(data4), data[counter], field_value);
      printf("Data to be sent: %d\n", field_value);
      uart_write(SIM_UART, (uint8_t *)data4,strlen(data4)); //strlen((char *)data)
    }
    else {
      uart_write(SIM_UART, (uint8_t *)data[counter],strlen(data[counter])); //strlen((char *)data)
    }
    printf("State: %d\n", counter);
  }
  return NULL;
}

void *thread2_handler(void *arg){
  (void)arg;
  msg_t msg;
  uint8_t resp = 0;
  // uint8_t state = 0;
  // uint8_t get = 0;
  // char str[50] = {0};
  // uint32_t ind = 0;
  while (1) {
    msg_receive(&msg);

    if (msg.content.value == 'O' /* capital letter 'o' */ && resp == 0) {
      resp = 1;
      printf("O");
    }
    else if (msg.content.value == 'K' /* capital letter 'K' */ && resp == 1) {
      // printf("resp: %d", resp);
      if (counter != 4){
        resp = 0;
        printf("K!\n");
        counter = (counter + 1) % 8;
        if (counter != 0)
        msg_send(&msg,thread1_pid);
      } else {
        printf("K!\n");
        resp = 2;
      }
    } else if (msg.content.value == '+' && resp == 2) {
      resp = 0;
      printf("ACTION!\n");
      counter = (counter + 1) % 8;
      msg_send(&msg,thread1_pid);
    } else if (resp != 2) {
      resp = 0;
      printf("%c", (char) msg.content.value);
    }
  }
  return NULL;
}

static void rx_cb(void *uart, uint8_t c){
  msg_t msg;
  msg.type = (int)uart;
  msg.content.value = (uint32_t)c;
  cRcv = c;
  thread_wakeup(thread2_pid);
  msg_send(&msg,thread2_pid);
  // printf("%c",(char)msg.content.value);
}

int main(void)
{
  printf("Send AT commands through UART1 from ARDUINO MEGA to SIM800L\n");
  printf("===========================================================\n");
  // xtimer_t timer;
  // xtimer_t timer2;
  /* UART1 : SIM800L */
  if (uart_init(SIM_UART, BAUDRATE, rx_cb, (void *)SIM_UART) == UART_OK)
    printf("UART 1 initialized successfully!\n");
  else printf("UART 1: INITIALIZATION ERROR!\n");
  thread1_pid = thread_create(thread1_stack, THREAD1_STACKSIZE,THREAD_PRIORITY_MAIN - 2,THREAD_CREATE_STACKTEST,thread_handler,NULL, "thread1");
  // Initialize button interrupt and its ISR handler
  if (gpio_init_int(buttonpin, GPIO_IN_PU, GPIO_RISING, &isr_button, NULL) == 0)
    printf("GPIO 2 initialized successfully!\n");
  else printf("GPIO 2: INITIALIZATION ERROR!");
  // Define LED pin (pin #13)
  if (gpio_init(ledpin, GPIO_OUT) == 0)
    printf("GPIO 13 initialized successfully!\n");
  else printf("GPIO 13: INITIALIZATION ERROR!");
  // victorrsolivera - Code_end
  thread2_pid = thread_create(thread2_stack, THREAD2_STACKSIZE,THREAD_PRIORITY_MAIN - 1,THREAD_CREATE_SLEEPING,thread2_handler,NULL, "thread2");
  // thread3_pid = thread_create(thread3_stack, THREAD3_STACKSIZE,THREAD_PRIORITY_MAIN - 3,THREAD_CREATE_SLEEPING,thread3_handler,NULL, "thread3");


  // /* Get main thread pid */
  // main_thread_pid = thread_getpid();
  // msg_t msg;
  // xtimer_set_msg(&timer, (uint32_t)4000000U,&msg, main_thread_pid);

  while(1){
    printf("Main thread executed!\n");
    xtimer_sleep(10);
    // thread_wakeup(thread1_pid);
    // msg_send(&msg,thread1_pid);
  }

  return 0;
}
