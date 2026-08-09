#include "stm32f10x.h"                  // Device header
#include "Delay.h"

/**
 * @brief 按键硬件初始化
 *        配置 GPIOB Pin1/Pin11 为上拉输入，
 *        映射到 EXTI Line1/Line11，
 *        使能双边沿触发中断，
 *        NVIC 优先级设为 6（允许 ISR 内安全调用 FreeRTOS FromISR API）
 */
void Key_Init(void)
{
    /* 使能 GPIOB 和 AFIO 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    /* GPIO 初始化：上拉输入 */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 将 GPIO 端口/引脚映射到 EXTI 线 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource1);   // PB1  -> EXTI1
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource11);  // PB11 -> EXTI11

    /*
     * NVIC 中断优先级说明：
     *   configMAX_SYSCALL_INTERRUPT_PRIORITY = 0x50 (抢占优先级 5)
     *   按键中断抢占优先级设为 6 → 原始值 0x60 ≥ 0x50
     *   可在 ISR 中安全调用 FreeRTOS xQueueSendFromISR / xTimerStartFromISR 等函数
     */
    NVIC_InitTypeDef NVIC_InitStructure;

    /* 配置 EXTI1 中断通道 */
    NVIC_InitStructure.NVIC_IRQChannel                   = EXTI1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_Init(&NVIC_InitStructure);

    /* 配置 EXTI15_10 中断通道（Pin11 属于 EXTI Line 11，归入 EXTI15_10_IRQn） */
    NVIC_InitStructure.NVIC_IRQChannel                   = EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 1;
    NVIC_Init(&NVIC_InitStructure);

    /* EXTI Line1 配置：中断模式，双边沿触发 */
    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line    = EXTI_Line1;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_Init(&EXTI_InitStructure);

    /* EXTI Line11 配置：中断模式，双边沿触发 */
    EXTI_InitStructure.EXTI_Line    = EXTI_Line11;
    EXTI_Init(&EXTI_InitStructure);
}

/**
 * @brief 按键扫描（轮询方式，带 20ms 消抖）
 * @retval 0=无按键按下，1=Pin1按下，2=Pin11按下
 */
uint8_t Key_GetNum(void)
{
    uint8_t KeyNum = 0;
    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)
    {
        Delay_ms(20);
        while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0);
        Delay_ms(20);
        KeyNum = 1;
    }
    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11) == 0)
    {
        Delay_ms(20);
        while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11) == 0);
        Delay_ms(20);
        KeyNum = 2;
    }

    return KeyNum;
}
