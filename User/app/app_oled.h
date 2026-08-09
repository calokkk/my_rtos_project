#ifndef APP_OLED_H
#define APP_OLED_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

void OLED1_Task(void *arg);
void OLED2_Task(void *arg);

#endif
