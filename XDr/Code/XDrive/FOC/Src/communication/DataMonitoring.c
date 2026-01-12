#include "DataMonitoring.h"
#include "foc_statemachine.h"
#include "math_fast.h"
#include "encoder.h"
#include "loop_control.h"
#include "system_parameters.h"
#include "protection_manager.h"
#include "flashDr.h"
#include "svpwm.h"
#include "system_statemachine.h"
#include "string.h"
u8 _sta[4];
float U, V, W; // 相电压
void stream_data_get(Data_stream_e stream, float *data)
{
    float temp_val = 0;
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
    case VOLTAGE_V:
    case VOLTAGE_W:
        fGetPhaseVoltage(&U, &V, &W);
        if (stream == VOLTAGE_U)
            *data = U;
        else if (stream == VOLTAGE_V)
            *data = V;
        else if (stream == VOLTAGE_W)
            *data = W;
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
        temp_val = rad_to_rpm(g_foc.val->omega_fb);
        *data = temp_val;
        break;
    case SPEED_con:
        temp_val = rad_to_rpm(g_foc.val->omega_con);
        *data = temp_val;
        break;
    case SPEED_ref:
        temp_val = rad_to_rpm(g_foc.val->omega_ref);
        *data = temp_val;
        break;
    case THETA_elec:
        temp_val = rad_to_deg(g_foc.val->theta_elec);
        *data = temp_val;
        break;
    case THETA_mech:
        temp_val = rad_to_deg(g_foc.val->theta_mech);
        *data = temp_val;
        break;
    case POSITION:
        temp_val = rad_to_deg(g_foc.val->pos_fb);
        *data = temp_val;
        break;
    case POSITION_con:
        temp_val = rad_to_deg(g_foc.val->pos_con);
        *data = temp_val;
        break;
    case POSITION_ref:
        temp_val = rad_to_deg(g_foc.val->pos_ref);
        *data = temp_val;
        break;
    default:
        break;
    }
}
