/*
USB、串口、CAN 端口映射
*/
#include "port_mapping.h"
#include "foc_statemachine.h"
#include "parameter_manager.h"
#include "system_parameters.h"
#include "log.h"
#include "protection_manager.h"
#include "can_port.h"
#include "usb_port.h"
#include "uart_port.h"
#include "string.h"

COM_FRAME_t com_frame;
communication_state_t com_state;
communication_state_t *com_state_get_adr()
{
    return &com_state;
}
static u8 execute = 0xfe;
static u8 failure = 0xf0;

void communication_init()
{
    CAN_PORT_Init(g_Param.can_id, g_Param.sw_canqueue);
    usart_port_Init();
    com_state.Host_port = NONE_port;
    com_state.com_port = NONE_port;
}
void fHostComputer_send()
{
    if (com_frame.com_port == UART_port)
        usart_frame_send(com_frame.cmd_id, com_frame.txdata, com_frame.txdatalen);
    else if (!usb_Frame_send(com_frame.cmd_id, com_frame.txdata, com_frame.txdatalen))
        com_state.Host_port = NONE_port;
}
static bool param_send_flag = false;
static u8 param_index = 0;
void all_params_send()
{
    Param_get((Parameter_e)param_index, &com_frame.txdata[1], &com_frame.txdatalen);
    com_frame.txdata[0] = param_index;
    com_frame.txdatalen += 1;
    fHostComputer_send();
    param_index++;
    if (param_index == COUNT_PARAM)
    {
        param_index = 0;
        param_send_flag = false;
    }
}
static bool log_send_flag = false;
void all_log_send()
{
    if (log_read_flash(com_frame.txdata, &com_frame.txdatalen))
        log_send_flag = false;
    else
        fHostComputer_send();
}
static u8 Noresponse_tic = 0; // 无响应次数
static bool system_message_send_flag = false;
void Status_send()
{
    com_frame.cmd_id = UC_connect;
    if (system_message_send_flag == false)
    {
        strcat((char *)com_frame.txdata, SYSTEM_DESC_str);
        com_frame.txdatalen = strlen((char *)com_frame.txdata);
        system_message_send_flag = true;
    }
    else
    {
        stream_data_get(STATUS, (float *)com_frame.txdata);
        stream_data_get(TEMPERATURE, (float *)&com_frame.txdata[4]);
        stream_data_get(VBUS, (float *)&com_frame.txdata[8]);
        com_frame.txdatalen = 12;
    }
    fHostComputer_send();
    Noresponse_tic++;
    if (Noresponse_tic > 10)
    {
        com_state.Host_port = NONE_port;
				system_message_send_flag=false;
        com_frame.stream_num = 0;
    }
}

static u8 data_id = 0;
static float value_ref[2];
// 命令解析
void frame_data_deal()
{
    if (com_frame.com_port == CAN_port)
    {
        switch (com_frame.cmd_id)
        {
        case CMD_REFVALUE_SET:
            FOC_SET_VER_VALUE((float *)com_frame.rxdata);
            break;
        case ENABLE:
            FOC_CHANGE_STATE(FOC_ENABLE);
            break;
        case DISABLE:
            FOC_CHANGE_STATE(FOC_DISABLE);
            break;
        case CMD_MODE_SET:
            FOC_SET_LOOPMODE(com_frame.rxdata[0]);
            break;
        case PARAM_READ:
            data_id = com_frame.rxdata[0];
            Param_get((Parameter_e)data_id, com_frame.txdata, &com_frame.txdatalen);
            CAN_Send_Msg(com_frame.txdata, com_frame.txdatalen);
            break;
        case CMD_STREAM_GET:
            data_id = com_frame.rxdata[0];
            stream_data_get((Data_stream_e)data_id, (float *)com_frame.txdata);
            CAN_Send_Msg(com_frame.txdata, 4);
            break;
        default:
            break;
        }
    }
    else // usb or usart
    {
        if (com_frame.rxdatalen == 0)
        {
            switch (com_frame.cmd_id)
            {
            case UC_connect:
                if (com_state.Host_port != NONE_port)
                {
                    Noresponse_tic = 0;
                }
                else
                {
                    com_state.Host_port = com_frame.com_port;
                }
                break;
            case UC_disconnect:
                system_message_send_flag = false;
                com_state.Host_port = NONE_port;
                com_frame.stream_num = 0;
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
            case LOG_GET:
                log_send_flag = true;
                break;
            case LOG_ERASE:
                log_erase();
                com_frame.txdata[0] = execute;
                com_frame.txdatalen = 1;
                fHostComputer_send();
                break;
            case PARAM_ERASE:
                Param_erase();
                com_frame.txdata[0] = execute;
                com_frame.txdatalen = 1;
                fHostComputer_send();
                break;
            case PARAM_SAVE: // 一键保存
                if (Param_save())
                    com_frame.txdata[0] = execute;
                else
                    com_frame.txdata[0] = failure;
                com_frame.txdatalen = 1;
                fHostComputer_send();
                break;
            case CMD_STREAM_SET: // 除了状态位清除检测值
                com_frame.stream_num = 0;
                break;

            default:
                break;
            }
        }
        else if (com_frame.rxdatalen <= 5 || com_frame.rxdatalen == 8)
        {
            switch (com_frame.cmd_id)
            {
            case PARAM_WRITE: // 指定写入
                Param_set(com_frame.rxdata[0], &com_frame.rxdata[1]);
                break;
            case PARAM_READ:
                if (com_frame.rxdata[0] == 0xff)
                { // 读取所有参数
                    param_send_flag = true;
                    break;
                } // 指定读取
                Param_get(com_frame.rxdata[0], com_frame.txdata, &com_frame.txdatalen);
                fHostComputer_send();
                break;
            case CMD_REFVALUE_SET: // 参考值设置 4byte||8byte
                memcpy(value_ref, com_frame.rxdata, 4);
                value_ref[1] = 0.0f;
                FOC_SET_VER_VALUE(value_ref);
                break;
            case CMD_MODE_SET: // 模式设置 1byte
                FOC_SET_LOOPMODE(com_frame.rxdata[0]);
                break;
            case CMD_STREAM_GET: // 监测值获取 单个值直接获取 1byte
                stream_data_get(com_frame.rxdata[1], (float *)com_frame.txdata);
                com_frame.txdatalen = 4;
                fHostComputer_send();
                break;
            case CMD_STREAM_SET: // 监测值设置 5byte
                com_frame.stream_num = com_frame.rxdatalen;
                for (u8 i = 0; i < com_frame.stream_num; i++)
                    com_frame.data_id_index[i] = com_frame.rxdata[i];

                break;
            default:
                break;
            }
        }
    }
    com_frame.is_busy = false;
}
// 端口映射
void CAN_RxData_Deal(u8 *RxData, u8 len)
{
    if (com_frame.is_busy)
        return;
    if (len == 4)
    {
        com_frame.cmd_id = CMD_REFVALUE_SET;
        com_frame.rxdatalen = len;
        com_frame.rxdata = RxData;
				memset(&com_frame.rxdata[4],0,4);
    }
		else if(len==8)
		{
		com_frame.cmd_id = CMD_REFVALUE_SET;
		com_frame.rxdatalen = len;
    com_frame.rxdata = RxData;	
		}
    else if (len == 1)
    {
        com_frame.cmd_id = *RxData;
        com_frame.rxdatalen = 0;
    }
    else if (len == 2)
    {
        com_frame.cmd_id = *RxData;
        com_frame.rxdatalen = 1;
        com_frame.rxdata = &RxData[1];
    }
    com_frame.is_busy = true;
    com_frame.com_port = CAN_port;
    frame_data_deal();
}

void usb_FrameData_deal(u8 id, u8 *data, u8 len)
{
    if (com_frame.is_busy)
        return;
    com_frame.is_busy = true;
    com_frame.com_port = USB_port;
    com_frame.cmd_id = id;
    com_frame.rxdatalen = len;
    com_frame.rxdata = data;
    frame_data_deal();
}

void usart_farmedata_deal(u8 id, u8 *data, u8 len)
{
    if (com_frame.is_busy)
        return;
    com_frame.is_busy = true;
    com_frame.com_port = UART_port;
    com_frame.cmd_id = id;
    com_frame.rxdatalen = len;
    com_frame.rxdata = data;
    frame_data_deal();
}

static u32 _time_ms = 0;
static u32 _time_prev_ms = 0;
static u32 _state_prev_ms = 0;
void stream_data_trans()
{
    if (com_frame.is_busy) // 端口忙
    {
        _state_prev_ms = HAL_GetTick();
        return;
    }
    _time_ms = HAL_GetTick();

    // 参数发送
    if (param_send_flag)
    {
        if (_time_ms - _time_prev_ms < DATA_stream_T)
            return;
        all_params_send();
        _time_prev_ms = HAL_GetTick();
        return;
    }
    // 日志发送
    if (log_send_flag)
    {
        if (_time_ms - _time_prev_ms < DATA_stream_T)
            return;
        all_log_send();
        _time_prev_ms = HAL_GetTick();
        return;
    }

    if (com_state.Host_port != NONE_port)
    { // 上位机连接状态下
        if ((_time_ms - _state_prev_ms > STATE_stream_T))
        { // 状态发送
            Status_send();
            _state_prev_ms = _time_ms;
        }
        else if ((_time_ms - _time_prev_ms > DATA_stream_T))
        { // 数据发送
            if (com_frame.stream_num == 0)
                return;
            com_frame.cmd_id = CMD_STREAM_SET;
            for (u8 i = 0; i < com_frame.stream_num; i++)
            {
                stream_data_get(com_frame.data_id_index[i], (float *)&com_frame.txdata[i * 4]);
            }
            com_frame.txdatalen = com_frame.stream_num * 4;
            fHostComputer_send();
            _time_prev_ms = _time_ms;
        }
    }
    else
    { // vofa端口
        if (com_frame.stream_num == 0)
            return;
        if ((_time_ms - _time_prev_ms < DATA_stream_T))
            return;
        _time_prev_ms = _time_ms;
        for (u8 i = 0; i < com_frame.stream_num; i++)
        {
            stream_data_get(com_frame.data_id_index[i], (float *)&com_frame.txdata[i * 4]);
        }
        vofa_send_multi_float((float *)com_frame.txdata, com_frame.stream_num);
    }
}
void usb_connected()
{
    com_state.usb_state = ONLINE;
}
void usb_disconnected()
{
    com_state.usb_state = OFFLINE;
}
void can_state_change(Drive_state_e state)
{
    com_state.can_state = state;
}
void communication_run()
{
    stream_data_trans();
    CAN_QUEUE_Deal();
}