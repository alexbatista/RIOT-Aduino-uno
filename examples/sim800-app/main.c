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
// #define THREAD2_STACKSIZE   (THREAD_STACKSIZE_MAIN)
// #define THREAD3_STACKSIZE   (THREAD_STACKSIZE_MAIN)

static char thread1_stack[THREAD1_STACKSIZE];
// static char thread2_stack[THREAD2_STACKSIZE];
// static char thread3_stack[THREAD3_STACKSIZE];

static kernel_pid_t thread1_pid;
// static kernel_pid_t thread2_pid;
// static kernel_pid_t thread3_pid;
// static kernel_pid_t main_thread_pid;

// Data sent to SIM800L module for HTTP connection _start
const char *data1 = "AT+SAPBR=1,1\r\n";
const char *data2 = "AT+HTTPINIT\r\n";
const char *data3 = "AT+HTTPPARA=\"CID\",1\r\n";
char data4[256];
const char *data5 = "AT+HTTPACTION=0\r\n";
const char *data6 = "AT+HTTPREAD\r\n";
const char *data7 = "AT+HTTPTERM\r\n";
const char *data8 = "AT+SAPBR=0,1\r\n";

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
  thread_wakeup(thread1_pid);
  msg_send(&msg,thread1_pid);
  return;
}

// Thread that sends a data to ThingSpeak using SIM800L module
void *thread_handler(void *arg){
  (void)arg;
  msg_t msg;
  while (1) {
    msg_receive(&msg);
    // Gets current system time
    time_struct = xtimer_now();
    field_value = time_struct.ticks32 % 100;
    snprintf(data4, sizeof(data4), "AT+HTTPPARA=\"URL\",\"api.thingspeak.com/update?api_key=TGGXXSHNNX87LYX7&field1=%d\"\r\n", field_value);
    // Toggles the built-in LED
    gpio_toggle(ledpin);
    printf("Thread1 is executing... Data to be sent: %d\n\n", field_value);
    // AT+SAPBR=1,1
    uart_write(SIM_UART, (uint8_t *)data1,strlen(data1)); //strlen((char *)data)
    xtimer_sleep(5);
    // AT+HTTPINIT
    uart_write(SIM_UART, (uint8_t *)data2,strlen(data2)); //strlen((char *)data)
    xtimer_sleep(1);
    // AT+HTTPPARA="CID",1
    uart_write(SIM_UART, (uint8_t *)data3,strlen(data3)); //strlen((char *)data)
    xtimer_sleep(1);
    // AT+HTTPPARA="URL",<addr>
    uart_write(SIM_UART, (uint8_t *)data4,strlen(data4)); //strlen((char *)data)
    xtimer_sleep(1);
    // AT+HTTPACTION=0
    uart_write(SIM_UART, (uint8_t *)data5,strlen(data5)); //strlen((char *)data)
    xtimer_sleep(5);
    // AT+HTTPREAD
    uart_write(SIM_UART, (uint8_t *)data6,strlen(data6)); //strlen((char *)data)
    xtimer_sleep(1);
    // AT+HTTPTERM
    uart_write(SIM_UART, (uint8_t *)data7,strlen(data7)); //strlen((char *)data)
    xtimer_sleep(1);
    // AT+SAPBR=0,1
    uart_write(SIM_UART, (uint8_t *)data8,strlen(data8)); //strlen((char *)data)
    xtimer_sleep(1);
    printf("Data sent!!!! The thread 1 is not running anymore.\n\n");
  }
  return NULL;
}

// void *thread2_handler(void *arg){
//   (void)arg;
//   msg_t msg;
//   while (1) {
//     msg_receive(&msg);
//     printf("Thread2 will send data through uart...\n");
//     uart_write(SIM_UART, (uint8_t *)data2,strlen(data2)); //strlen((char *)data)
//     xtimer_usleep(1000000U);
//     printf("Data sent!!!! The thread 2 is not running anymore.\n\n");
//     xtimer_usleep(5000000U);
//     thread_wakeup(thread3_pid);
//     msg_send(&msg,thread3_pid);
//   }
//   return NULL;
// }

// void *thread3_handler(void *arg){
//   (void)arg;
//   msg_t msg;
//   while (1) {
//     msg_receive(&msg);
//     printf("Thread3 will send data through uart...\n");
//     uart_write(SIM_UART, (uint8_t *)data2,strlen(data2)); //strlen((char *)data)
//     xtimer_usleep(1000000U);
//     printf("Data sent!!!! The thread 3 is not running anymore.\n\n");
//     xtimer_usleep(5000000U);
//     msg_send(&msg,main_thread_pid);
//   }
//   return NULL;
// }

static void rx_cb(void *uart, uint8_t c){
  msg_t msg;
  msg.type = (int)uart;
  msg.content.value = (uint32_t)c;
  printf("%c",(char)msg.content.value);
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
  // thread2_pid = thread_create(thread2_stack, THREAD2_STACKSIZE,THREAD_PRIORITY_MAIN - 1,THREAD_CREATE_SLEEPING,thread2_handler,NULL, "thread2");
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
