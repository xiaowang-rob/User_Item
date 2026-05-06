#ifndef __USB_PORT_H
#define __USB_PORT_H

#include "bsp_usb.h"
#include "protocol_defs.h"

typedef struct
{
    u8 head;
    u8 id;
    u8 len;
    u8 data[MAX_FRAME_LENGTH];
    u8 check;
    u8 tail;
} tUSB_Frame;

void fUSB_Init(void);
bool fUSB_SendFrame(u8 id, u8 *data, u8 len);
void fUSB_RxFrameCallback(u8 id, u8 *data, u8 len);

#endif /* __USB_INTERFACE_H */