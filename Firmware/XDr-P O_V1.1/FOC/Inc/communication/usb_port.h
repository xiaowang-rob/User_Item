#ifndef __USB_PORT_H
#define __USB_PORT_H
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
} tUSB_Frame;
bool fUSB_SendFrame(u8 id, u8 *data, u8 len);
void fUSB_RxFrameCallback(u8 id, u8 *data, u8 len);

#endif /* __USB_INTERFACE_H */