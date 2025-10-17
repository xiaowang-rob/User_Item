#include "usb_interface.h"
#include "usb_protocol.h"
#include "usbDr.h"
#include "system_parameters.h"
#include "string.h"
#include "stream_transmission.h"
#include "foc_statemachine.h"
#include "foc_core.h"
#include "protection_manager.h"
#include "log.h"
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
static u8 _tx_log[256] = {0};
static u8 _log_len = 0;
static u8 _Index;
static u8 _save_state = 0;
static u8 _tx_state[2];
static u8 _float_num = 0;
static u8 _id[3] = {0};
static float _float_data[3] = {0};

void usb_FrameData_deal(u8 id, u8 *data, u8 len)
{

    switch (id)
    {
    case USB_connect:
        USB_Connect_Status_set(data[0]);
        break;
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
        parameter_apply();
        break;
    case PARAMETER_save:
        _save_state = 0;
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
        _float_num = data[0] > 3 ? 3 : data[0];
        for (u8 i = 0; i < _float_num; i++)
        {
            _id[i] = data[1 + i];
        }
        break;
    case STATUS_read:
        STATUS_get(&_tx_state[0], &_tx_state[1]);
        usb_Frame_send(STATUS_read, _tx_state, 2);
        break;
    case START_TUNNING:
        if (FOC_Get_state() != FOC_AUTO_TUNE)
            FOC_CHANGE_STATE(FOC_AUTO_TUNE);
        break;
    case BREAK_STOP:
        if (FOC_Get_state() != FOC_SHUTDOWN)
            FOC_CHANGE_STATE(FOC_SHUTDOWN);
        break;
    case CLEAR_ERROR:
        protection_manager_clear_fault();
        break;
    case READ_LOG: // 0为读取最新数据
        log_read(data[0], _tx_log, &_log_len);
        usb_Frame_send(READ_LOG, _tx_log, _log_len);
        break;
    default:
        break;
    }
}
void usb_stream_data_trans()
{
    for (u8 i = 0; i < _float_num; i++)
    {
        stream_data_get(_id[i], &_float_data[i]);
    }
    usb_Frame_send(STREAM_read, (u8 *)&_float_data, sizeof(float) * _float_num);
}
