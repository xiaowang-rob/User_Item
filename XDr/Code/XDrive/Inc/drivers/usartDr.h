#ifndef __USART_DR_H
#define __USART_DR_H

#include "main.h"
#include "usart_protocol.h"

typedef enum
{
    Data,
    Cmd,
    Ask
} msgID_e;
typedef enum
{
    sum,
    id
} Check_e;
typedef struct
{
    msgID_e msgID;
    u8 head;
    u8 tail;
    u8 len;
    u8 Index;
    u8 data[Max_Data_Length];
} Usart_Farme_t;

void usartDrInit();
void usartSendByte(u8 *data);
void usartSendData(u8 *data, u8 len);
void usart_frame_send(msgID_e id, u8 *data, u8 len);

// 发送单个浮点数（VOFA+ Float 格式）
void vofa_send_float(float value);

// 发送多个浮点数（VOFA+ Float 格式）
void vofa_send_multi_float(const float *data, u8 count);

#endif