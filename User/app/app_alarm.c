#include "stm32f10x.h"
#include "IR.h"
#include "Uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "app/app_led.h"
#include "app/app_buzzer.h"
#include "app/app_alarm.h"
#include "app/isr.h"

/* ========== 内部队列/定时器句柄 ========== */
static QueueHandle_t xAlarmEventQueue = NULL;
static QueueHandle_t xOledDispQueue   = NULL;
static TimerHandle_t xTick1sTimer     = NULL;

/* ========== 可配置参数 ========== */
#define ARMING_COUNTDOWN_SEC    3       /* 布防倒计时（秒） */
#define ALARM_TIMEOUT_SEC       5       /* 报警后 IR 消失等待（秒） */
#define LOOP_PERIOD_MS          50      /* 主循环周期（毫秒） */

static void ir_isr_callback(void);
static void tick_1s_callback(TimerHandle_t xTimer);

/* ========== 接口函数 ========== */

void app_alarm_init(void)
{
    xAlarmEventQueue = xQueueCreate(20, sizeof(ALARM_EVT_E));
    xOledDispQueue   = xQueueCreate(3,  sizeof(OLED_DISP_T));

    /* 1秒滴答定时器，自动重载，向事件队列发送 TICK_1S */
    xTick1sTimer = xTimerCreate("Tick1s",
                                pdMS_TO_TICKS(1000),
                                pdTRUE,           /* 自动重载 */
                                NULL,
                                tick_1s_callback);
    /* 定时器在进入 ARMING / ALARM 时按需启动/重置，不在 init 启动 */

    /* 注册 IR 中断回调 */
    ir_exti_callback_register(ir_isr_callback);
}

void Alarm_SendEvent(ALARM_EVT_E evt)
{
    if (xAlarmEventQueue != NULL)
    {
        xQueueSend(xAlarmEventQueue, &evt, pdMS_TO_TICKS(20));
    }
}

QueueHandle_t Alarm_GetOledQueue(void)
{
    return xOledDispQueue;
}

/* ========== 中断回调 ========== */

/**
 * @brief IR EXTI 中断回调（由 isr.c 的 EXTI15_10_IRQHandler 调用）
 *
 * 在 ISR 上下文中读取 IR GPIO 电平，向事件队列发送 IR_HIGH/IR_LOW。
 * xQueueSendFromISR 同时唤醒阻塞在 xQueueReceive 上的 Alarm_Task。
 */
static void ir_isr_callback(void)
{
    ALARM_EVT_E evt;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    evt = IR_state_get() ? ALARM_EVT_IR_HIGH : ALARM_EVT_IR_LOW;

    if (xAlarmEventQueue != NULL)
    {
        xQueueSendFromISR(xAlarmEventQueue, &evt, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/**
 * @brief 1秒滴答定时器回调
 *
 * 通过队列发送 TICK_1S，统一进入事件驱动流程。
 * 在定时器服务任务上下文中运行，使用 xQueueSend（非 FromISR）。
 */
static void tick_1s_callback(TimerHandle_t xTimer)
{
    (void)xTimer;
    ALARM_EVT_E evt = ALARM_EVT_TICK_1S;
    if (xAlarmEventQueue != NULL)
    {
        xQueueSend(xAlarmEventQueue, &evt, 0);
    }
}

/* ========== 内部辅助函数 ========== */

/**
 * @brief 发送显示数据到 OLED 队列
 */
static void oled_disp(ALARM_MODE_E mode, uint8_t countdown, uint8_t ir_status)
{
    if (xOledDispQueue == NULL) return;
    OLED_DISP_T disp;
    disp.mode      = mode;
    disp.countdown = countdown;
    disp.ir_status = ir_status;
    xQueueSend(xOledDispQueue, &disp, 0);
}

/**
 * @brief 进入报警模式（由 IR_HIGH 边沿或 TICK_1S 兜底检测触发）
 * @param mode      当前模式指针，将被置为 ALARM
 * @param countdown 报警倒计时指针，将被重置
 * @note  非阻塞实现：不在任务中做长延时，避免事件队列积压
 */
static void enter_alarm_state(ALARM_MODE_E *mode, uint8_t *countdown)
{
    *mode      = ALARM_MODE_ALARM;
    *countdown = ALARM_TIMEOUT_SEC;
    xTimerReset(xTick1sTimer, 0);  /* 对齐滴答 */
    LED_SendCmd(LED_BLINK);
    Buzzer_SendCmd(BUZZER_CMD_BEEP_ALARM);
    uart_printf("[Alarm] IR triggered! ALARM!\r\n");
}

/* ========== 告警状态机任务 ========== */

/**
 * @brief 安防系统核心状态机（事件驱动，单一 switch）
 *
 *   DISARMED → (Key1短按) → ARMING → (3秒倒计时) → ARMED
 *                                          ↕ (Key1短按取消)
 *   ARMED    → (IR触发)   → ALARM  → (5秒无IR)    → ARMED
 *                                            ↕ (Key1短按强制撤防)
 *   * 任意模式 → (Key1长按) → CONFIG → (Key1长按) → DISARMED
 */
void Alarm_Task(void *arg)
{
    (void)arg;
    ALARM_EVT_E evt;
    ALARM_MODE_E mode      = ALARM_MODE_DISARMED;
    uint8_t      countdown = 0;
    uint8_t      ir_status = 0;

    /* 初始化 */
    oled_disp(mode, countdown, ir_status);
    uart_printf("[Alarm] System start, DISARMED\r\n");

    while (1)
    {
        /* ---- 统一阻塞等事件（按键 / IR / 滴答） ---- */
        if (xQueueReceive(xAlarmEventQueue, &evt,
                          pdMS_TO_TICKS(LOOP_PERIOD_MS)) != pdTRUE)
        {
            continue;  /* 50ms 超时，无事件，下一轮 */
        }

        /* ---- 事件驱动处理（每个模式在同一处处理所有事件类型） ---- */
        switch (mode)
        {
            /* ======== DISARMED: 撤防待机 ======== */
            case ALARM_MODE_DISARMED:
                switch (evt)
                {
                    case ALARM_EVT_KEY1_SHORT:
                        mode      = ALARM_MODE_ARMING;
                        countdown = ARMING_COUNTDOWN_SEC;
                        xTimerReset(xTick1sTimer, 0);  /* 对齐滴答 */
                        LED_SendCmd(LED_BLINK);
                        uart_printf("[Alarm] Arming... %ds\r\n", countdown);
                        break;

                    case ALARM_EVT_KEY1_LONG:
                        mode = ALARM_MODE_CONFIG;
                        uart_printf("[Alarm] Enter CONFIG\r\n");
                        break;

                    default:
                        break;
                }
                break;

            /* ======== ARMING: 布防倒计时中 ======== */
            case ALARM_MODE_ARMING:
                if (evt == ALARM_EVT_KEY1_SHORT)
                {
                    mode      = ALARM_MODE_DISARMED;
                    countdown = 0;
                    LED_SendCmd(LED_OFF);
                    uart_printf("[Alarm] Arming cancelled\r\n");
                }
                else if (evt == ALARM_EVT_TICK_1S)
                {
                    if (countdown > 0)
                    {
                        countdown--;
                        uart_printf("[Alarm] Arming... %ds\r\n", countdown);
                    }
                    if (countdown == 0)
                    {
                        mode = ALARM_MODE_ARMED;
                        LED_SendCmd(LED_ON);
                        uart_printf("[Alarm] Armed!\r\n");
                    }
                }
                break;

            /* ======== ARMED: 警戒中 ======== */
            case ALARM_MODE_ARMED:
                if (evt == ALARM_EVT_KEY1_SHORT)
                {
                    mode = ALARM_MODE_DISARMED;
                    LED_SendCmd(LED_OFF);
                    Buzzer_SendCmd(BUZZER_CMD_OFF);
                    uart_printf("[Alarm] Disarmed\r\n");
                }
                else if (evt == ALARM_EVT_KEY1_LONG)
                {
                    mode = ALARM_MODE_CONFIG;
                    uart_printf("[Alarm] Enter CONFIG\r\n");
                }
                else if (evt == ALARM_EVT_IR_HIGH)
                {
                    /* IR 边沿触发 → 立即读电平确认（非阻塞） */
                    if (IR_state_get())
                    {
                        ir_status = 1;
                        enter_alarm_state(&mode, &countdown);
                    }
                }
                else if (evt == ALARM_EVT_TICK_1S)
                {
                    /* 兜底：布防完成时 IR 已为高（无新边沿）也能在 1s 内报警 */
                    if (IR_state_get())
                    {
                        ir_status = 1;
                        enter_alarm_state(&mode, &countdown);
                    }
                }
                break;

            /* ======== ALARM: 报警中 ======== */
            case ALARM_MODE_ALARM:
                if (evt == ALARM_EVT_KEY1_SHORT)
                {
                    /* 强制撤防 */
                    mode      = ALARM_MODE_DISARMED;
                    countdown = 0;
                    ir_status = 0;
                    LED_SendCmd(LED_OFF);
                    Buzzer_SendCmd(BUZZER_CMD_OFF);
                    uart_printf("[Alarm] Force disarmed\r\n");
                }
                else if (evt == ALARM_EVT_IR_LOW)
                {
                    /* 人体离开边沿 → 立即读电平确认（非阻塞），
                       倒计时由 TICK_1S 兜底驱动 */
                    if (!IR_state_get())
                    {
                        ir_status = 0;
                    }
                }
                else if (evt == ALARM_EVT_IR_HIGH)
                {
                    /* 人体再次出现 → 重置倒计时 */
                    ir_status = 1;
                    countdown = ALARM_TIMEOUT_SEC;
                }
                else if (evt == ALARM_EVT_TICK_1S)
                {
                    /* 每秒检查：还在 → 重置；离开了 → 倒计时 */
                    if (IR_state_get())
                    {
                        ir_status = 1;
                        countdown = ALARM_TIMEOUT_SEC;
                    }
                    else
                    {
                        ir_status = 0;
                        if (countdown > 0)
                        {
                            countdown--;
                            uart_printf("[Alarm] IR clear, %ds to disarm\r\n",
                                        countdown);
                        }
                        if (countdown == 0)
                        {
                            mode = ALARM_MODE_ARMED;
                            LED_SendCmd(LED_ON);
                            Buzzer_SendCmd(BUZZER_CMD_OFF);
                            uart_printf("[Alarm] Alarm timeout, back to ARMED\r\n");
                        }
                    }
                }
                break;

            /* ======== CONFIG: 配置菜单 ======== */
            case ALARM_MODE_CONFIG:
                if (evt == ALARM_EVT_KEY1_LONG)
                {
                    mode = ALARM_MODE_DISARMED;
                    uart_printf("[Alarm] Exit CONFIG\r\n");
                }
                else if (evt == ALARM_EVT_KEY2_SHORT)
                {
                    uart_printf("[Alarm] Config: next item\r\n");
                }
                break;
        }

        /* 每次事件处理后刷新 OLED */
        oled_disp(mode, countdown, ir_status);
    }
}
