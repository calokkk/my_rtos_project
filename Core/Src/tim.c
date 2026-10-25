/**
  ******************************************************************************
  * @file    tim.c
  * @brief   TIM1 作为 HAL 时基（1 ms）
  *
  * @note    1. STM32F103C8T6 属中密度产品，只有 TIM1~TIM4，**没有 TIM6/TIM7**，
  *             因此时基选用挂 APB2 的 TIM1（APB2 分频为 1，时钟 = PCLK2 = 72MHz）。
  *          2. 直接操作 TIM1 寄存器，不依赖 HAL_TIM 模块，
  *             避免引入 stm32f1xx_hal_tim.c 与句柄状态机，开销最小。
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "tim.h"

/* Private function prototypes -----------------------------------------------*/
static void tim1_config_1ms(void);

/**
 * @brief  把 TIM1 配置成 1 ms 更新中断（配置后不启动计数）
 * @note   F1 上 APB2 预分频不为 1 时，挂在 APB2 的定时器时钟 = PCLK2 × 2。
 *         本项目 APB2 = HCLK/1（PCLK2 = 72 MHz），故 TIM1 时钟 = 72 MHz。
 */
static void tim1_config_1ms(void)
{
    RCC_ClkInitTypeDef clkconfig;
    uint32_t           flatency;
    uint32_t           timclock;

    __HAL_RCC_TIM1_CLK_ENABLE();

    HAL_RCC_GetClockConfig(&clkconfig, &flatency);

    if (clkconfig.APB2CLKDivider == RCC_HCLK_DIV1)
    {
        timclock = HAL_RCC_GetPCLK2Freq();
    }
    else
    {
        timclock = 2UL * HAL_RCC_GetPCLK2Freq();
    }

    TIM1->CR1 &= ~TIM_CR1_CEN;                             /* 先停计数         */
    TIM1->PSC  = (uint16_t)((timclock / 1000000UL) - 1UL); /* → 1 MHz 计数时钟 */
    TIM1->ARR  = (uint16_t)(1000UL - 1UL);                 /* → 1 ms 更新周期  */
    TIM1->RCR  = 0U;                                       /* 重复计数器清零   */
    TIM1->CR1  = TIM_CR1_URS;                              /* 仅溢出才产生更新 */
    TIM1->EGR  = TIM_EGR_UG;                               /* 装载 PSC/ARR     */
    TIM1->SR   = 0UL;                                      /* 清更新标志       */
    TIM1->DIER = TIM_DIER_UIE;                             /* 开更新中断       */
}

/**
 * @brief  覆盖 stm32f1xx_hal.c 中的 __weak HAL_InitTick()
 * @note   由 HAL_Init() 内部调用。此处改由 TIM1 提供时基，
 *         SysTick 不再被 HAL 占用，完全留给 FreeRTOS。
 */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    tim1_config_1ms();

    HAL_NVIC_SetPriority(TIM1_UP_IRQn, TickPriority, 0U);
    HAL_NVIC_EnableIRQ(TIM1_UP_IRQn);

    TIM1->CR1 |= TIM_CR1_CEN;                              /* 启动计数 */

    /* HAL 约定：记录时基优先级，供后续 HAL_RCC_ClockConfig() 复用。
     * 必须赋值：否则它传入的 uwTickPrio 保持初值 (1<<4)=16，
     * 经 4 位抢占优先级截断后变成 0，时基中断会被悄悄配成最高优先级。 */
    uwTickPrio = TickPriority;

    return HAL_OK;
}
