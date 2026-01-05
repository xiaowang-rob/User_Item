#ifndef __PORT_MAPPING_H__
#define __PORT_MAPPING_H__

#include "main.h"
#include "protocol.h"
typedef enum
{
    CAN_port,
    UART_port,
    USB_port
} COM_e;

typedef struct
{
    bool is_busy;
    COM_e com_port;
    u8 cmd_id;
    u8 datalen;
    u8 *data;
} RAW_FRAME_t;

typedef struct
{
    COM_e com_port;
    u8 cmd_id;
    u8 datalen;
    u8 data[MAX_frame_length];
} SEND_FRAME_t;

#endif