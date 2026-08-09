#ifndef UART_H
#define UART_H

#include "stm32f10x.h"

typedef unsigned char u8;
typedef unsigned short u16;

// 回调函数类型
typedef void (*uart_frame_callback)(uint8_t *buf, uint8_t len);

void uart_init(void);
void uart_send_byte(u8 data);
void uart_send_buffer(u8 *data, u16 len);
void uart_send_string(const char *str);
int uart_printf(const char *format, ...);
void uart_recv_buffer(u8 data);
void uart_frame_cb_register(uart_frame_callback cb);

#endif
