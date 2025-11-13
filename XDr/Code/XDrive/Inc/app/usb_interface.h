#ifndef __USB_INTERFACE_H
#define __USB_INTERFACE_H
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
} USB_t;

void usb_cdc_run();

#endif /* __USB_INTERFACE_H */