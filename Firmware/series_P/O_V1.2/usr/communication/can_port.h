#ifndef __CAN_PORT_H
#define __CAN_PORT_H

#include "bsp_can.h"
#include "queue.h"

typedef struct
{
    u32 id;
    u8 err_count;
    bool queue_flag;
    u8 queue_head;
    u8 queue_tail;
    tStaticQueue rx_queue;
} tCAN_handle;

// 函数声明
void fCAN_PortInit(u32 CAN_ID, bool canQUEUE);
void fCAN_SetConfig(u32 CAN_ID, bool canQUEUE);
bool fCAN_SendData(u8 *msg, u8 len);
void fCAN_RxDataCallback(u8 *RxData, u8 len);
void fCAN_QueueData_deal();

#endif