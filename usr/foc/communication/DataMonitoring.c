#include "usr_config.h"
#include "foc_main.h"
#include "protection_manager.h"
#include "svpwm.h"
#include "string.h"

static u8 _sta[4];
static float temp_val = 0;

static u8 txdata_queue[64];

void fStreamDataGet(DataStreamId stream, float *data)
{

    switch (stream)
    {
    case STREAM_STATUS:
        // todo:先空着，后续可以添加can速率等
        _sta[0] = 0; // 预留位
        _sta[1] = g_foc.state;
        _sta[2] = g_pro_manager.fault;
        _sta[3] = g_pro_manager.warning;
        memcpy(data, _sta, 4);
        break;
    case STREAM_TEMPERATURE:
        *data = g_pro_manager.temperature;
        break;
    case STREAM_VBUS:
        *data = g_foc.core->motor->Udc;
        break;
    case STREAM_VOLTAGE_U:
        //        temp_val = fGetVoltage_u();
        temp_val = g_foc.core->foc_val->Iu;
        memcpy(data, &temp_val, 4);
        break;
    case STREAM_VOLTAGE_V:
        //        temp_val = fGetVoltage_v();
        temp_val = g_foc.core->foc_val->Iv;
        memcpy(data, &temp_val, 4);
        break;
    case STREAM_VOLTAGE_W:
        //        temp_val = fGetVoltage_w();
        temp_val = g_foc.core->foc_val->Iw;
        memcpy(data, &temp_val, 4);
        break;
    case STREAM_VOLTAGE_Q:
        *data = g_foc.core->foc_val->uq;
        break;
    case STREAM_VOLTAGE_D:
        *data = g_foc.core->foc_val->ud;
        break;
    case STREAM_CURRENT_ALPHA:
        *data = g_foc.core->foc_val->Ialpha;
        //*data = g_foc.core->foc_val->Ualpha_hfi;
        //*data = g_foc.core->foc_val->Ualpha;
        //		temp_val =g_foc.core->foc_val->Iu;
        //		*data =temp_val;
        break;
    case STREAM_CURRENT_BETA:
        *data = g_foc.core->foc_val->Ibeta;
        //*data = g_foc.core->foc_val->Ubeta_hfi;
        //*data = g_foc.core->foc_val->Ubeta;
        //		temp_val = fGetVoltage_u();
        //		*data =temp_val;
        break;
    case STREAM_CURRENT_Q:
        *data = g_foc.core->foc_val->iq_fb;
        break;
    case STREAM_CURRENT_D:
        *data = g_foc.core->foc_val->id_fb;
        break;
    case STREAM_CURRENT_Q_REF:
        *data = g_foc.core->foc_val->iq_ref;
        break;
    case STREAM_CURRENT_D_REF:
        *data = g_foc.core->foc_val->id_ref;
        break;
    case STREAM_SPEED:
        *data = g_foc.core->foc_val->rpm_fb;
        break;
    case STREAM_SPEED_REF:
        *data = g_foc.core->foc_val->rpm_ref;
        break;
    case STREAM_THETA_ELEC:
        *data = g_foc.core->foc_val->theta_elec;
        break;
    case STREAM_THETA_MECH:
        *data = g_foc.core->foc_val->theta_mech;
        break;
    case STREAM_POSITION:
        *data = g_foc.core->foc_val->pos_fb;
        break;
    case STREAM_POSITION_REF:
        *data = g_foc.core->foc_val->pos_ref;
        break;
    default:
        break;
    }
}

void fStreamDataPrepare(DataStreamId stream, u8 index, u8 *data, bool _tx)
{
    fStreamDataGet(stream, (float *)&txdata_queue[index * 4]);
    if (_tx)
    {
        memcpy(data, txdata_queue, (index + 1) * 4);
    }
}
