#ifndef APP_LED_H
#define APP_LED_H

#include "FreeRTOS.h"
#include "queue.h"

/* LED 控制命令 */
typedef enum
{
    LED_ON,
    LED_OFF,
    LED_BLINK,
} LED_CMD_E;

/* 模块初始化（队列创建等），main.c 初始化阶段调用 */
void app_led_init(void);

/* 向 LED 任务发送命令，供其他模块调用 */
void LED_SendCmd(LED_CMD_E cmd);

/* LED 任务入口 */
void LED_Task(void *arg);
void LED_state_set(uint8_t state);

#endif
