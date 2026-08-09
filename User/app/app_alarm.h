#ifndef APP_ALARM_H
#define APP_ALARM_H

#include "FreeRTOS.h"
#include "queue.h"

/* ========== 系统工作模式 ========== */
typedef enum
{
    ALARM_MODE_DISARMED,    /* 撤防待机 */
    ALARM_MODE_ARMING,      /* 布防倒计时中 */
    ALARM_MODE_ARMED,       /* 警戒中 */
    ALARM_MODE_ALARM,       /* 报警中 */
    ALARM_MODE_CONFIG,      /* 配置菜单 */
} ALARM_MODE_E;

/* ========== 告警事件（发往 Alarm_Task） ========== */
typedef enum
{
    ALARM_EVT_KEY1_SHORT,   /* 按键1 短按 */
    ALARM_EVT_KEY1_LONG,    /* 按键1 长按 */
    ALARM_EVT_KEY2_SHORT,   /* 按键2 短按 */
    ALARM_EVT_IR_HIGH,      /* IR 检测到人体（边沿中断触发） */
    ALARM_EVT_IR_LOW,       /* IR 人体离开（边沿中断触发） */
    ALARM_EVT_TICK_1S,      /* 1秒滴答（软件定时器驱动） */
} ALARM_EVT_E;

/* ========== OLED 显示数据结构 ========== */
typedef struct
{
    ALARM_MODE_E mode;      /* 当前模式 */
    uint8_t      countdown; /* 倒计时秒数 */
    uint8_t      ir_status; /* IR 状态: 0=正常, 1=触发 */
} OLED_DISP_T;

/* ========== 对外接口 ========== */

/* 模块初始化（队列创建等），main.c 初始化阶段调用 */
void app_alarm_init(void);

/* 发送事件到告警状态机（供按键模块调用） */
void Alarm_SendEvent(ALARM_EVT_E evt);

/* 获取 OLED 显示数据队列（供 OLED 任务读取） */
QueueHandle_t Alarm_GetOledQueue(void);

/* 告警任务入口 */
void Alarm_Task(void *arg);

#endif
