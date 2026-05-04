#ifndef __UART_PORT_H
#define __UART_PORT_H

#include "main.h"
#include "usr_config.h"

typedef struct
{
    u8 head;
    u8 msgID;
    u8 len;
    u8 data[MAX_FRAME_LENGTH];
    u8 check;
    u8 tail;
} tUartFrame;

// 函数声明
void fUartPortInit();
void fUartPortSendData(u8 *data, u8 len);
void fUartPortSendFrame(u8 id, u8 *data, u8 len);
void fUartRxFrameCallback(u8 id, u8 *data, u8 len);

// 发送多个浮点数（VOFA+ Float 格式）
void fVOFA_FloatDataSend(const float *data, u8 count);

#endif