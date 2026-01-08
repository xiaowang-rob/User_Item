#include "DataMonitoring.h"
#include "foc_core.h"
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
    const FOC_val_t *focval = FOC_GET_VAL_adr();
    switch (stream)
    {
    case STATUS:
        _sta[0] = SystemState_get();
        _sta[1] = FOC_Get_state();
        _sta[2] = g_protection_manager.fault;
        _sta[3] = g_protection_manager.warning;
        memcpy(data, _sta, 4);
        break;
    case TEMPERATURE:
        *data = g_protection_manager.temperature;
        break;
    case VBUS:
        ADC_GET_Voltage(data);
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
        *data = focval->uq;
        break;
    case VOLTAGE_d:
        *data = focval->ud;
        break;
    case CURRENT_U:
        *data = focval->Iu;
        break;
    case CURRENT_V:
        *data = focval->Iv;
        break;
    case CURRENT_W:
        *data = focval->Iw;
        break;
    case CURRENT_q:
        *data = focval->iq_fb;
        break;
    case CURRENT_d:
        *data = focval->id_fb;
        break;
    case CURRENT_q_ref:
        *data = focval->iq_ref;
        break;
    case CURRENT_d_ref:
        *data = focval->id_ref;
        break;
    case SPEED:
        *data = rad_to_rpm(focval->omega_fb);
        break;
    case SPEED_con:
        *data = rad_to_rpm(focval->omega_con);
        break;
    case SPEED_ref:
        *data = rad_to_rpm(focval->omega_ref);
        break;
    case THETA_elec:
        *data = rad_to_deg(focval->theta_elec);
        break;
    case THETA_mech:
        *data = rad_to_deg(focval->theta_mech);
        break;
    case THETA_mech_con:
        *data = rad_to_deg(focval->pos_con);
        break;
    case THETA_mech_ref:
        *data = rad_to_deg(focval->pos_ref);
        break;
    default:
        break;
    }
}
