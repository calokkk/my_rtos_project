/**
  ******************************************************************************
  * @file    tim.h
  * @brief   TIM1 作为 HAL 时基（1 ms）的驱动接口
  *
  * @note    设计目的：把 HAL 时基与 FreeRTOS 心跳彻底分开
  *          - HAL 时基      → TIM1（本文件）
  *          - FreeRTOS 心跳 → SysTick（FreeRTOS 独占）
  *          解决 HAL_Init() 打开 SysTick 后、调度器启动前进 xTaskIncrementTick()
  *          访问未初始化内核对象导致的 HardFault。
  *
  *          STM32F103C8T6 无 TIM6/TIM7，故时基用 TIM1（APB2）。
  ******************************************************************************
  */

#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported functions prototypes ---------------------------------------------*/


#ifdef __cplusplus
}
#endif

#endif /* __TIM_H__ */
