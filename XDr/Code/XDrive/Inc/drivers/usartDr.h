#ifndef __USART_DR_H
#define __USART_DR_H

#include "main.h"
#include "usart_protocol.h"

typedef enum
{
    Data,
    Cmd,
    Ask
} USART_MSG_ID_e;
typedef struct
{
    u8 head;
    USART_MSG_ID_e msgID;
    u8 len;
    u8 data[Max_Data_Length];
    u8 check;
    u8 tail;
} Usart_Farme_t;

void usartDrInit();
void usartSendByte(u8 *data);
void usartSendData(u8 *data, u8 len);
void usartRecvByte(u8 *data);
void usart_frame_send(USART_MSG_ID_e id, u8 *data, u8 len);
void usart_farmedata_deal(u8 id, u8 *data, u8 len);
// 发送单个浮点数（VOFA+ Float 格式）
void vofa_send_float(float value);

// 发送多个浮点数（VOFA+ Float 格式）
void vofa_send_multi_float(const float *data, u8 count);

#endif