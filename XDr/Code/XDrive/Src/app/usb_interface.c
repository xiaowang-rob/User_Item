#include "usb_interface.h"
#include "usb_protocol.h"
#include "usbDr.h"
#include "system_parameters.h"
#include "string.h"
#include "stream_transmission.h"

void Send_System_Desc()
{
    char _str[60] = {0};
    strcat(_str, Description);
    strcat(_str, VERSION);
    strcat(_str, AUTHOR);
    strcat(_str, Frequency_string);
    strcat(_str, MAX_CURRENT_string);
    strcat(_str, MAX_Voltage_string);
    strcat(_str, MIN_Voltage_string);
    strcat(_str, MAX_Temperature_string);
    u8 len_str = strlen(_str);
    usb_Frame_send(GET_SYSTEM_DESC, (u8 *)_str, len_str);
}

void usb_FrameData_deal(u8 id, u8 *data, u8 len)
{
    u8 _Index;
    u8 _save_state = 0;
    float _tx_data;
    u8 _tx_state[2];
    switch (id)
    {
    case GET_SYSTEM_DESC:
        Send_System_Desc();
        break;
    case PARAMETER_erase:
        parameter_erase();
        break;
    case PARAMETER_read:
        if (data[0] == 0xff)
        {
            u32 _tx_data[64];
            u8 len;
            all_parameters_ask(_tx_data, &len);
            usb_Frame_send(PARAMETER_read, (u8 *)_tx_data, len * sizeof(u32));
        }
        else
        {
            u32 _tx_data;
            parameter_ask(data[0], &_tx_data);
            usb_Frame_send(PARAMETER_read, (u8 *)_tx_data, sizeof(u32));
        }
        break;
    case PARAMETER_write:
        _Index = data[0];
        if (_Index == 0xff)
            all_parameters_set((u32 *)&data[0]);
        else
            parameter_set(_Index, (u32 *)(data + 1));
        break;
    case PARAMETER_save:
        if (parameter_save())
            _save_state = 1;
        usb_Frame_send(PARAMETER_save, (u8 *)&_save_state, 1);
        break;
    case CONTROL_VALUE_write:
        CONTROL_value_update((float *)data);
        break;
    case CONTROL_MODE_write:
        CONTROL_mode_updata(data[0]);
        break;
    case STREAM_read:
        stream_data_get(data[0], &_tx_data);
        usb_Frame_send(STREAM_read, (u8 *)&_tx_data, sizeof(float));
        break;
    case STATUS_read:
        STATUS_get(&data[0], &data[1]);
        usb_Frame_send(STATUS_read, _tx_state, 2);
        break;
    default:
        break;
    }
}