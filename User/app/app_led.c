#include "stm32f10x.h"
#include "LED.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "app/app_led.h"
#include "app/defines.h"

static QueueHandle_t xLedcmdQueue = NULL;

void app_led_init(void)
{
    xLedcmdQueue = xQueueCreate(5, sizeof(LED_CMD_E));
}

void LED_SendCmd(LED_CMD_E cmd)
{
    if (xLedcmdQueue != NULL)
    {
        xQueueSend(xLedcmdQueue, &cmd, 0);
    }
}

void LED_Task(void *arg)
{
    (void)arg;
    LED_CMD_E cmd;

    while (1)
    {
        if (xQueueReceive(xLedcmdQueue, &cmd, portMAX_DELAY) == pdTRUE)
        {
			switch (cmd)
        	{
				case LED_ON:
					LED2_ON();
					break;

				case LED_OFF:
					LED2_OFF();
					break;

				case LED_BLINK:
					LED2_Turn();
					break;

				default:
					break;
        }
    }


    }
}

void LED_state_set(DEVICE_STATE_E state)
{
    if (state == DEVICE_ON)
        LED1_ON();
    else
        LED1_OFF();
}
