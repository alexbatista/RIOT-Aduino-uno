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
  Using AT commands, an HTTP GET command is sent with the sampled value to the
  ThingSpeak cloud.
*/

#define SIM_UART  UART_DEV(1)
#define BAUDRATE (9600U)
#define THREAD_SEND_DATA_STACKSIZE   (THREAD_STACKSIZE_MAIN)
#define THREAD_PROCESS_SIM_RESPONSE_STACKSIZE   (THREAD_STACKSIZE_MAIN)

//GENERAL DEFINES
#define HTTPACTION_STR_SIZE_PLUS_1 11
#define NEXT_COMMAND_INDEX(index_cmd) ((index_cmd + 1) % 8)
#define NEXT_INDEX(index) (index + 1)

static char thread_send_data_stack[THREAD_SEND_DATA_STACKSIZE];
static char thread_process_sim_response_stack[THREAD_PROCESS_SIM_RESPONSE_STACKSIZE];

static kernel_pid_t thread_send_data_pid;
static kernel_pid_t thread_process_sim_response_pid;

// Data sent to SIM800L module for HTTP connection _start
const char *simCommands[] = {
  "AT+SAPBR=1,1\r\n",
  "AT+HTTPINIT\r\n",
  "AT+HTTPPARA=\"CID\",1\r\n",
  "AT+HTTPPARA=\"URL\",\"api.thingspeak.com/update?api_key=TGGXXSHNNX87LYX7&field1=%d\"\r\n",
  "AT+HTTPACTION=0\r\n",
  "AT+HTTPREAD\r\n",
  "AT+HTTPTERM\r\n",
  "AT+SAPBR=0,1\r\n"
};

typedef enum {
  SAPBR_OPEN_INDEX,
  HTTPINIT_INDEX,
  HTTPPARA_CID_INDEX,
  HTTPPARA_URL_INDEX,
  HTTPACTION_INDEX,
  HTTPREAD_INDEX,
  HTTPTERM_INDEX,
  SAPBR_CLOSE_INDEX
} SimCommandsIndex;

SimCommandsIndex command_index = SAPBR_OPEN_INDEX;
// This enum type represents the response from the SIM800L module.

typedef enum {
  WAITING_OK,
  READING_OK,
  OK,
  WAITING_ACTION,
  READING_ACTION,
  ACTION,
  ERROR
} SimResponseState;


// Button and LED pins
gpio_t buttonpin = ARDUINO_PIN_2;
gpio_t ledpin = ARDUINO_PIN_13;
// Struct for getting current system time
xtimer_ticks32_t time_struct;
// Data that will be sent in the URL as a field value to ThingSpeak channel
uint8_t field_value;

// ISR that handles a button interrupt - it sends a message to Thread Send Data
void isr_button(void *arg){
  (void)arg;
  msg_t msg;
  msg_send(&msg,thread_send_data_pid);
  return;
}

// Thread that sends a data to ThingSpeak using SIM800L module
void *threadSendDataHandler(void *arg){
  (void)arg;
  msg_t msg;
  while (1) {
    msg_receive(&msg);
    if (command_index == HTTPPARA_URL_INDEX) {
      char data[100];
      // Gets current system time
      time_struct = xtimer_now();
      field_value = time_struct.ticks32 % 100;
      snprintf(data, sizeof(data), simCommands[command_index], field_value);
      printf("Data to be sent: %d\n", field_value);
      uart_write(SIM_UART, (uint8_t *)data,strlen(data)); //strlen((char *)data)
    }
    else {
      uart_write(SIM_UART, (uint8_t *)simCommands[command_index],strlen(simCommands[command_index])); //strlen((char *)data)
    }
    printf("Command: %d\n", command_index);
  }
  return NULL;
}

void *processSimResponseHandler(void *arg){
  (void)arg;
  msg_t msg;
  SimResponseState status = WAITING_OK;
  const char * okStr = "OK";
  const char * actionStr = "+HTTPACTION";
  const char * errorStr = "ERROR";
  uint8_t okIndex = 0;
  uint8_t actionIndex = 0;
  uint8_t errorIndex = 0;

  while (1) {
    msg_receive(&msg);
    switch (status) {

      case WAITING_OK: // waiting for an 'OK' response from SIM800L
        if (msg.content.value == okStr[okIndex]) {
          okIndex = NEXT_INDEX(okIndex);
          if (okIndex == 2) { // 2 is the size of string 'OK'
            printf("OK\n");
            okIndex = 0;
            if (command_index != HTTPACTION_INDEX){
              status = WAITING_OK;
              command_index = NEXT_COMMAND_INDEX(command_index);
              if (command_index != SAPBR_OPEN_INDEX)
                msg_send(&msg,thread_send_data_pid);
            } else {
              status = WAITING_ACTION;
            }
          }
        } else if(msg.content.value == errorStr[errorIndex]) {
          errorIndex = NEXT_INDEX(errorIndex);
          // TODO: This if catches an 'E' and 'R' from +HTTPREAD response
          if (errorIndex == 5) { // 5 is the size of string 'ERROR'
            printf("ERROR\n");
            errorIndex = 0;
          }
        }
        break;

      case WAITING_ACTION:
        if(msg.content.value == actionStr[actionIndex]){
          actionIndex = NEXT_INDEX(actionIndex);
          printf("%c", (char) msg.content.value);
          if (actionIndex == HTTPACTION_STR_SIZE_PLUS_1) { // 10 is the size of string 'HTTPACTION'
            printf(" ");
            actionIndex = 0;
            status = ACTION;
          }
        }
        break;

      case ACTION:
        if (msg.content.value != '\r') {
          printf("%c", (char) msg.content.value);
        }
        else {
          printf("\n");
          command_index = NEXT_COMMAND_INDEX(command_index);
          status = WAITING_OK;
          msg_send(&msg,thread_send_data_pid);
        }
       break;
      default:
        break;
    }
  }
  return NULL;
}

static void rx_cb(void *uart, uint8_t c){
  msg_t msg;
  msg.type = (int)uart;
  msg.content.value = (uint32_t)c;
  thread_wakeup(thread_process_sim_response_pid);
  msg_send(&msg,thread_process_sim_response_pid);
}

int main(void)
{
  printf("Send AT commands through UART1 from ARDUINO MEGA to SIM800L\n");
  printf("===========================================================\n");

  /* UART1 : SIM800L */
  if (uart_init(SIM_UART, BAUDRATE, rx_cb, (void *)SIM_UART) == UART_OK)
    printf("UART 1 initialized successfully!\n");
  else printf("UART 1: INITIALIZATION ERROR!\n");
  thread_send_data_pid = thread_create(thread_send_data_stack, THREAD_SEND_DATA_STACKSIZE,
    THREAD_PRIORITY_MAIN - 2,THREAD_CREATE_STACKTEST,threadSendDataHandler,NULL, "Send Data");
  // Initialize button interrupt and its ISR handler
  if (gpio_init_int(buttonpin, GPIO_IN_PU, GPIO_RISING, &isr_button, NULL) == 0)
    printf("GPIO 2 initialized successfully!\n");
  else
    printf("GPIO 2: INITIALIZATION ERROR!");
  // Define LED pin (pin #13)
  if (gpio_init(ledpin, GPIO_OUT) == 0)
    printf("GPIO 13 initialized successfully!\n");
  else
    printf("GPIO 13: INITIALIZATION ERROR!");

  thread_process_sim_response_pid = thread_create(thread_process_sim_response_stack,
    THREAD_PROCESS_SIM_RESPONSE_STACKSIZE,THREAD_PRIORITY_MAIN - 1,THREAD_CREATE_SLEEPING,
    processSimResponseHandler,NULL, "Process SIM response");

  while(1){
    printf("Main thread executed!\n");
    xtimer_sleep(10);
  }

  return 0;
}
