#include "can_interface.h"
#include "canDr.h"
#include "stream_transmission.h"
#include "protocol.h"
#include "foc_core.h"

static u8 idindex;
static u8 txbuffer[8];
static u8 rxtemp;
static u8 rxbuffer[8];
static u8 rxindex;
/*
控制 4byte和pvt 8byte
模式设置 2byte
模式/参数查询 3byte
数据流查询 2byte
使能 1byte
失能 1byte
*/
void CAN_RxData_Deal(u8 *RxData, u8 len)
{
    if (len == 4 || len == 8) // 参考值设置
    {
        CONTROL_value_update((float *)RxData);
    }
    else if (len == 1) // 使能失能
    {
        if (*RxData == ENABLE)
            FOC_CHANGE_STATE(FOC_ENABLE);
        else if (*RxData == DISABLE)
            FOC_CHANGE_STATE(FOC_DISABLE);
    }
    else if (len == 2 || len == 3)
    {
        idindex = RxData[1];
        switch (RxData[0])
        {
        case CMD_MODE_SET:
            CONTROL_mode_updata(RxData[1]);
            break;
        case PARAM_READ:
            if (RxData[2] == 0)
            {
                parameter_ask((Parameter_e)idindex, (u32 *)&txbuffer);
                CAN_Send_Msg(txbuffer, 4);
            }
            else
            {
                mode_ask((Mode_e)idindex, (u8 *)&txbuffer);
                CAN_Send_Msg(txbuffer, 1);
            }
            break;
        case CMD_STREAM_GET:
            stream_data_get((Data_stream_e)idindex, (float *)&txbuffer);
            CAN_Send_Msg(txbuffer, 4);
            break;
        default:
            break;
        }
    }
}
static bool _get_head = false;
void CAN_data_byte_deal(u8 data)
{
    if (_get_head)
    {
        if (rxindex < 8)
        {
            if (data == 0x5E)
            {
                CAN_RxData_Deal(rxbuffer, rxindex + 1);
                rxindex = 0;
                _get_head = false;
            }
            else
                rxbuffer[rxindex++] = data;
        }
        else
            _get_head = false;
    }
    else if (data == 0xE5)
        _get_head = true;
}
void CAN_QUEUE_Deal()
{
    while (CAN_deQUEUE_data(&rxtemp) == QUEUE_OK)
    {
        CAN_data_byte_deal(rxtemp);
    }
}
