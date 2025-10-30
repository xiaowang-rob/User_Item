#ifndef __CAN_DR_H
#define __CAN_DR_H

#include "main.h"
#include "queue.h"

extern StaticQueue CAN_rx_queue;
void CANDr_Init(u32 CAN_ID, bool canQUEUE);
bool CAN_Send_Msg(u8 *msg, u8 len);
void CAN_RxData_Deal(u8 *RxData, u8 len);
u8 CAN_STATE_get();
#endif
