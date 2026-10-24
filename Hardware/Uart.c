#include "Uart.h"
#include "main.h"
#include "usart.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>


#define RECV_BUF_MAX    64

typedef void (*uart_frame_callback)(uint8_t *buf,uint8_t len);
uart_frame_callback uart_frame_cb = NULL;

u8 recv_buf[RECV_BUF_MAX];
u8 read_idx, write_idx, data_len;
static uint8_t rx_byte;        /* ← 新增：HAL 接收中断临时缓冲 */

void uart_init()
{
#if 0
    // 启用GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    // 启用USART2时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;//TX
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3; // RX
	GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitTypeDef uart_struct;

    uart_struct.USART_BaudRate = 115200;
    uart_struct.USART_WordLength = USART_WordLength_8b;
    uart_struct.USART_StopBits = USART_StopBits_1;
    uart_struct.USART_Parity = USART_Parity_No ;
    uart_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    uart_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    
    USART_Init(USART2, &uart_struct);
    USART_Cmd(USART2, ENABLE);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    
    // 配置 NVIC 中断优先级
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
#endif

    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}


// 发送单个字节
void uart_send_byte(u8 data)
{
    HAL_UART_Transmit(&huart2, &data, 1, HAL_MAX_DELAY);
}

// 发送字节数组
void uart_send_buffer(u8 *data, u16 len)
{
    HAL_UART_Transmit(&huart2, data, len, HAL_MAX_DELAY);
}

// 发送字符串
void uart_send_string(const char *str)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}

// 实现printf风格的日志函数
int uart_printf(const char *format, ...)
{
    char buffer[128];
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    if (ret > 0) uart_send_string(buffer);
    return ret;
}

// 重定向printf到UART
int fputc(int ch, FILE *f)
{
    uart_send_byte((uint8_t)ch);
    return ch;
}

// protocol => AA len d1 d2 d3 BB
void uart_recv_buffer(u8 data)
{
    // 1. 缓冲区溢出保护（写入前检查）
    if(read_idx >= RECV_BUF_MAX)
    {
        read_idx = 0;
        return;
    }

    // 2. 等待帧头
    if(read_idx == 0 && data != 0xAA)
    {
        return;
    }

    // 3. 存入数据
    recv_buf[read_idx++] = data;

    // 4. 刚收到长度字段（第2个字节），解析并检查
    if(read_idx == 2)
    {
        data_len = recv_buf[1];

        // 长度有效性检查：最小3（AA,len,BB），最大不超过缓冲区
        if(data_len < 3 || data_len > RECV_BUF_MAX)
        {
            read_idx = 0;   // 长度无效，丢弃整帧
            data_len = 0;
            return;
        }
    }

    // 5. 根据长度判断是否收完一帧
    if(read_idx == data_len)
    {
        // 检查帧尾
        if(recv_buf[data_len - 1] == 0xBB)
        {
            // 处理完整帧
            if(uart_frame_cb != NULL)
            {
                uart_frame_cb(recv_buf, data_len);
            }
        }
        // 无论对错，重置索引，准备接收下一帧
        read_idx = 0;
    }
}

void uart_frame_cb_register(uart_frame_callback cb)
{
    uart_frame_cb = cb;
}

/* 接收完成回调（替代原来的 USART2_IRQHandler 手工搬运） */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        uart_recv_buffer(rx_byte);
        HAL_UART_Receive_IT(&huart2, &rx_byte, 1);   /* 重新挂载 */
    }
}
