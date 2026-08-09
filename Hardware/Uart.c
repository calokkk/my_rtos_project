#include "Uart.h"
#include "stm32f10x.h"   
#include <stdio.h>
#include <stdarg.h>

#define RECV_BUF_MAX    64

typedef void (*uart_frame_callback)(uint8_t *buf,uint8_t len);
uart_frame_callback uart_frame_cb = NULL;

u8 recv_buf[RECV_BUF_MAX];
u8 read_idx, write_idx, data_len;

void uart_init()
{
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
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}


// 发送单个字节
void uart_send_byte(u8 data)
{
    USART_SendData(USART2, data);
    while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
}

// 发送字节数组
void uart_send_buffer(u8 *data, u16 len)
{
    for(u16 i = 0; i < len; i++)
    {
        uart_send_byte(data[i]);
    }
}

// 发送字符串
void uart_send_string(const char *str)
{
    while(*str)
    {
        uart_send_byte(*str++);
    }
}

// 实现printf风格的日志函数
int uart_printf(const char *format, ...)
{
    char buffer[128];
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if(ret > 0)
    {
        uart_send_string(buffer);
    }
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


void USART2_IRQHandler(void)
{
    // 检查是否是接收中断
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        uint8_t data = USART_ReceiveData(USART2);
        uart_recv_buffer(data);  // 存入缓冲区
        // 清除中断标志（读取数据后自动清除，但显式调用更安全）
        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
    }
    
    // 如果需要处理发送完成中断
    if(USART_GetITStatus(USART2, USART_IT_TC) != RESET)
    {
        USART_ClearITPendingBit(USART2, USART_IT_TC);
    }
}
