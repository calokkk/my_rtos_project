#include "stm32f10x.h"                  // Device header
#include "Key.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "app/isr.h"

/* 按键引脚定义 */
#define BTN_PIN         GPIO_Pin_1       /* 按键 GPIO 引脚 */
#define BTN_PORT        GPIOB            /* 按键 GPIO 端口 */

/* 长短按时间阈值 (ms) */
#define BTN_SHORT_PRESS_TIME        1000
#define BTN_LONG_PRESS_TIME         5000
#define BTN_LONG_LONG_PRESS_TIME    10000

/* 按键状态枚举 */
typedef enum {
    BTN_IDLE,
    BTN_PRESSED,
    BTN_PRESSED_LONG,
    BTN_PRESSED_LONG_LONG,
} BTN_STATE_E;

/* 静态变量 */
static uint8_t      btn_state      = BTN_IDLE;
static uint8_t      btn_last_state = 0;
static TimerHandle_t btn_debounce_timer = NULL;
static TimerHandle_t btn_state_timer    = NULL;

/* 硬件抽象：读取 GPIO 引脚电平 */
static inline uint8_t hal_gpio_read(uint16_t pin)
{
    return GPIO_ReadInputDataBit(BTN_PORT, pin);
}

/**
 * @brief 按键状态定时器回调
 *        管理按钮状态机的状态迁移：
 *        IDLE → PRESSED → PRESSED_LONG → PRESSED_LONG_LONG
 */
static void btn_state_handler(TimerHandle_t xTimer)
{
    (void)xTimer;
    switch (btn_state)
    {
        case BTN_IDLE:
            btn_state = BTN_PRESSED;
            xTimerChangePeriod(btn_state_timer,
                               pdMS_TO_TICKS(BTN_LONG_PRESS_TIME - BTN_SHORT_PRESS_TIME),
                               0);
            break;

        case BTN_PRESSED:
            btn_state = BTN_PRESSED_LONG;
            xTimerChangePeriod(btn_state_timer,
                               pdMS_TO_TICKS(BTN_LONG_LONG_PRESS_TIME - BTN_LONG_PRESS_TIME),
                               0);
            break;

        case BTN_PRESSED_LONG:
            btn_state = BTN_PRESSED_LONG_LONG;
            xTimerStop(btn_state_timer, 0);
            break;

        case BTN_PRESSED_LONG_LONG:
            /* 已达最长状态，保持 */
            break;

        default:
            break;
    }
}

/**
 * @brief 按键消抖定时器回调
 *        消抖时间到后确认按键状态稳定，启动状态机定时器
 */
static void btn_debounce_handler(TimerHandle_t xTimer)
{
    (void)xTimer;
    if (hal_gpio_read(BTN_PIN) == btn_last_state)
    {
        if (btn_last_state == 0)
        {
            /* 按下确认：启动状态机定时器 */
            btn_state = BTN_IDLE;
            xTimerStart(btn_state_timer, pdMS_TO_TICKS(BTN_SHORT_PRESS_TIME));
        }
        else
        {
            /* 释放确认：停止状态机，复位 */
            xTimerStop(btn_state_timer, 0);
            btn_state = BTN_IDLE;
        }
    }
    /* 否则电平抖动，丢弃本次触发 */
}

/**
 * @brief 按键 ISR 回调（由 isr.c 的 key_irq_handler 调用）
 * @param key_num 按键编号
 * @param state   按键引脚电平 (0=按下, 1=释放)
 *
 * 在中断上下文中调用，记录按键状态并启动消抖定时器。
 * 注意：必须从 FreeRTOS 定时器回调函数中使用 xTimerStartFromISR
 */
void key_isr_handler(uint8_t key_num, uint8_t state)
{
    (void)key_num;  /* 当前仅处理单按键，预留扩展 */
    btn_last_state = state;
    /* 从中断上下文启动定时器 */
    xTimerStartFromISR(btn_debounce_timer, NULL);
}

/**
 * @brief 按键任务初始化（创建消抖定时器和状态定时器，注册 ISR 回调）
 */
static void btn_init(void)
{
    btn_state      = BTN_IDLE;
    btn_last_state = hal_gpio_read(BTN_PIN);

    btn_debounce_timer = xTimerCreate("BtnDebounce",
                                      pdMS_TO_TICKS(50),
                                      pdFALSE,
                                      NULL,
                                      btn_debounce_handler);

    btn_state_timer = xTimerCreate("BtnState",
                                   pdMS_TO_TICKS(BTN_SHORT_PRESS_TIME),
                                   pdFALSE,
                                   NULL,
                                   btn_state_handler);

    /* 注册 ISR 回调，将按键中断桥接到 FreeRTOS 定时器处理 */
    key_exti_callback_register(key_isr_handler);
}

/**
 * @brief FreeRTOS 按键任务
 *        初始化按键系统后休眠；按键检测完全由中断 + 定时器驱动
 */
void Key_Task(void *arg)
{
    (void)arg;
    btn_init();

    /* 按键事件由 ISR + FreeRTOS 定时器异步处理，任务仅保持存活 */
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
