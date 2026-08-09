#include "stm32f10x.h"
#include "Buzzer.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "app/app_buzzer.h"

static QueueHandle_t xBuzzerCmdQueue = NULL;

void app_buzzer_init(void)
{
    xBuzzerCmdQueue = xQueueCreate(5, sizeof(BUZZER_CMD_E));
}

void Buzzer_SendCmd(BUZZER_CMD_E cmd)
{
    if (xBuzzerCmdQueue != NULL)
    {
        xQueueSend(xBuzzerCmdQueue, &cmd, 0);
    }
}

void Buzzer_Task(void *arg)
{
    (void)arg;
    BUZZER_CMD_E cmd;
    BUZZER_CMD_E current = BUZZER_CMD_OFF;
    uint8_t beep_once_step = 0;     /* 0:空闲, 1:已开蜂鸣器等100ms */

    while (1)
    {
        /* 根据当前模式决定等待时长 */
        TickType_t timeout;
        if (current == BUZZER_CMD_BEEP_ALARM)
            timeout = pdMS_TO_TICKS(100);       /* 100ms翻转 */
        else if (current == BUZZER_CMD_BEEP_ONCE)
            timeout = pdMS_TO_TICKS(100);       /* 鸣叫100ms */
        else
            timeout = portMAX_DELAY;            /* 常响/常灭时无限等 */

        if (xQueueReceive(xBuzzerCmdQueue, &cmd, timeout) == pdTRUE)
        {
            current = cmd;
            beep_once_step = 0;     /* 收到新命令, 重置BEEP_ONCE状态 */
        }

        switch (current)
        {
            case BUZZER_CMD_OFF:
                Buzzer_off();
                beep_once_step = 0;
                break;

            case BUZZER_CMD_ON:
                Buzzer_on();
                break;

            case BUZZER_CMD_BEEP_ALARM:
                Buzzer_toggle();    /* 每100ms翻转一次 */
                break;

            case BUZZER_CMD_BEEP_ONCE:
                if (beep_once_step == 0)
                {
                    Buzzer_on();            /* on */
                    beep_once_step = 1;
                }
                else
                {
                    Buzzer_off();           /* off */
                    current = BUZZER_CMD_OFF;
                    beep_once_step = 0;
                }
                break;

            default:
                break;
        }
    }
}
