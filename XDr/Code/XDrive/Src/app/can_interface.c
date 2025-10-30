#include "can_interface.h"
#include "canDr.h"
#include "stream_transmission.h"
#include "can_protocol.h"

static u8 _cmd = 0;
static u8 _index = 0;
static u8 _txdata[5];
static u8 _tx_state[2];

/*
控制 4bit和pvt 8bit
模式设置 1bit
参数查询 id 1bit
数据流查询 id 1bit
状态查询 1bit

*/
void CAN_RxData_Deal(u8 *RxData, u8 len)
{
    if (len == 4 || len == 8)
    {
        CONTROL_value_update((float *)&RxData[0]);
    }
    else
    {
        _cmd = RxData[0];
        switch (_cmd)
        {
        case CAN_CONTROL_mode_set:
            CONTROL_mode_updata(RxData[1]);
            break;
        case CAN_CONTROL_value_set:

            break;
        case CAN_prometer_ask:
            _index = RxData[1];
            _txdata[0] = CAN_prometer_ask;
            parameter_ask((Parameter_e)_index, (u32 *)&_txdata[1]);
            CAN_Send_Msg(_txdata, 5);
            break;
        case CAN_streamdata_ask:
            _index = RxData[1];
            _txdata[0] = CAN_streamdata_ask;
            stream_data_get((Data_stream_e)_index, (float *)&_txdata[1]);
            CAN_Send_Msg(_txdata, 5);
            break;
        case CAN_status_ask:
            STATUS_get(&_tx_state[0], &_tx_state[1]);
            CAN_Send_Msg(_tx_state, 2);
            break;
        default:
            break;
        }
    }
}
u8 _temp_data[8];
void CAN_QUEUE_Deal()
{
    u8 _data;
    if (static_queue_dequeue(&CAN_rx_queue, &_data) == QUEUE_OK)
    {
        if (_data == 0xE5)
        {
            u8 _len = 0;
            while (_len < 8)
            {
                static_queue_dequeue(&CAN_rx_queue, &_data);
                if (_data == 0x5E)
                    break;
                else
                    _temp_data[_len++] = _data;
            }
            CAN_RxData_Deal(_temp_data, _len);
        }
    }
}