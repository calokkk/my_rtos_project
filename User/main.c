#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "LED.h"
#include "Buzzer.h"
#include "OLED.h"
#include "Key.h"
#include "MyCAN.h"
#include "Uart.h"
#include "IR.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

// 包含app目录下的头文件
#include "app/app_led.h"
#include "app/app_oled.h"
#include "app/app_key.h"
#include "app/app_uart.h"
#include "app/app_buzzer.h"
#include "app/app_alarm.h"
#include "app/app_fsm.h"
#define QueueLength 10

typedef struct
{
	uint8_t event_id;
	uint8_t count;
}KEY_INFO_T;


// 全局变量声明
TaskHandle_t LED_TaskHandler;
TaskHandle_t OLED_TaskHandler;
TaskHandle_t KEY_TaskHandler;
TaskHandle_t IR_TaskHadnlter;

SemaphoreHandle_t oled_mutex;
QueueHandle_t xKeyEventQueue;

int main(void)
{
	// 初始化硬件
	OLED_Init();
	Key_Init();
	MyCAN_Init();
	LED_Init();
	Buzzer_Init();
	IR_Init();
	uart_init();

	// 初始化 app 模块（队列创建等）
	app_led_init();
	app_buzzer_init();
	app_alarm_init();

	// 创建信号量和队列
	oled_mutex = xSemaphoreCreateMutex();
	xKeyEventQueue = xQueueCreate(QueueLength, sizeof(KEY_INFO_T));

	if(oled_mutex == NULL)
	{
		
	}

	// 创建任务
	xTaskCreate(LED_Task, "LED_Task", 256, NULL, 1, &LED_TaskHandler);
	// xTaskCreate(OLED1_Task, "OLED_Task", 256, NULL, 1, &OLED_TaskHandler);
	xTaskCreate(OLED2_Task, "OLED_Task", 256, NULL, 1, NULL);
	xTaskCreate(Key_Task, "KEY_Task", 256, NULL, 1, &KEY_TaskHandler);
	xTaskCreate(UART_Task, "UART_Task", 512, NULL, 1, NULL);
	xTaskCreate(Alarm_Task, "Alarm_Task", 512, NULL, 3, NULL);
	fsm_start_with_simulate(FSM_STATE_IDLE, 10);
	vTaskStartScheduler();

	while (1)
	{

	}
}
