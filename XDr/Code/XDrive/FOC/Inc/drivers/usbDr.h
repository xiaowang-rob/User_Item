#ifndef __USBDR_H
#define __USBDR_H

#include "main.h"
#include "protocol.h"

typedef struct
{
    u8 head;
    u8 id;
    u8 len;
    u8 data[MAX_frame_length];
    u8 check;
    u8 tail;
} USB_frame_t;
bool usb_Frame_send(u8 id, u8 *data, u8 len);
void usb_FrameData_deal(u8 id, u8 *data, u8 len);
void USB_Connect_Status_set(u8 status);
#endif /* __USB_CDC_H */