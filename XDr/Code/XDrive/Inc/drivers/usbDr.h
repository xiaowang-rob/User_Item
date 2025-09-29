#ifndef __USBDR_H
#define __USBDR_H
#include "usb_protocol.h"
#include "main.h"
typedef enum
{
    HANDS,
    CMD,
    DATA,
} USB_MSG_ID_e;

typedef struct
{
    u8 head;
    USB_MSG_ID_e msg_id;
    u8 len;
    u8 data[USB_PACKET_MAX_SIZE];
    u8 check;
    u8 tail;
} USB_frame_t;

bool usb_Frame_send(USB_MSG_ID_e msg_id, u8 *data, u16 len);
void usb_FrameData_deal(USB_MSG_ID_e msg_id, u8 *data, u16 *len);
#endif /* __USB_CDC_H */