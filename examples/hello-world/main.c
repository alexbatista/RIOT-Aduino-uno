/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Hello World application
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Ludwig Knüpfer <ludwig.knuepfer@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include "thread.h"
#include "msg.h"

char stack[THREAD_STACKSIZE_MAIN];
static kernel_pid_t thread1_pid;
// static kernel_pid_t thread2_pid;

void *thread_handler(void *arg){
  msg_t msg_req, msg_resp;
  (void)arg;
  while (1) {
      msg_receive(&msg_req);
      msg_resp.content.value = msg_req.content.value + 1;
      msg_reply(&msg_req, &msg_resp);
  }
  return NULL;
}

// void *thread_handler2(void *arg){
//   msg_t m;
//   m.content.value = 415;
//   printf("trying to send message\n");
//   if(msg_send(&m,(kernel_pid_t)arg)){
//     printf("Send msg for %"PRIkernel_pid"\n",(kernel_pid_t)arg);
//   }
//   return NULL;
// }

int main(void)
{
  msg_t msg_req, msg_resp;
  msg_resp.content.value = 0;

  printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
  printf("This board features a(n) %s MCU.\n", RIOT_MCU);
  thread1_pid = thread_create(stack, sizeof(stack),THREAD_PRIORITY_MAIN - 2,THREAD_CREATE_STACKTEST,thread_handler,NULL, "thread1");
  // thread2_pid = thread_create(stack, sizeof(stack),THREAD_PRIORITY_MAIN - 1,THREAD_CREATE_STACKTEST,thread_handler2,(void *)pid1, "thread2");

  while (1) {
     msg_req.content.value = msg_resp.content.value;
     msg_send_receive(&msg_req, &msg_resp, thread1_pid);
     printf("Result: %" PRIu32 "\n", msg_resp.content.value);
  }
  return 0;
}
