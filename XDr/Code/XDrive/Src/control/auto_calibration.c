#include "auto_calibration.h"
#include "smo.h"
#include "system_parameters.h"
#include "foc_core.h"
#include "math_fast.h"
#include "encoder.h"
Motor_t g_Motor;
param_tuning_t g_param_tuning;

void auto_calibration_init(float initial_Rs, float initial_Ls,
                           float initial_Psi_f, float initial_pole_pairs, TUNE_MODE_E tune_mode)

{
    param_tuning_init(&g_param_tuning,
                      initial_Rs, initial_Ls,
                      initial_Psi_f, initial_pole_pairs,
                      Tcon);
    g_param_tuning.tune_mode = tune_mode;
    if (tune_mode == ENCODER_TUNE)
        g_param_tuning.tune_state = PARAM_TUNE_POLE_PAIRS;
    else
        g_param_tuning.tune_state = PARAM_TUNE_RS;
    g_param_tuning.tune_samples = 0;
}
u32 time = 0;
bool auto_calibration_update()
{
    if (g_param_tuning.tune_mode == ENCODER_TUNE)
    { // 有感整定
        g_monitor.theta_mech = GET_ENCODER_ANGLE_RAD();
        g_monitor.omega_fb = (g_monitor.theta_mech - g_monitor.theta_mech_last) / g_param_tuning.dt;
        g_monitor.theta_mech_last = g_monitor.theta_mech;
        encoder_param_tuning_update(&g_param_tuning, g_monitor.Ualpha, g_monitor.Ubeta, g_monitor.Ialpha, g_monitor.Ibeta, g_monitor.theta_mech, g_monitor.omega_fb * g_param_tuning.pole_pairs);

        switch (g_param_tuning.tune_state)
        {
        case PARAM_TUNE_POLE_PAIRS:
            g_monitor.theta_elec += OMEGA_TUNE_POLE_PAIRS * g_param_tuning.dt;
            if (g_monitor.theta_elec >= M2_PI)
                g_monitor.theta_elec -= M2_PI;
            g_monitor.ud = 0;
            g_monitor.uq = 3.0f;
            inv_park_transform(g_monitor.ud, g_monitor.uq, g_monitor.theta_elec, &g_monitor.Ualpha, &g_monitor.Ubeta);
            break;
        case PARAM_TUNE_RS:
            g_monitor.Ualpha = 0.5f;
            g_monitor.Ubeta = 0.5f;
            break;
        case PARAM_TUNE_LS:
            g_monitor.Ualpha = Umax_TUNE_LS * arm_sin_f32(M2_PI / 8 * time);
            time++;
            g_monitor.Ubeta = 0;
            break;
        case PARAM_TUNE_FLUX:
            time = 0;
            g_foccore.run_mode = ENCODER_CONTROL;
            g_foccore.loop_mode = SPEED_LOOP_CONTROL;
            g_foccore.omega_con = 52 / g_param_tuning.pole_pairs; // 500电角速度
            break;
        case PARAM_TUNE_INERTIA:
            g_foccore.run_mode = ENCODER_CONTROL;
            g_foccore.loop_mode = CURRENT_LOOP_CONTROL;

            time++;
            if (time > 1000)
                g_foccore.iq_ref = 3.0f;
            else
                g_foccore.iq_ref = 1.0f;
            break;
        case PARAM_TUNE_FRICTION:
            time = 0;
            g_foccore.run_mode = ENCODER_CONTROL;
            g_foccore.loop_mode = SPEED_LOOP_CONTROL;
            g_foccore.omega_con = 0.2;
            break;
        case PARAM_TUNE_COMPLETE:
            g_foccore.run_mode = AUTO_TUNE_CONTROL;
            g_foccore.loop_mode = IDLE;
            g_monitor.Ualpha = 0;
            g_monitor.Ubeta = 0;
            break;
        default:
            break;
        }
    }
    else
    { // 无感整定

        sensorless_param_tuning_update(&g_param_tuning, g_monitor.Ualpha, g_monitor.Ubeta, g_monitor.Ialpha, g_monitor.Ibeta, g_monitor.omega_fb);
        switch (g_param_tuning.tune_state)
        {
        case PARAM_TUNE_RS:
            g_monitor.Ualpha = 0.5f;
            g_monitor.Ubeta = 0.5f;
            break;
        case PARAM_TUNE_LS:
            g_monitor.Ualpha = Umax_TUNE_LS * arm_sin_f32(M2_PI / 8 * time);
            time++;
            g_monitor.Ubeta = 0;
            break;
        case PARAM_TUNE_FLUX:
            time = 0;
            g_foccore.run_mode = SENSORLESS_CONTROL;
            g_foccore.loop_mode = SPEED_LOOP_CONTROL;
            g_foccore.omega_con = 52 / g_param_tuning.pole_pairs; // 500电角速度
            break;
        case PARAM_TUNE_COMPLETE:
            g_foccore.run_mode = AUTO_TUNE_CONTROL;
            g_foccore.loop_mode = IDLE;
            g_monitor.Ualpha = 0;
            g_monitor.Ubeta = 0;
            break;
        default:
            break;
        }
    }
    if (g_param_tuning.tune_state == PARAM_TUNE_COMPLETE)
    {
        g_Motor.Rs = g_param_tuning.Rs;
        g_Motor.Ls = g_param_tuning.Ls;
        g_Motor.Psi_f = g_param_tuning.Psi_f;
        g_Motor.pole_pairs = g_param_tuning.pole_pairs;
        g_Motor.J = g_param_tuning.J;
        g_Motor.B = g_param_tuning.B;
        g_Motor.torque_constant = g_param_tuning.torque_constant;
        return true;
    }
    return false;
}
bool get_motor_fault_flag(void)
{
    return g_param_tuning.fault_flag;
}