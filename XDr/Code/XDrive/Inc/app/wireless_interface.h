#ifndef __WIRELESS_INTERFACE_H
#define __WIRELESS_INTERFACE_H

#include "main.h"
#include "stream_transmission.h"
typedef struct
{
    bool connnect_state;
    u8 stream_num;
    Data_stream_e stream_index[8];
    float stream_data[8];
    u8 Txbuffer[64];
    u8 parameter_tx;
    u8 mode_tx;
} usart_t;
void usart_stream_data_trans();
#endif /* __WIRELESS_INTERFACE_H */