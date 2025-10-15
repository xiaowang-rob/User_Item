#ifndef __CAN_DR_H
#define __CAN_DR_H

#include "main.h"

void CANDr_Init(void);
u8 CAN_Send_Msg(u8 *msg, u8 len);

#endif
