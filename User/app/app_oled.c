#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

// 外部变量声明
extern SemaphoreHandle_t oled_mutex;
extern QueueHandle_t xKeyEventQueue;

typedef struct
{
	uint8_t event_id;
	uint8_t count;
}KEY_INFO_T;

enum{
	KEY_2S,
	KEY_15S
};

void OLED1_Task(void *arg)
{
	static uint8_t count;

	while(1)
	{
		if(xSemaphoreTake(oled_mutex, portMAX_DELAY))
		{
			OLED_ShowString(1, 1, "t1:");
			OLED_ShowHexNum(1, 6, count, 3);
			count++;
			xSemaphoreGive(oled_mutex);
			vTaskDelay(100);
		}
	}
}

void OLED2_Task(void *arg)
{
	KEY_INFO_T event;

	while(1)
	{
		if(xQueueReceive(xKeyEventQueue, &event, portMAX_DELAY))
		{
			switch (event.event_id)
			{
			case KEY_2S:
				OLED_ShowString(1, 1, "press 2s:");
				OLED_ShowNum(1, 12, event.count, 3);
				break;

			case KEY_15S:
				OLED_ShowString(2, 1, "press 15s:");
				OLED_ShowNum(2, 12, event.count, 3);
				break;

			default:
				break;
			}
		}
	}
}
