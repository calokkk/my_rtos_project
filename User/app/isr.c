#include "main.h"
#include "app/isr.h"
#include <stddef.h>

static key_cb_t key_cb = NULL;
static ir_cb_t ir_exti_cb = NULL;
/**
 * @brief 注册按键中断回调函数
 * @param cb 回调函数指针，在按键中断触发时调用
 */
void key_exti_callback_register(key_cb_t cb)
{
    key_cb = cb;
}


void ir_exti_callback_register(ir_cb_t cb)
{
	ir_exti_cb = cb;
}


/* HAL 统一 EXTI 回调，替代原来的两个 IRQHandler */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    switch (GPIO_Pin)
    {
        case KEY_1_Pin:
            if (key_cb) key_cb(1, HAL_GPIO_ReadPin(KEY_1_GPIO_Port, KEY_1_Pin));
            break;
        case KEY_2_Pin:
            if (key_cb) key_cb(2, HAL_GPIO_ReadPin(KEY_2_GPIO_Port, KEY_2_Pin));
            break;
        case IR_Pin:
            if (ir_exti_cb) ir_exti_cb();
            break;
        default:
            break;
    }
}