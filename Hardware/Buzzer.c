#include "main.h"                  // Device header
#include "Buzzer.h"

void Buzzer_Init(void)
{
	// RCC_APB2PeriphClockCmd (RCC_APB2Periph_GPIOA,ENABLE);
	// GPIO_InitTypeDef GPIO_InitStructure;
	// GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
	// GPIO_InitStructure.GPIO_Pin=GPIO_Pin_1;
	// GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	// GPIO_Init(GPIOA,&GPIO_InitStructure);
	// // 初始化为高电平，避免一上电就响
	// GPIO_SetBits(GPIOA,GPIO_Pin_1);
}

void Buzzer_on(void)
{
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
}

void Buzzer_off(void)
{
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
}

void Buzzer_toggle(void)
{
    HAL_GPIO_TogglePin(BUZZER_GPIO_Port, BUZZER_Pin);
}
