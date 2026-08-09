#ifndef __ISR_H
#define __ISR_H

#include "stm32f10x.h"

/**
 * @brief 按键回调函数类型
 * @param key_num 按键编号 (1=Pin1, 2=Pin11)
 * @param state   按键当前引脚电平 (0=按下, 1=释放，上拉输入模式)
 */
typedef void (*key_cb_t)(uint8_t key_num, uint8_t state);
typedef void (*ir_cb_t)(void);

void key_exti_callback_register(key_cb_t cb);
void ir_exti_callback_register(ir_cb_t cb);

#endif /* __ISR_H */
