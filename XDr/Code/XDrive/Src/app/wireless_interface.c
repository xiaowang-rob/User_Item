#include "wireless_interface.h"
#include "usartDr.h"
#include "foc_core.h"
#include "protection_manager.h"
#include "log.h"
#include "string.h"
#include "system_parameters.h"

static usart_t usart;
static u8 execute = 0xfe;
static u8 failure = 0xf0;
void usart_farmedata_deal(u8 id, u8 *data, u8 len)
{
    if (len == 0)
    {
        switch (id)
        {
        case USART_connect:
            usart.connnect_state = true;
            strcat((char *)usart.Txbuffer, SYSTEM_DESC_str);
            usart_frame_send(SYSTEM_DESC, (u8 *)&usart.Txbuffer, strlen((char *)usart.Txbuffer));
            usart.stream_index[0] = STATUS;
            usart.stream_index[1] = TEMPERATURE;
            usart.stream_index[2] = VBUS;
            usart.stream_num = 3;
            break;
        case USART_disconnect:
            usart.connnect_state = false;
            usart.stream_num = 0;
            break;
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
            usart_frame_send(LOG_ERASE, &execute, 1);
            break;
        default:
            break;
        }
        if (id == LOG_GET)
        {
            u8 _txbuf[512] = {0};
            u8 _len = 0;
            log_read(&_txbuf[0], (u32 *)&_txbuf[1], &_len, &_txbuf[5]);
            usart_frame_send(LOG_GET, _txbuf, _len + 5);
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
            usart_frame_send(PARAM_ERASE, &execute, 1);
            break;
        case PARAM_SAVE: // 一键保存 1byte
            if (data[0] == 0x00)
                if (parameter_save())
                {
                    usart_frame_send(PARAM_SAVE, &execute, 1);
                    return;
                }
            if (data[0] == 0x01)
                if (mode_save())
                {
                    usart_frame_send(PARAM_SAVE, &execute, 1);
                    return;
                }
            usart_frame_send(PARAM_SAVE, &failure, 1);
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
                parameter_ask(data[1], (u32 *)&usart.parameter_tx);
                usart_frame_send(PARAM_READ, (u8 *)&usart.parameter_tx, 4);
            }
            else if (data[0] == 0x01)
            {
                mode_ask(data[1], &usart.mode_tx);
                usart_frame_send(PARAM_READ, &usart.mode_tx, 1);
            }
            break;
        case CMD_REFVALUE_SET: // 参考值设置 4byte||8byte
            CONTROL_value_update((float *)data);
            break;
        case CMD_MODE_SET: // 模式设置 1byte
            CONTROL_mode_updata(data[0]);
            break;
        case CMD_STREAM_GET: // 监测值获取 单个值直接获取 1byte
            stream_data_get(data[0], &usart.stream_data[7]);
            usart_frame_send(CMD_STREAM_GET, (u8 *)&usart.stream_data[7], 4);
            break;
        case CMD_STREAM_SET: // 监测值设置 5byte
            usart.stream_num = len + 3;
            for (u8 i = 3; i < usart.stream_num; i++)
                usart.stream_index[i] = data[i - 3];
            break;
        default:
            break;
        }
    }
}
void usart_stream_data_trans()
{
    if (usart.stream_num == 0)
        return;
    for (u8 i = 0; i < usart.stream_num; i++)
    {
        stream_data_get(usart.stream_index[i], &usart.stream_data[i]);
    }
    usart_frame_send(CMD_STREAM_SET, (u8 *)&usart.stream_data, sizeof(float) * usart.stream_num);
}