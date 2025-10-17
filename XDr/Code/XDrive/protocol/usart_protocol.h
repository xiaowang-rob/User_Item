#ifndef __USART_PROTOCOL_H
#define __USART_PROTOCOL_H
// 默认和校验
// 115200bps
//[包头][ID][数据长度][数据域][和校验][包尾]
#define PACKET_HEAD 0x55
#define PACKET_TAIL 0xAA
#define Max_Data_Length 256
/*
接收 包头 ID 数据长度 数据（Index+Value）    校验 包尾
发送 包头 ID 数据长度 数据（value)          校验 包尾
*/
#define PARAMETER_ask 0x01
#define STREAM_TRANSMISSION 0x02
#define CONTROL_Value_write 0x03
#define CONTROL_mode_write 0x04
#define VOFA_float_stream 0x05
#define STATUS_ask 0x06
#endif