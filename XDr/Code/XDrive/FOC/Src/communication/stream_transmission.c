#include "stream_transmission.h"
#include "adaptive_control.h"
#include "foc_core.h"
#include "math_fast.h"
#include "encoder.h"
#include "auto_calibration.h"
#include "loop_control.h"
#include "system_parameters.h"
#include "protection_manager.h"
#include "flashDr.h"
#include "canDr.h"
#include "svpwm.h"
#include "system_statemachine.h"
#include "string.h"
u8 _sta[4];
void stream_data_get(Data_stream_e stream, float *data)
{
    switch (stream)
    {
    case STATUS:
        _sta[0] = SystemState_get();
        _sta[1] = g_foccore.state;
        _sta[2] = g_protection_manager.fault;
        _sta[3] = g_protection_manager.warning;
        memcpy(data, _sta, 4);
        break;
    case TEMPERATURE:
        *data = g_adaptive_con.tempareture;
        break;
    case VBUS:
        *data = g_adaptive_con.Udc;
        break;
    case VOLTAGE_U:
        *data = g_svpwm.ticu * g_adaptive_con.Udc / ticpwm;
        break;
    case VOLTAGE_V:
        *data = g_monitor.Ibeta * g_adaptive_con.Udc / ticpwm;
        break;
    case VOLTAGE_W:
        *data = g_monitor.Ualpha * g_adaptive_con.Udc / ticpwm;
        break;
    case VOLTAGE_q:
        *data = g_monitor.uq;
        break;
    case VOLTAGE_d:
        *data = g_monitor.ud;
        break;
    case CURRENT_U:
        *data = g_monitor.Iu;
        break;
    case CURRENT_V:
        *data = g_monitor.Iv;
        break;
    case CURRENT_W:
        *data = g_monitor.Iw;
        break;
    case CURRENT_q:
        *data = g_monitor.iq_fb;
        break;
    case CURRENT_d:
        *data = g_monitor.id_fb;
        break;
    case CURRENT_q_ref:
        *data = g_foccore.iq_ref;
        break;
    case CURRENT_d_ref:
        *data = g_foccore.id_ref;
        break;
    case SPEED:
        *data = rad_to_rpm(g_monitor.omega_fb);
        break;
    case SPEED_con:
        *data = rad_to_rpm(g_foccore.omega_con);
        break;
    case SPEED_ref:
        *data = rad_to_rpm(g_foccore.omega_ref);
        break;
    case THETA_elec:
        *data = rad_to_deg(g_monitor.theta_elec);
        break;
    case THETA_mech:
        *data = rad_to_deg(g_monitor.theta_mech);
        break;
    case THETA_mech_con:
        *data = rad_to_deg(g_foccore.pos_con);
        break;
    case THETA_mech_ref:
        *data = rad_to_deg(g_foccore.pos_ref);
        break;
    default:
        break;
    }
}

void mode_set(Mode_e mode, u8 *data)
{
    switch (mode)
    {
    case SW_CANQUEUE:
        g_foc_mode.canqueue = *data;
        break;
    case SW_WEAKMAG:
        g_foc_mode.weakmag = *data;
        break;
    case SW_FAN:
        g_foc_mode.fan = *data;
        break;
    case SW_TLC:
        g_foc_mode.tls = *data;
        break;
    case SW_CLS:
        g_foc_mode.cls = *data;
        break;
    case SW_VAGUE_PID:
        g_foc_mode.vague_pid = *data;
        break;
    case SW_PVT:
        g_foc_mode.pvt = *data;
        break;
    case FOC_RUN_MODE:
        g_foc_mode.foc_run_mode = *data;
        break;
    case FOC_LOOP_MODE:
        g_foc_mode.foc_loop_mode = *data;
        break;
    case FOC_AUTOTUNE_MODE:
        g_foc_mode.foc_autotune_mode = *data;
        break;
    default:
        break;
    }
}
void mode_ask(Mode_e mode, u8 *data)
{
    if (data == NULL)
    {
        // 空指针检查，防止程序崩溃
        return;
    }
    switch (mode)
    {
    case SW_CANQUEUE:
        *data = g_foc_mode.canqueue;
        break;
    case SW_WEAKMAG:
        *data = g_foc_mode.weakmag;
        break;
    case SW_FAN:
        *data = g_foc_mode.fan;
        break;
    case SW_TLC:
        *data = g_foc_mode.tls;
        break;
    case SW_CLS:
        *data = g_foc_mode.cls;
        break;
    case SW_VAGUE_PID:
        *data = g_foc_mode.vague_pid;
        break;
    case SW_PVT:
        *data = g_foc_mode.pvt;
        break;
    case FOC_RUN_MODE:
        *data = g_foc_mode.foc_run_mode;
        break;
    case FOC_LOOP_MODE:
        *data = g_foc_mode.foc_loop_mode;
        break;
    case FOC_AUTOTUNE_MODE:
        *data = g_foc_mode.foc_autotune_mode;
        break;
    default:
        break;
    }
}

void STATUS_get(u8 *foc_status, u8 *fault)
{
    *foc_status = g_foccore.state;
    *fault = GET_Protect_fault();
}