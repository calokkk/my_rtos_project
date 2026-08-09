#include "stm32f10x.h"                  // Device header
#include "Buzzer.h"

void Buzzer_Init(void)
{
	RCC_APB2PeriphClockCmd (RCC_APB2Periph_GPIOA,ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	// 初始化为高电平，避免一上电就响
	GPIO_SetBits(GPIOA,GPIO_Pin_1);
}

void Buzzer_on(void)
{
    GPIO_ResetBits(GPIOA,GPIO_Pin_1);
}

void Buzzer_off(void)
{
    GPIO_SetBits(GPIOA,GPIO_Pin_1);
}

void Buzzer_toggle(void)
{
    if(GPIO_ReadOutputDataBit(GPIOA,GPIO_Pin_1))
    {
        GPIO_ResetBits(GPIOA,GPIO_Pin_1);
    }
    else
    {
        GPIO_SetBits(GPIOA,GPIO_Pin_1);
    }
}
