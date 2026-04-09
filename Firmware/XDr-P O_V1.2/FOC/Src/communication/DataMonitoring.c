#include "DataMonitoring.h"
#include "foc_statemachine.h"
#include "protection_manager.h"
#include "svpwm.h"
#include "system_statemachine.h"
#include "string.h"

static u8 _sta[4];
static float temp_val = 0;

static u8 txdata_queue[64];

void fStreamDataGet(Data_stream_e stream, float *data)
{

    switch (stream)
    {
    case STATUS:
        _sta[0] = fSystemStateGet();
        _sta[1] = g_foc.state;
        _sta[2] = g_pro_manager.fault;
        _sta[3] = g_pro_manager.warning;
        memcpy(data, _sta, 4);
        break;
    case TEMPERATURE:
        *data = g_pro_manager.temperature;
        break;
    case VBUS:
        *data = g_foc.core->motor->Udc;
        break;
    case VOLTAGE_U:
//        temp_val = fGetVoltage_u();
		temp_val =g_foc.core->foc_val->Iu;
        memcpy(data, &temp_val, 4);
        break;
    case VOLTAGE_V:
//        temp_val = fGetVoltage_v();
		temp_val =g_foc.core->foc_val->Iv;
        memcpy(data, &temp_val, 4);
        break;
    case VOLTAGE_W:
//        temp_val = fGetVoltage_w();
		temp_val =g_foc.core->foc_val->Iw;
        memcpy(data, &temp_val, 4);
        break;
    case VOLTAGE_q:
        *data = g_foc.core->foc_val->uq;
        break;
    case VOLTAGE_d:
        *data = g_foc.core->foc_val->ud;
        break;
    case CURRENT_alpha:
        *data = g_foc.core->foc_val->Ialpha;
		//*data = g_foc.core->foc_val->Ualpha_hfi;
		//*data = g_foc.core->foc_val->Ualpha;
//		temp_val =g_foc.core->foc_val->Iu;
//		*data =temp_val;
        break;
    case CURRENT_beta:
        *data = g_foc.core->foc_val->Ibeta;
		//*data = g_foc.core->foc_val->Ubeta_hfi;
		//*data = g_foc.core->foc_val->Ubeta;
//		temp_val = fGetVoltage_u();
//		*data =temp_val;
        break;
    case CURRENT_q:
        *data = g_foc.core->foc_val->iq_fb;
        break;
    case CURRENT_d:
        *data = g_foc.core->foc_val->id_fb;
        break;
    case CURRENT_q_ref:
        *data = g_foc.core->foc_val->iq_ref;
        break;
    case CURRENT_d_ref:
        *data = g_foc.core->foc_val->id_ref;
        break;
    case SPEED:
        *data = g_foc.core->foc_val->rpm_fb;
        break;
    case SPEED_ref:
        *data = g_foc.core->foc_val->rpm_ref;
        break;
    case THETA_elec:
        *data = g_foc.core->foc_val->theta_elec;
        break;
    case THETA_mech:
        *data = g_foc.core->foc_val->theta_mech;
        break;
    case POSITION:
        *data = g_foc.core->foc_val->pos_fb;
        break;
    case POSITION_ref:
        *data = g_foc.core->foc_val->pos_ref;
        break;
    default:
        break;
    }
}

void fStreamDataPrepare(Data_stream_e stream, u8 index, u8 *data, bool _tx)
{
    fStreamDataGet(stream, (float *)&txdata_queue[index * 4]);
    if (_tx)
    {
        memcpy(data, txdata_queue, (index + 1) * 4);
    }
}
