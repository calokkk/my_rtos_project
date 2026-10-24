#include "main.h"                  // Device header
#include "can.h"

void MyCAN_Init(void)
{
    CAN_FilterTypeDef filter = {0};
    filter.FilterBank           = 0;
    filter.FilterMode           = CAN_FILTERMODE_IDMASK;
    filter.FilterScale          = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh         = 0x0000;
    filter.FilterIdLow          = 0x0000;
    filter.FilterMaskIdHigh     = 0x0000;
    filter.FilterMaskIdLow      = 0x0000;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation     = ENABLE;
    HAL_CAN_ConfigFilter(&hcan, &filter);
    HAL_CAN_Start(&hcan);
}

void MyCAN_Transmit(uint32_t ID, uint8_t Length, uint8_t *Data)
{
    CAN_TxHeaderTypeDef tx = {0};
    uint32_t mailbox;
    tx.StdId = ID;
    tx.IDE   = CAN_ID_STD;
    tx.RTR   = CAN_RTR_DATA;
    tx.DLC   = Length;
    HAL_CAN_AddTxMessage(&hcan, &tx, Data, &mailbox);
}

uint8_t MyCAN_ReceiveFlag(void)
{
	return (HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0) > 0) ? 1 : 0;
}

void MyCAN_Receive(uint32_t *ID, uint8_t *Length, uint8_t *Data)
{
    CAN_RxHeaderTypeDef rx = {0};
    HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &rx, Data);
    *ID     = (rx.IDE == CAN_ID_STD) ? rx.StdId : rx.ExtId;
    *Length = rx.DLC;
}
