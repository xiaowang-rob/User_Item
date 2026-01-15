#ifndef __PORT_MAPPING_H__
#define __PORT_MAPPING_H__

#include "main.h"
#include "protocol.h"
#include "DataMonitoring.h"
#include "drive_state.h"
typedef enum
{
    NONE_port,
    CAN_port,
    UART_port,
    USB_port
} COM_e;

typedef struct
{
    bool is_busy;
    COM_e com_port;
    u8 cmd_id;
    u8 rxdatalen;
    u8 *rxdata;

    u8 txdatalen;
    u8 txdata[MAX_frame_length];

    u8 stream_num;
    Data_stream_e data_id_index[8];
} COM_FRAME_t;

typedef struct
{
    COM_e Host_port;
    COM_e com_port;
    Drive_state_e can_state;
    Drive_state_e usb_state;
} communication_state_t;
communication_state_t *com_state_get_adr();

void communication_init();
void communication_run();

void usb_connected();
void usb_disconnected();

void can_state_change(Drive_state_e state);

#endif