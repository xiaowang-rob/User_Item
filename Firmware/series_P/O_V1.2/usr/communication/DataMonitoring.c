#include "DataMonitoring.h"
#include "foc_main.h"
#include "svpwm.h"
#include "string.h"

static float temp_val = 0;

static u8 txdata_queue[64];

void stream_data_get(eData_stream stream, float *data)
{

    switch (stream)
    {
    case CURRENT_U:
        //        temp_val = fGetVoltage_u();
        temp_val = g_foc.core->foc_val->iu;
        memcpy(data, &temp_val, 4);
        break;
    case CURRENT_V:
        //        temp_val = fGetVoltage_v();
        temp_val = g_foc.core->foc_val->iv;
        memcpy(data, &temp_val, 4);
        break;
    case CURRENT_W:
        //        temp_val = fGetVoltage_w();
        temp_val = g_foc.core->foc_val->iw;
        memcpy(data, &temp_val, 4);
        break;
    case VOLTAGE_Q:
        *data = g_foc.core->foc_val->uq;
        break;
    case VOLTAGE_D:
        *data = g_foc.core->foc_val->ud;
        break;
    case CURRENT_ALPHA:
        *data = g_foc.core->foc_val->ialpha;
        //*data = g_foc.core->foc_val->ualpha_hfi;
        //*data = g_foc.core->foc_val->ualpha;
        //		temp_val =g_foc.core->foc_val->iu;
        //		*data =temp_val;
        break;
    case CURRENT_BETA:
        *data = g_foc.core->foc_val->ibeta;
        //*data = g_foc.core->foc_val->ubeta_hfi;
        //*data = g_foc.core->foc_val->ubeta;
        //		temp_val = fGetVoltage_u();
        //		*data =temp_val;
        break;
    case CURRENT_Q:
        *data = g_foc.core->foc_val->iq_fb;
        break;
    case CURRENT_D:
        *data = g_foc.core->foc_val->id_fb;
        break;
    case CURRENT_Q_REF:
        *data = g_foc.core->foc_val->iq_ref;
        break;
    case CURRENT_D_REF:
        *data = g_foc.core->foc_val->id_ref;
        break;
    case SPEED:
        *data = g_foc.core->foc_val->rpm_fb;
        break;
    case SPEED_REF:
        *data = g_foc.core->foc_val->rpm_ref;
        break;
    case THETA_ELEC:
        *data = g_foc.core->foc_val->theta_elec;
        break;
    case THETA_MECH:
        *data = g_foc.core->foc_val->theta_mech;
        break;
    case POSITION:
        *data = g_foc.core->foc_val->pos_fb;
        break;
    case POSITION_REF:
        *data = g_foc.core->foc_val->pos_ref;
        break;
    default:
        break;
    }
}

void stream_data_prepare(eData_stream stream, u8 index, u8 *data, bool _tx)
{
    stream_data_get(stream, (float *)&txdata_queue[index * 4]);
    if (_tx)
    {
        memcpy(data, txdata_queue, (index + 1) * 4);
    }
}
