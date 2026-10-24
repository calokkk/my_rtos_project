#include "main.h"                  // Device header

void LED_Init(void)
{
	// RCC_APB2PeriphClockCmd (RCC_APB2Periph_GPIOC,ENABLE);
	// GPIO_InitTypeDef GPIO_InitStructure;
	// GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
	// GPIO_InitStructure.GPIO_Pin=GPIO_Pin_14 | GPIO_Pin_13;
	// GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	// GPIO_Init(GPIOC,&GPIO_InitStructure);
	// GPIO_SetBits(GPIOC,GPIO_Pin_14 | GPIO_Pin_13);
}
void LED1_ON(void)
{
	HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);	
}

void LED1_OFF(void)
{
	HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);	
}

void LED2_ON(void)
{
	HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_RESET);
}
void LED2_OFF(void)
{
	HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_SET);
}


void LED1_Turn(void)
{
    if (HAL_GPIO_ReadPin(LED_1_GPIO_Port, LED_1_Pin) == GPIO_PIN_RESET)
        HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);
    else
        HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);
}

void LED2_Turn(void)
{
    if (HAL_GPIO_ReadPin(LED_2_GPIO_Port, LED_2_Pin) == GPIO_PIN_RESET)
        HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_SET);
    else
        HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_RESET);
}
