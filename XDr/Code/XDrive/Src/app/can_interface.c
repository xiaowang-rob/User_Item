#include "can_interface.h"
#include "canDr.h"
#include "stream_transmission.h"
#include "can_protocol.h"
static u8 _cmd = 0;
static u8 _index = 0;
static u8 _txdata[5];
static u8 _tx_state[2];
void CAN_RxData_Deal(u8 *RxData, u8 len)
{
    _cmd = RxData[0];
    switch (_cmd)
    {
    case CAN_CONTROL_mode_set:
        CONTROL_mode_updata(RxData[1]);
        break;
    case CAN_CONTROL_value_set:
        CONTROL_value_update((float *)&RxData[1]);
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