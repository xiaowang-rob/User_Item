#include "foc_core.h"
#include "adcDr.h"
#include "svpwm.h"
#include "auto_calibration.h"
#include "math_fast.h"
#include "encoder.h"
#include "string.h"
#include "smo.h"
#include "loop_control.h"
#include "system_parameters.h"
#include "tim.h"
SVPWM_t g_svpwm = {0};
Monitor_t g_monitor = {0};
foc_core_t g_foccore = {0};
smo_sensorless_t g_smo = {0};
startup_mechine_t startup_machine;
void startup_machine_init(float pos_gradient, float pos_gradient, float align_current, float align_time, float min_changeloop_time, float changeloop_speed)
{
    memset(&startup_machine, 0, sizeof(startup_mechine_t));
    startup_machine.pos_gradient = pos_gradient * Tcon;
    startup_machine.pos_gradient = pos_gradient * Tcon;
    startup_machine.align_current = align_current;
    startup_machine.align_steps = (u32)(align_time / Tcon);
    startup_machine.min_changeloop_steps = (u32)(min_changeloop_time / Tcon);
    startup_machine.changeloop_speed = changeloop_speed;
}
void foc_core_init()
{
    memset(&g_foccore, 0, sizeof(foc_core_t));
    memset(&g_monitor, 0, sizeof(Monitor_t));
}
void Current_reconstruction()
{
    float ui, vi, wi;
    ADC_GET_Current(&g_monitor.Iu, &g_monitor.Iv, &g_monitor.Iw);
    switch (g_svpwm.sector)
    {
    case 1:
        ui = g_monitor.Iv + g_monitor.Iw;
        vi = -g_monitor.Iv;
        wi = -g_monitor.Iw;
        break;
    case 2:
        ui = -g_monitor.Iu;
        vi = g_monitor.Iu + g_monitor.Iw;
        wi = -g_monitor.Iw;
        break;
    case 3:
        ui = -g_monitor.Iu;
        vi = -g_monitor.Iv;
        wi = g_monitor.Iu + g_monitor.Iv;
        break;
    case 4:
        ui = g_monitor.Iv + g_monitor.Iw;
        vi = -g_monitor.Iv;
        wi = -g_monitor.Iw;
        break;
    case 5:
        ui = -g_monitor.Iu;
        vi = g_monitor.Iu + g_monitor.Iw;
        wi = -g_monitor.Iw;
        break;
    case 6:
        ui = -g_monitor.Iu;
        vi = -g_monitor.Iv;
        wi = g_monitor.Iu + g_monitor.Iv;
    default:
        ui = 0;
        vi = 0;
        wi = 0;
        break;
    }
    clark_transform(ui, vi, wi, &g_monitor.Ialpha, &g_monitor.Ibeta);
}
void foc_core_run()
{
    if (g_foccore.enable == false)
        return;
    else
        Frequency_division_reset(&g_loop_con.fd);

    Frequency_division_updatta(&g_loop_con.fd);
    Current_reconstruction();
    if (g_foccore.run_mode == ENCODER_CONTROL)
    { // 有感模式

        g_monitor.theta_elec = GET_ENCODER_ANGLE_RAD() * g_Motor.pole_pairs;
        park_transform(g_monitor.Ialpha, g_monitor.Ibeta, g_monitor.theta_elec, &g_monitor.id_fb, &g_monitor.iq_fb);
        if (g_foccore.loop_mode == POSITION_ABS_CONTROL)
        {
            g_monitor.pos_fb = GET_ENCODER_ANGLE_INC();
            if (g_loop_con.fd.position_updata)
            {
                // 目标位置归一化
                if (g_foccore.pos_ref > M2_PI)
                    g_foccore.pos_ref -= M2_PI;
                if (g_foccore.pos_ref < -M2_PI)
                    g_foccore.pos_ref += M2_PI;
                // 反馈角度归一化
                if (g_monitor.pos_fb / g_foccore.Reduction_ratio > M2_PI)
                    g_monitor.pos_fb -= M2_PI * g_foccore.Reduction_ratio;
                if (g_monitor.pos_fb / g_foccore.Reduction_ratio < -M2_PI)
                    g_monitor.pos_fb += M2_PI * g_foccore.Reduction_ratio;
                // 启动过程
                if (g_foccore.pos_ref * g_foccore.Reduction_ratio - g_foccore.pos_con > startup_machine.pos_gradient)
                    g_foccore.pos_con += startup_machine.pos_gradient;
                else if (g_foccore.pos_ref * g_foccore.Reduction_ratio - g_foccore.pos_con < -startup_machine.pos_gradient)
                    g_foccore.pos_con -= startup_machine.pos_gradient;
                else
                    g_foccore.pos_con = g_foccore.pos_ref * g_foccore.Reduction_ratio;
                // 进位置环
                g_foccore.omega_ref = Position_abs_loop(g_foccore.pos_con, g_monitor.pos_fb);
            }
            g_monitor.omega_fb = (g_monitor.pos_fb - g_monitor.theta_mech_last) / Tcon;
            g_monitor.theta_mech_last = g_monitor.pos_fb;
            // 进速度环
            if (g_loop_con.fd.speed_updata)
            {
                if (g_foccore.omega_ref - g_foccore.omega_con > startup_machine.omega_gradient)
                    g_foccore.omega_con += startup_machine.omega_gradient;
                if (g_foccore.omega_ref - g_foccore.omega_con < -startup_machine.omega_gradient)
                    g_foccore.omega_con -= startup_machine.omega_gradient;
                g_foccore.iq_ref = Speed_loop(g_foccore.omega_con, g_monitor.omega_fb);
            }
        }
        if (g_foccore.loop_mode == POSITION_REL_CONTROL)
        {
            g_monitor.pos_fb = GET_ENCODER_ANGLE_INC();
            if (g_loop_con.fd.position_updata)
            {
                if (g_foccore.pos_ref * g_foccore.Reduction_ratio - g_foccore.pos_con > startup_machine.pos_gradient)
                    g_foccore.pos_con += startup_machine.pos_gradient;
                else if (g_foccore.pos_ref * g_foccore.Reduction_ratio - g_foccore.pos_con < -startup_machine.pos_gradient)
                    g_foccore.pos_con -= startup_machine.pos_gradient;
                else
                    g_foccore.pos_con = g_foccore.pos_ref * g_foccore.Reduction_ratio;
                g_foccore.omega_ref = Position_rel_loop(g_foccore.pos_con, g_monitor.pos_fb);
            }
            g_monitor.omega_fb = (g_monitor.pos_fb - g_monitor.theta_mech_last) / Tcon;
            g_monitor.theta_mech_last = g_monitor.pos_fb;
            if (g_loop_con.fd.speed_updata)
            {
                if (g_foccore.omega_ref - g_foccore.omega_con > startup_machine.omega_gradient)
                    g_foccore.omega_con += startup_machine.omega_gradient;
                if (g_foccore.omega_ref - g_foccore.omega_con < -startup_machine.omega_gradient)
                    g_foccore.omega_con -= startup_machine.omega_gradient;
                g_foccore.iq_ref = Speed_loop(g_foccore.omega_con, g_monitor.omega_fb);
            }
        }
        if (g_foccore.loop_mode == SPEED_LOOP_CONTROL)
        {
            g_monitor.theta_mech = GET_ENCODER_ANGLE_INC();
            g_monitor.omega_fb = (g_monitor.theta_mech - g_monitor.theta_mech_last) / Tcon;
            g_monitor.theta_mech_last = g_monitor.pos_fb;
            if (g_loop_con.fd.speed_updata)
            {
                if (g_foccore.omega_ref - g_foccore.omega_con > startup_machine.omega_gradient)
                    g_foccore.omega_con += startup_machine.omega_gradient;
                if (g_foccore.omega_ref - g_foccore.omega_con < -startup_machine.omega_gradient)
                    g_foccore.omega_con -= startup_machine.omega_gradient;
                g_foccore.iq_ref = Speed_loop(g_foccore.omega_con, g_monitor.omega_fb);
            }
        }
    }
    else
    { // 无感模式
        smo_sensorless_update(&g_smo, g_monitor.Ualpha, g_monitor.Ubeta, g_monitor.Ialpha, g_monitor.Ibeta);
        g_monitor.theta_elec = g_smo.theta;
        park_transform(g_monitor.Ialpha, g_monitor.Ibeta, g_monitor.theta_elec, &g_monitor.id_fb, &g_monitor.iq_fb);
        if (g_foccore.loop_mode == SPEED_LOOP_CONTROL)
        {
            g_monitor.omega_fb = smo_sensorless_get_omega(&g_smo);
            if (g_loop_con.fd.speed_updata)
                g_foccore.iq_ref = Speed_loop(g_foccore.omega_con, g_monitor.omega_fb);
        }
    }
    if (g_foccore.loop_mode != IDLE)
    {
        g_monitor.ud = Magnetic_loop(g_foccore.id_ref, g_monitor.id_fb);
        g_monitor.uq = Current_loop(g_foccore.iq_ref, g_monitor.iq_fb);
        inv_park_transform(g_monitor.ud, g_monitor.uq, g_monitor.theta_elec, &g_monitor.Ualpha, &g_monitor.Ubeta);
    }
    svpwm(g_monitor.Ualpha, g_monitor.Ubeta, g_svpwm);
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM8)
    {
        foc_core_run();
    }
}