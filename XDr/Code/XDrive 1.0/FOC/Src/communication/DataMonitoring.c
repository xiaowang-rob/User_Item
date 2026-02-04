#include "DataMonitoring.h"
#include "foc_statemachine.h"
#include "math_fast.h"
#include "encoder.h"
#include "loop_control.h"
#include "drive_parameters.h"
#include "protection_manager.h"
#include "flashDr.h"
#include "svpwm.h"
#include "system_statemachine.h"
#include "string.h"
u8 _sta[4];
static float temp_val = 0;
void stream_data_get(Data_stream_e stream, float *data)
{

    switch (stream)
    {
    case STATUS:
        _sta[0] = SystemState_get();
        _sta[1] = g_foc.state;
        _sta[2] = g_pro_manager.fault;
        _sta[3] = g_pro_manager.warning;
        memcpy(data, _sta, 4);
        break;
    case TEMPERATURE:
        *data = g_pro_manager.temperature;
        break;
    case VBUS:
        *data = g_foc.motor->Udc;
        break;
    case VOLTAGE_U:
        temp_val = fGetVoltage_u();
        memcpy(data, &temp_val, 4);
        break;
    case VOLTAGE_V:
        temp_val = fGetVoltage_v();
        memcpy(data, &temp_val, 4);
        break;
    case VOLTAGE_W:
        temp_val = fGetVoltage_w();
        memcpy(data, &temp_val, 4);
        break;
    case VOLTAGE_q:
        *data = g_foc.val->uq;
        break;
    case VOLTAGE_d:
        *data = g_foc.val->ud;
        break;
    case CURRENT_U:
        *data = g_foc.val->Iu;
        break;
    case CURRENT_V:
        *data = g_foc.val->Iv;
        break;
    case CURRENT_W:
        *data = g_foc.val->Iw;
        break;
    case CURRENT_q:
        *data = g_foc.val->iq_fb;
        break;
    case CURRENT_d:
        *data = g_foc.val->id_fb;
        break;
    case CURRENT_q_ref:
        *data = g_foc.val->iq_ref;
        break;
    case CURRENT_d_ref:
        *data = g_foc.val->id_ref;
        break;
    case SPEED:
        temp_val = fRadToRpm(g_foc.val->omega_fb);
        memcpy(data, &temp_val, 4);
        break;
    case SPEED_con:
        temp_val = fRadToRpm(g_foc.val->omega_con);
        memcpy(data, &temp_val, 4);
        break;
    case SPEED_ref:
        temp_val = fRadToRpm(g_foc.val->omega_ref);
        memcpy(data, &temp_val, 4);
        break;
    case THETA_elec:
        temp_val = fRadToDeg(g_foc.val->theta_elec);
        memcpy(data, &temp_val, 4);
        break;
    case THETA_mech:
        temp_val = fRadToDeg(g_foc.val->theta_mech);
        memcpy(data, &temp_val, 4);
        break;
    case POSITION:
        temp_val = fRadToDeg(g_foc.val->pos_fb);
        memcpy(data, &temp_val, 4);
        break;
    case POSITION_con:
        temp_val = fRadToDeg(g_foc.val->pos_con);
        memcpy(data, &temp_val, 4);
        break;
    case POSITION_ref:
        temp_val = fRadToDeg(g_foc.val->pos_ref);
        memcpy(data, &temp_val, 4);
        break;
    default:
        break;
    }
}
