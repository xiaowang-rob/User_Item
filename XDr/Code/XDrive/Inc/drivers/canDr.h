#ifndef __CAN_DR_H
#define __CAN_DR_H

#include "main.h"

typedef enum
{
    CAN_OK,
    CAN_INIT_fault,
    CAN_COMUNICATION_FAULT,
} CAN_STATE_E;

void CANDr_Init(u32 CAN_ID);
bool CAN_Send_Msg(u8 *msg, u8 len);
void CAN_RxData_Deal(u8 *RxData, u8 len);
CAN_STATE_E CAN_STATE_get();
#endif
