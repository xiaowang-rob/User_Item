#ifndef __UART_PORT_H
#define __UART_PORT_H

#include "main.h"
#include "protocol.h"

typedef struct
{
    u8 head;
    u8 msgID;
    u8 len;
    u8 data[MAX_frame_length];
    u8 check;
    u8 tail;
} Usart_Farme_t;

void usart_port_Init();
void usartSendByte(u8 *data);
void usartSendData(u8 *data, u8 len);
void usartRecvByte(u8 *data);
void usart_frame_send(u8 id, u8 *data, u8 len);
void usart_farmedata_deal(u8 id, u8 *data, u8 len);

// 发送多个浮点数（VOFA+ Float 格式）
void vofa_send_multi_float(const float *data, u8 count);

#endif