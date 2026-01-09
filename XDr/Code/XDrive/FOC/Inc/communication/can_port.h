#ifndef __CAN_PORT_H
#define __CAN_PORT_H
#include "main.h"
#include "queue.h"

typedef struct
{
    u32 id;
    u8 err_count;
    bool queue_flag;
    u8 queue_head;
    u8 queue_tail;
    StaticQueue rx_queue;
} CAN_Handle_t;


void CAN_PORT_Init(u32 CAN_ID, bool canQUEUE);
bool CAN_Send_Msg(u8 *msg, u8 len);
QueueStatus CAN_deQUEUE_data(u8 *data);
void CAN_RxData_Deal(u8 *RxData, u8 len);

void CAN_QUEUE_Deal();
#endif