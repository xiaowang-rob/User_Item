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

COM_FRAME_t com_frame;
PORT_t port;
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
}
void fHostComputer_send()
{
    if (com_frame.com_port == UART_port)
        usart_frame_send(com_frame.cmd_id, com_frame.txdata, com_frame.txdatalen);
    else
        usb_Frame_send(com_frame.cmd_id, com_frame.txdata, com_frame.txdatalen);
}

static u8 data_id = 0;
// 命令解析
void frame_data_deal()
{
    if (com_frame.com_port == CAN_port)
    {
        switch (com_frame.cmd_id)
        {
        case CMD_REFVALUE_SET:
            CONTROL_value_update((float *)com_frame.rxdata);
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
            Param_get((Parameter_e)data_id, &com_frame.txdata, &com_frame.txdatalen);
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
            case USART_connect:
                port.connect_com = com_frame.com_port;
                com_state.usb_state = OFFLINE;
                com_state.uart_state = ONLINE;
                strcat((char *)com_frame.txdata, SYSTEM_DESC_str);
                usart_frame_send(SYSTEM_DESC, (u8 *)&com_frame.txdata, strlen((char *)com_frame.txdata));
                port.data_id_index[0] = STATUS;
                port.data_id_index[1] = TEMPERATURE;
                port.data_id_index[2] = VBUS;
                port.stream_num = 3;
                break;
            case USART_disconnect:
                port.connect_com = NONE_port;
                port.stream_num = 0;
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
            default:
                break;
            }
            if (com_frame.cmd_id == LOG_GET)
            {
                // todo:改进
            }
        }
        else if (com_frame.rxdatalen <= 5 || com_frame.rxdatalen == 8)
        {
            switch (com_frame.cmd_id)
            {
            case PARAM_WRITE: // 指定写入
                Param_set(com_frame.rxdata[0], &com_frame.rxdata[1]);
                break;
            case PARAM_READ: // 指定读取
                Param_get(com_frame.rxdata[0], com_frame.txdata, &com_frame.txdatalen);
                fHostComputer_send();
                break;
            case CMD_REFVALUE_SET: // 参考值设置 4byte||8byte
                FOC_SET_VER_VALUE((float *)com_frame.rxdata);
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
                if (port.connect_com == NONE_port)
                { // 直接 vofa float格式发送
                    port.stream_num = com_frame.txdatalen;
                    for (u8 i = 0; i < port.stream_num; i++)
                        port.data_id_index[i] = com_frame.rxdata[i];
                }
                else // usb、串口上位机
                {
                    if (port.stream_num == 0)
                    {
                        port.stream_num = 3;
                        port.data_id_index[0] = STATUS;
                        port.data_id_index[1] = TEMPERATURE;
                        port.data_id_index[2] = VBUS;
                    }
                    else
                    {
                        port.stream_num = com_frame.txdatalen + 3;
                        for (u8 i = 3; i < port.stream_num; i++)
                            port.data_id_index[i] = com_frame.rxdata[i - 3];
                    }
                }
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
    if (len == 4 || len == 8)
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
void stream_data_trans()
{
    if (port.stream_num == 0)
        return;
    _time_ms = HAL_GetTick();
    if (_time_ms - _time_prev_ms < DATA_stream_T)
        return;
    _time_prev_ms = HAL_GetTick();

    for (u8 i = 0; i < port.stream_num; i++)
    {
        stream_data_get(port.data_id_index[i], (float *)&com_frame.txdata[i * 4]);
    }
    com_frame.txdatalen = port.stream_num * 4;
    if (port.connect_com == NONE_port)
    { // 直接 vofa float格式发送
        vofa_send_multi_float((float *)com_frame.txdata, port.stream_num);
    }
    else // usb、串口上位机
    {
        fHostComputer_send();
    }
}

void communication_run()
{
    stream_data_trans();
    CAN_QUEUE_Deal();
    com_state.com_port = port.connect_com;
    com_state.can_state = CAN_state_get();
    if (USB_Connect_Status_get())
    {
        com_state.usb_state = ONLINE;
        com_state.uart_state = OFFLINE;
    }
    // todo:串口和usb状态获取 暂时没必要
}