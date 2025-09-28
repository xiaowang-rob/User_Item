#ifndef __USART_PROTOCOL_H
#define __USART_PROTOCOL_H
// 默认和校验
// 115200bps
//[包头][ID][数据长度][数据域][和校验][包尾]
#define PACKET_HEAD 0x55
#define PACKET_TAIL 0xAA
#define Max_Data_Length 256
#endif