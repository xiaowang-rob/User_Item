/*
USB、串口、CAN 端口映射
*/
#include "port_mapping.h"
#include "usr_config.h"

#include "DataMonitoring.h"
#include "foc_main.h"
#include "parameter_manager.h"
#include "log.h"
#include "protection_manager.h"
#include "can_port.h"
#include "usb_port.h"
#include "uart_port.h"
#include "string.h"
#include "stdio.h"

#include "bsp.h"
#include "bsp_flash.h"

#define STR(x) #x
#define XSTR(x) STR(x)

tCOM_Frame com_frame;
tCommunicationState g_com_state = {.com_port = &com_frame.com_port, .is_busy = &com_frame.is_busy};

const u8 EXECUTE = FEEDBACK_EXECUTE;
const u8 FAILURE = FEEDBACK_FAILURE;

static u8 no_response_tic = 0; // 无响应次数
static bool system_message_send_flag = false;

// 通讯层初始化
void fCommunicateInit()
{
    fCAN_PortInit(g_Param.can_id, g_Param.sw_canqueue);
    fUartPortInit();
    fUSB_Init();
    g_com_state.Host_port = NONE_port;
}
// 上位机发送 缓存中的数据
void fHostComputer_send()
{
    if (com_frame.com_port == UART_port)
        fUartPortSendFrame(com_frame.cmd_id, com_frame.txdata, com_frame.txdatalen);
    else if (!fUSB_SendFrame(com_frame.cmd_id, com_frame.txdata, com_frame.txdatalen))
    {
        g_com_state.Host_port = NONE_port;
        system_message_send_flag = false;
        com_frame.stream_num = 0;
    }
}
// 参数发送
static bool param_send_flag = false;
static u8 param_index = 0;
static inline void _all_params_send()
{
    fParamGet((eParameter)param_index, &com_frame.txdata[1], &com_frame.txdatalen);
    com_frame.txdata[0] = param_index;
    com_frame.txdatalen += 1;
    fHostComputer_send();
    param_index++;
    if (param_index == PARAM_NUM)
    {
        param_index = 0;
        param_send_flag = false;
    }
}
// 日志发送
static bool log_send_flag = false;
static inline void _all_log_send()
{
    if (fLogReadFlash(com_frame.txdata, &com_frame.txdatalen))
        log_send_flag = false;
    else
        fHostComputer_send();
}
// 状态发送
static inline void _status_send()
{
    com_frame.cmd_id = UC_CONNECT;
    if (system_message_send_flag == false)
    {
        static char drive_msg[128] = {0};
        if (drive_msg[0] == '\0')
        {
            snprintf(drive_msg, sizeof(drive_msg),
                     "%s,%s,%s,%s,%s,%s,%s,%s,%s-%s,%s",
                     FIRM_NAME,
                     FIRM_V_DATE,
                     FIRM_AUTHOR,
                     XSTR(F_PWM),
                     XSTR(F_CURRENT),
                     XSTR(F_SPEED),
                     XSTR(F_POSITION),
                     XSTR(MAX_CURRENT),
                     XSTR(MIN_VOLTAGE),
                     XSTR(MAX_VOLTAGE),
                     XSTR(MAX_TEMPERATURE));
        }
        com_frame.txdata[0] = '\0';
        strcat((char *)com_frame.txdata, drive_msg);
        com_frame.txdatalen = strlen((char *)com_frame.txdata);
        system_message_send_flag = true;
    }
    else
    {
        // 状态包 对应 usr_config.json 中的状态包顺序
        com_frame.txdata[0] = g_foc.tun->state;
        com_frame.txdata[1] = g_foc.state;
        com_frame.txdata[2] = g_pro_manager.fault;
        com_frame.txdata[3] = g_pro_manager.warning;
        memcpy(&com_frame.txdata[4], &g_pro_manager.temperature, sizeof(float));
        memcpy(&com_frame.txdata[8], &g_foc.core->motor->Udc, sizeof(float));
        com_frame.txdatalen = 12;
    }
    fHostComputer_send();
    no_response_tic++;
    if (no_response_tic > 10)
    {
        g_com_state.Host_port = NONE_port;
        system_message_send_flag = false;
        com_frame.stream_num = 0;
        no_response_tic = 0;
    }
}

static u8 data_id = 0;
static float value_ref[2];
// 命令解析
static void _frame_data_deal()
{
    if (com_frame.com_port == CAN_port)
    {
        switch (com_frame.cmd_id)
        {
        case CMD_REFVALUE_SET:
            fFOC_SetTargetValue((float *)com_frame.rxdata);
            break;
        case CMD_ENABLE:
            fFOC_StateUpdate(FOC_ENABLE);
            break;
        case CMD_DISABLE:
            fFOC_StateUpdate(FOC_DISABLE);
            break;
        case CMD_MODE_SET:
            fFOC_SetRunMode(com_frame.rxdata[0]);
            break;
        case CMD_STREAM_GET:
            data_id = com_frame.rxdata[0];
            fStreamDataGet((eData_stream)data_id, (float *)com_frame.txdata);
            fCAN_SendData(com_frame.txdata, 4);
            break;
        case CMD_SYSTEM_RESET:
            BSP_SystemReset();
            break;
        case CMD_SET_ZERO_POS:
            fFOC_SetZeroPOS(); // 以当前位置为0点
            break;
        case CMD_SET_LIMIT_POS:
            fFOC_SetLimitPOS();
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
            case UC_CONNECT:
                if (g_com_state.Host_port != NONE_port)
                {
                    no_response_tic = 0;
                }
                else
                {
                    g_com_state.Host_port = com_frame.com_port;
                }
                break;
            case UC_DISCONNECT:
                system_message_send_flag = false;
                g_com_state.Host_port = NONE_port;
                com_frame.stream_num = 0;
                break;
            case START_TUNNING:
                fFOC_StateUpdate(FOC_TUNE);
                break;
            case BRAKE:
                fFOC_StateUpdate(FOC_SHUTDOWN);
                break;
            case FOC_NRST:
                fProManagerClearFalg();
                fFOC_StateUpdate(FOC_RESET);
                break;
            case CMD_ENABLE:
                fFOC_StateUpdate(FOC_ENABLE);
                break;
            case CMD_DISABLE:
                fFOC_StateUpdate(FOC_DISABLE);
                break;
            case LOG_GET:
                log_send_flag = true;
                break;
            case LOG_ERASE:
                fLogErase();
                com_frame.txdata[0] = EXECUTE;
                com_frame.txdatalen = 1;
                fHostComputer_send();
                break;
            case PARAM_ERASE:
                fParamErase();
                com_frame.txdata[0] = EXECUTE;
                com_frame.txdatalen = 1;
                fHostComputer_send();
                break;
            case PARAM_SAVE: // 一键保存
                if (fParamSave())
                    com_frame.txdata[0] = EXECUTE;
                else
                    com_frame.txdata[0] = FAILURE;
                com_frame.txdatalen = 1;
                fHostComputer_send();
                break;
            case CMD_STREAM_SET: // 除了状态位清除检测值
                com_frame.stream_num = 0;
                break;
            case CMD_SET_ZERO_POS:
                fFOC_SetZeroPOS(); // 以当前位置为0点
                break;
            case CMD_SET_LIMIT_POS:
                fFOC_SetLimitPOS(); // 以当前位置为极限位置
                break;
            case CMD_SYSTEM_RESET: // 系统复位
                BSP_SystemReset();
                break;

            case CMD_IAP_ENTER:
                strcat((char *)com_frame.txdata, FIRM_VERSION);
                if (BSP_JumpToBootloader(com_frame.txdata, 24))
                    com_frame.txdata[0] = EXECUTE;
                else
                    com_frame.txdata[0] = FAILURE;
                com_frame.txdatalen = 1;
                fHostComputer_send();
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
                fParamSet(com_frame.rxdata[0], &com_frame.rxdata[1]);
                break;
            case PARAM_READ:
                if (com_frame.rxdata[0] == 0xff)
                { // 读取所有参数
                    param_send_flag = true;
                    break;
                } // 指定读取
                fParamGet(com_frame.rxdata[0], com_frame.txdata, &com_frame.txdatalen);
                fHostComputer_send();
                break;
            case CMD_REFVALUE_SET: // 参考值设置 4byte||8byte
                memcpy(value_ref, com_frame.rxdata, 4);
                value_ref[1] = 0.0f;
                fFOC_SetTargetValue(value_ref);
                break;
            case CMD_MODE_SET: // 模式设置 1byte
                fFOC_SetRunMode(com_frame.rxdata[0]);
                break;
            case CMD_STREAM_GET: // 监测值获取 单个值直接获取 1byte
                fStreamDataGet(com_frame.rxdata[1], (float *)com_frame.txdata);
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
void fCAN_RxDataCallback(u8 *RxData, u8 len)
{
    if (com_frame.is_busy)
        return;
    if (len == 4)
    {
        com_frame.cmd_id = CMD_REFVALUE_SET;
        com_frame.rxdatalen = len;
        com_frame.rxdata = RxData;
        memset(&com_frame.rxdata[4], 0, 4);
    }
    else if (len == 8)
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
    _frame_data_deal();
}

void fUSB_RxFrameCallback(u8 id, u8 *data, u8 len)
{
    if (com_frame.is_busy)
        return;
    com_frame.is_busy = true;
    com_frame.com_port = USB_port;
    com_frame.cmd_id = id;
    com_frame.rxdatalen = len;
    com_frame.rxdata = data;
    _frame_data_deal();
}

void fUartRxFrameCallback(u8 id, u8 *data, u8 len)
{
    if (com_frame.is_busy)
        return;
    com_frame.is_busy = true;
    com_frame.com_port = UART_port;
    com_frame.cmd_id = id;
    com_frame.rxdatalen = len;
    com_frame.rxdata = data;
    _frame_data_deal();
}

static u32 _time_ms = 0;
static u32 _time_prev_ms = 0;
static u32 _state_prev_ms = 0;
static u8 _datanum = 0;
void _stream_data_trans()
{
    if (com_frame.is_busy) // 端口忙
    {
        _state_prev_ms = BSP_GetTick();
        return;
    }
    _time_ms = BSP_GetTick();

    // 参数发送
    if (param_send_flag)
    {
        if (_time_ms - _time_prev_ms < T_DATA_STREAM)
            return;
        _all_params_send();
        _time_prev_ms = BSP_GetTick();
        return;
    }
    // 日志发送
    if (log_send_flag)
    {
        if (_time_ms - _time_prev_ms < T_DATA_STREAM)
            return;
        _all_log_send();
        _time_prev_ms = BSP_GetTick();
        return;
    }

    if (g_com_state.Host_port != NONE_port)
    { // 上位机连接状态下
        if ((_time_ms - _state_prev_ms > T_STATE_STREAM))
        { // 状态发送
            _status_send();
            _state_prev_ms = _time_ms;
            _time_prev_ms = _time_ms;
        }
        else if ((_time_ms - _time_prev_ms > T_DATA_STREAM))
        { // 数据发送
            if (com_frame.stream_num == 0)
                return;
            bool txflag = _datanum >= 12 / com_frame.stream_num * com_frame.stream_num - 1;
            fStreamDataPrepare(com_frame.data_id_index[_datanum % com_frame.stream_num], _datanum, com_frame.txdata, txflag);
            _datanum++;
            if (txflag)
            {
                com_frame.cmd_id = CMD_STREAM_SET;
                com_frame.txdatalen = _datanum * 4;
                fHostComputer_send();
                _datanum = 0;
            }
            _time_prev_ms = _time_ms;
        }
    }
    else
    { // vofa端口
        if (com_frame.stream_num == 0)
            return;
        if ((_time_ms - _time_prev_ms < T_DATA_STREAM))
            return;
        _time_prev_ms = _time_ms;
        for (u8 i = 0; i < com_frame.stream_num; i++)
        {
            fStreamDataGet(com_frame.data_id_index[i], (float *)&com_frame.txdata[i * 4]);
        }
        fVOFA_FloatDataSend((float *)com_frame.txdata, com_frame.stream_num);
    }
}

void fCommunicateMainLoop()
{
    _stream_data_trans();
    fCAN_QueueData_deal();
}