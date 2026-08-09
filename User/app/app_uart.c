#include "stm32f10x.h"                  // Device header
#include "Uart.h"
#include "LED.h"
#include "OLED.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "defines.h"
#include "app/app_led.h"
#include "app/app_buzzer.h"
#include <string.h>  // for memcpy

// 外部变量声明
extern SemaphoreHandle_t oled_mutex;

// 队列句柄
QueueHandle_t xUartQueue;

// 定义UART命令
#define CMD_LED_ON    0x01
#define CMD_LED_OFF   0x02

void uart_frame_data_handler(uint8_t *buf, uint8_t len)
{
    uint8_t send_buf[64];

    memcpy(send_buf, buf, len);
    xQueueSendFromISR(xUartQueue, send_buf, NULL);
    
    // 调试输出：打印收到的帧内容
    uart_printf("Recv frame: ");
    for(uint8_t i = 0; i < len; i++)
    {
        uart_printf("0x%02x ", buf[i]);
    }
    uart_printf("\r\n");
}

void UART_Task(void *arg)
{
    u8 buf[64];
    DEVICE_ID_E dev_id = (DEVICE_ID_E)0;
    DEVICE_STATE_E dev_state = (DEVICE_STATE_E)0;

    // 创建队列（必须在注册回调之前创建，避免回调访问未创建的队列）
    xUartQueue = xQueueCreate(10, sizeof(buf));
    
    // 注册回调（回调中使用xUartQueue，所以队列必须先创建）
    uart_frame_cb_register(uart_frame_data_handler);
    
    while(1)
    {
        // 接收队列数据
        if(xQueueReceive(xUartQueue, buf, portMAX_DELAY))
        {
            dev_id = (DEVICE_ID_E)buf[2];
            dev_state = (DEVICE_STATE_E)buf[3];

            // 解析命令
            switch(dev_id)
            {
                case DEVICE_LED:
                    LED_state_set(dev_state);
                    if(xSemaphoreTake(oled_mutex, portMAX_DELAY))
                    {
                        OLED_ShowString(1, 1, dev_state ? "LED:on " : "LED:off");
                        xSemaphoreGive(oled_mutex);
                    }
                    break;

                case DEVICE_BUZZER:
                    Buzzer_SendCmd(dev_state ? BUZZER_CMD_ON : BUZZER_CMD_OFF);
                    if(xSemaphoreTake(oled_mutex, portMAX_DELAY))
                    {
                        OLED_ShowString(1, 1, dev_state ? "Buz:on " : "Buz:off");
                        xSemaphoreGive(oled_mutex);
                    }
                    break;
                default:
                    if(xSemaphoreTake(oled_mutex, portMAX_DELAY))
                    {
                        OLED_ShowString(1, 1, "Unknown cmd");
                        xSemaphoreGive(oled_mutex);
                    }
                    break;
            }
            
            // 在OLED上显示接收到的数据
            if(xSemaphoreTake(oled_mutex, portMAX_DELAY))
            {
                OLED_ShowString(4, 1, "Data:");
                for(u8 i = 0; i < buf[1]; i++)
                {
                    OLED_ShowHexNum(4, 6 + i * 2, buf[i], 2);
                }
                xSemaphoreGive(oled_mutex);
            }
        }
    }
}

