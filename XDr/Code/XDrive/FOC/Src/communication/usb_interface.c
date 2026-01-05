#include "usb_interface.h"
#include "usbDr.h"
#include "system_parameters.h"
#include "string.h"
#include "foc_core.h"
#include "protection_manager.h"
#include "log.h"

static USB_t usb = {.stream_index = {STATUS, TEMPERATURE, VBUS}, .stream_num = 3};
static u8 execute = 0xfe;
static u8 failure = 0xf0;
void usb_FrameData_deal(u8 id, u8 *data, u8 len)
{
    if (len == 0)
    {
        if (id == LOG_GET)
        {
            u8 _txbuf[512] = {0};
            u8 _len = 0;
            log_read(&_txbuf[0], (u32 *)&_txbuf[1], &_len, &_txbuf[5]);
            usb_Frame_send(LOG_GET, _txbuf, _len + 5);
        }
        switch (id)
        {
        case START_TUNNING:
            FOC_CHANGE_STATE(FOC_AUTO_TUNE);
            break;
        case BRAKE:
            FOC_CHANGE_STATE(FOC_SHUTDOWN);
            break;
        case FOC_NRST:
            FOC_CHANGE_STATE(FOC_RESET);
            break;
        case ENABLE:
            FOC_CHANGE_STATE(FOC_ENABLE);
            break;
        case DISABLE:
            FOC_CHANGE_STATE(FOC_DISABLE);
            break;
        case PROTECT_RESET:
            protection_manager_reset();
            break;
        case LOG_ERASE:
            log_erase();
            usb_Frame_send(LOG_ERASE, &execute, 1);
            break;
        default:
            break;
        }
    }
    else if (len <= 5 || len == 8)
    {
        switch (id)
        {
        case PARAM_ERASE: // 一键擦除 1byte
            if (data[0] == 0x00)
                parameter_erase();
            if (data[0] == 0x01)
                mode_erase();
            usb_Frame_send(PARAM_ERASE, &execute, 1);
            break;
        case PARAM_SAVE: // 一键保存 1byte
            if (data[0] == 0x00)
                if (parameter_save())
                {
                    usb_Frame_send(PARAM_SAVE, &execute, 1);
                    return;
                }
            if (data[0] == 0x01)
                if (mode_save())
                {
                    usb_Frame_send(PARAM_SAVE, &execute, 1);
                    return;
                }
            usb_Frame_send(PARAM_SAVE, &failure, 1);
            break;
        case PARAM_WRITE: // 指定写入 5byte
            if (len == 5)
                parameter_set(data[0], (u32 *)&data[1]);
            else if (len == 2)
                mode_set(data[0], &data[1]);
            break;
        case PARAM_READ: // 指定读取 2byte
            if (data[0] == 0x00)
            {
                parameter_ask(data[1], (u32 *)&usb.parameter_tx);
                usb_Frame_send(PARAM_READ, (u8 *)&usb.parameter_tx, 4);
            }
            else if (data[0] == 0x01)
            {
                mode_ask(data[1], &usb.mode_tx);
                usb_Frame_send(PARAM_READ, &usb.mode_tx, 1);
            }
            break;
        case CMD_REFVALUE_SET: // 参考值设置 4byte||8byte
            CONTROL_value_update((float *)data);
            break;
        case CMD_MODE_SET: // 模式设置 1byte
            CONTROL_mode_updata(data[0]);
            break;
        case CMD_STREAM_GET: // 监测值获取 单个值直接获取 1byte
            stream_data_get(data[0], &usb.stream_data[7]);
            usb_Frame_send(CMD_STREAM_GET, (u8 *)&usb.stream_data[7], 4);
            break;
        case CMD_STREAM_SET: // 监测值设置 5byte
            usb.stream_num = len + 3;
            for (u8 i = 3; i < usb.stream_num; i++)
                usb.stream_index[i] = data[i - 3];
            break;
        default:
            break;
        }
    }
}
void usb_stream_data_trans()
{
    if (usb.stream_num == 0)
        return;
    for (u8 i = 0; i < usb.stream_num; i++)
    {
        stream_data_get(usb.stream_index[i], &usb.stream_data[i]);
    }
    usb_Frame_send(CMD_STREAM_SET, (u8 *)&usb.stream_data, sizeof(float) * usb.stream_num);
}

void usb_cdc_run()
{
    if (USB_Connect_Status_get() == 0)
    {
        if (usb.connnect_state)
            usb.connnect_state = false;
        return;
    }
    if (!usb.connnect_state)
    {
        usb.connnect_state = true;
        char _str[64] = SYSTEM_DESC_str;
        u8 len_str = strlen(_str);
        usb_Frame_send(SYSTEM_DESC, (u8 *)_str, len_str);
    }
    else
    {
        usb_stream_data_trans();
    }
}
