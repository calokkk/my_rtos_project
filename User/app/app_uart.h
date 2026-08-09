#ifndef APP_UART_H
#define APP_UART_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

void UART_Task(void *arg);

#endif
