#include "isr.h"
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

/**
 * @brief 按键中断统一处理入口
 * @param key_num 按键编号: 1=GPIOB_Pin1, 2=GPIOB_Pin11
 *
 * 读取对应 GPIO 引脚电平后，调用已注册的回调函数。
 * 回调函数中可安全调用 FreeRTOS FromISR API（因 NVIC 优先级已调整至允许范围）。
 * 静默失败处理：无回调注册或无效按键编号时直接返回。
 */
static void key_irq_handler(uint8_t key_num)
{
    if (key_cb == NULL)
        return;

    uint8_t state = 0;
    switch (key_num)
    {
        case 1:
            state = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1);
            break;
        case 2:
            state = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11);
            break;
        default:
            return;
    }

    key_cb(key_num, state);
}

/* ========================================================================
 * 按键 EXTI 中断服务例程（强实现，覆盖 stm32f10x_it.c 中的 __weak 默认）
 * ======================================================================== */

/**
 * @brief EXTI Line1 中断处理（按键1：GPIOB Pin1）
 */
void EXTI1_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line1) != RESET)
    {
        key_irq_handler(1);
        EXTI_ClearITPendingBit(EXTI_Line1);
    }
}

/**
 * @brief EXTI Line15~10 中断处理（按键2：GPIOB Pin11 使用 Line11）
 */
void EXTI15_10_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line11) != RESET)
    {
        key_irq_handler(2);
        EXTI_ClearITPendingBit(EXTI_Line11);
    }

    if(EXTI_GetITStatus(EXTI_Line10) != RESET)
	{
		EXTI_ClearITPendingBit(EXTI_Line10);

		if(ir_exti_cb != NULL)
		{
			ir_exti_cb();
		}
	}
}
