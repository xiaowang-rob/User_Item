#include "wireless_interface.h"
#include "usartDr.h"
#include "usart_protocol.h"
#include "stream_transmission.h"

static u32 _tx_data;
static u8 _float_num = 0;
static u8 _id[8] = {0};
float _float_data[8] = {0};
void usart_farmedata_deal(u8 id, u8 *data, u8 len)
{
    _float_num = 0;
    u8 _tx_state[2];
    switch (id)
    {
    case PARAMETER_ask:
        
        parameter_ask(data[0], &_tx_data);
        usart_frame_send(PARAMETER_ask, (u8 *)&_tx_data, 4);
        break;
    case STREAM_TRANSMISSION:
        // todo:以后做无线上位机的时候再写
        break;
    case CONTROL_Value_write:
        CONTROL_value_update((float *)data);
        break;
    case CONTROL_mode_write:
        CONTROL_mode_updata(data[0]);
        break;
    case VOFA_float_stream:
        _float_num = data[0];
        if (_float_num > 8)
            _float_num = 8;
        for (u8 i = 0; i < _float_num; i++)
        {
            _id[i] = data[1 + i];
        }
        break;
    case STATUS_ask:
        STATUS_get(&_tx_state[0], &_tx_state[1]);
        usart_frame_send(STATUS_ask, _tx_state, 2);
        break;
    default:
        break;
    }
}
void usart_stream_data_trans()
{
    for (u8 i = 0; i < _float_num; i++)
    {
        stream_data_get(_id[i], &_float_data[i]);
    }
    vofa_send_multi_float(_float_data, _float_num);
}