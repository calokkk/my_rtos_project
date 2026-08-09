#ifndef _APP_BUZZER_H_
#define _APP_BUZZER_H_

#include "FreeRTOS.h"
#include "queue.h"

/* 蜂鸣器控制命令 */
typedef enum
{
    BUZZER_CMD_OFF,         /* 关闭 */
    BUZZER_CMD_ON,          /* 常响 */
    BUZZER_CMD_BEEP_ALARM,  /* 报警: 100ms响/100ms停循环 */
    BUZZER_CMD_BEEP_ONCE,   /* 短鸣一声: 100ms */
} BUZZER_CMD_E;

/* 模块初始化（队列创建等），main.c 初始化阶段调用 */
void app_buzzer_init(void);

/* 向蜂鸣器任务发送命令，供其他模块调用 */
void Buzzer_SendCmd(BUZZER_CMD_E cmd);

/* 蜂鸣器任务入口 */
void Buzzer_Task(void *arg);

#endif
