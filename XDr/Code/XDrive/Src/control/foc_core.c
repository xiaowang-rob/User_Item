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

SVPWM_t g_svpwm = {0};
Monitor_t g_monitor = {0};
foc_core_t g_foccore = {0};
smo_sensorless_t g_smo = {0};
startup_mechine_t startup_machine;
void startup_machine_init(LOOP_CON_t *loopcon, float omega_gradient, float pos_gradient, float align_current, float align_time, float openloop_cur, float openloop_omega, float changeloop_omega)
{
    memset(&startup_machine, 0, sizeof(startup_mechine_t));
    startup_machine.omega_gradient = omega_gradient * loopcon->fd.Tspd;
    startup_machine.pos_gradient = pos_gradient * loopcon->fd.Tpos;
    startup_machine.align_cur = align_current;
    startup_machine.align_steps = (u32)(align_time / Tcon);
    startup_machine.openloop_cur = openloop_cur;
    startup_machine.openloop_omega = openloop_omega;
    startup_machine.changeloop_speed = changeloop_omega;
}
void FOC_con_loop_start()
{
    ADC_DR_Init();
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
void theta_align()
{
    if (startup_machine.current_steps < startup_machine.align_steps)
    {
        startup_machine.current_steps++;
        g_foccore.id_ref = startup_machine.align_cur;
    }
    else
    {
        startup_machine.current_steps = 0;
        g_foccore.id_ref = 0;
        g_monitor.theta_elec = 0;
        startup_machine.align_flag = true;
    }
}
void oloop_to_cloop() // 无感模式 开环切闭环
{
    g_monitor.theta_elec += startup_machine.openloop_omega * Tcon;
    g_foccore.iq_ref = startup_machine.openloop_cur;
    g_monitor.omega_fb = smo_sensorless_get_omega(&g_smo);
    if (g_monitor.omega_fb > startup_machine.changeloop_speed)
        startup_machine.change_flag = true;
}

u8 change_Index = 0;
void smaple_point_change()
{
    switch (g_svpwm.sector)
    {
    case 1:
    case 4:
        if (g_svpwm.ticu > ticDT + ticTN)
        {
            if (change_Index != 1)
                change_Index = 1;
            else
                return;
        }
        else if (alltic_tsdttn > 2 * g_svpwm.ticu)
        {
            if (change_Index != 3)
                change_Index = 3;
            else
                return;
        }
        else
        {
            if (change_Index != 2)
                change_Index = 2;
            else
                return;
        }
        break;
    case 2:
    case 5:
        if (g_svpwm.ticv > ticDT + ticTN)
        {
            if (change_Index != 1)
                change_Index = 1;
            else
                return;
        }
        else if (alltic_tsdttn > 2 * g_svpwm.ticv)
        {
            if (change_Index != 3)
                change_Index = 3;
            else
                return;
        }
        else
        {
            if (change_Index != 2)
                change_Index = 2;
            else
                return;
        }
        break;
    default:
        if (g_svpwm.ticw > ticDT + ticTN)
        {
            if (change_Index != 1)
                change_Index = 1;
            else
                return;
        }
        else if (alltic_tsdttn > 2 * g_svpwm.ticw)
        {
            if (change_Index != 3)
                change_Index = 3;
            else
                return;
        }
        else
        {
            if (change_Index != 2)
                change_Index = 2;
            else
                return;
        }
        break;
    }

    switch (change_Index)
    {
    case 1:
        ADC_sample_change(1);
        break;
    case 2:
        ADC_sample_change(g_svpwm.ticu - tics);
        break;
    case 3:
        ADC_sample_change(g_svpwm.ticu + ticDT + ticTN);
        break;
    default:
        break;
    }
}
bool foc_enable_flag = false;
void foc_core_run()
{
    if (foc_enable_flag == false)
        return;
    Frequency_division_updatta();
    Current_reconstruction();
    switch (g_foccore.run_mode)
    {
    case AUTO_TUNE_CONTROL:
        // todo:暂时还想不到干啥
        break;
    case ENCODER_CONTROL:
        g_monitor.theta_elec = GET_ENCODER_ANGLE_RAD() * g_Motor.pole_pairs;
        park_transform(g_monitor.Ialpha, g_monitor.Ibeta, g_monitor.theta_elec, &g_monitor.id_fb, &g_monitor.iq_fb);
        switch (g_foccore.loop_mode)
        {
        case POSITION_ABS_CONTROL:
            g_monitor.pos_fb = GET_ENCODER_ANGLE_INC();
            if (position_loop_updata())
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
            if (speed_loop_updata())
            {
                if (g_foccore.omega_ref - g_foccore.omega_con > startup_machine.omega_gradient)
                    g_foccore.omega_con += startup_machine.omega_gradient;
                if (g_foccore.omega_ref - g_foccore.omega_con < -startup_machine.omega_gradient)
                    g_foccore.omega_con -= startup_machine.omega_gradient;
                g_foccore.iq_ref = Speed_loop(g_foccore.omega_con, g_monitor.omega_fb);
            }
            break;
        case POSITION_REL_CONTROL:
            g_monitor.pos_fb = GET_ENCODER_ANGLE_INC();
            if (position_loop_updata())
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
            if (speed_loop_updata())
            {
                if (g_foccore.omega_ref - g_foccore.omega_con > startup_machine.omega_gradient)
                    g_foccore.omega_con += startup_machine.omega_gradient;
                if (g_foccore.omega_ref - g_foccore.omega_con < -startup_machine.omega_gradient)
                    g_foccore.omega_con -= startup_machine.omega_gradient;
                g_foccore.iq_ref = Speed_loop(g_foccore.omega_con, g_monitor.omega_fb);
            }
            break;
        case SPEED_LOOP_CONTROL:
            g_monitor.theta_mech = GET_ENCODER_ANGLE_INC();
            g_monitor.omega_fb = (g_monitor.theta_mech - g_monitor.theta_mech_last) / Tcon;
            g_monitor.theta_mech_last = g_monitor.pos_fb;
            if (speed_loop_updata())
            {
                if (g_foccore.omega_ref - g_foccore.omega_con > startup_machine.omega_gradient)
                    g_foccore.omega_con += startup_machine.omega_gradient;
                if (g_foccore.omega_ref - g_foccore.omega_con < -startup_machine.omega_gradient)
                    g_foccore.omega_con -= startup_machine.omega_gradient;
                g_foccore.iq_ref = Speed_loop(g_foccore.omega_con, g_monitor.omega_fb);
            }
            break;
        default:
            break;
        }
        break;
    case SENSORLESS_CONTROL:
        if (!startup_machine.align_flag)
            theta_align();
        else
        {
            smo_sensorless_update(&g_smo, g_monitor.Ualpha, g_monitor.Ubeta, g_monitor.Ialpha, g_monitor.Ibeta);
            if (!startup_machine.change_flag)
                oloop_to_cloop();
            else
                g_monitor.theta_elec = g_smo.theta;
            park_transform(g_monitor.Ialpha, g_monitor.Ibeta, g_monitor.theta_elec, &g_monitor.id_fb, &g_monitor.iq_fb);
            g_monitor.omega_fb = smo_sensorless_get_omega(&g_smo);
            if (g_foccore.loop_mode == SPEED_LOOP_CONTROL)
            {
                if (speed_loop_updata())
                {
                    if (g_foccore.omega_ref - g_foccore.omega_con > startup_machine.omega_gradient)
                        g_foccore.omega_con += startup_machine.omega_gradient;
                    if (g_foccore.omega_ref - g_foccore.omega_con < -startup_machine.omega_gradient)
                        g_foccore.omega_con -= startup_machine.omega_gradient;
                    g_foccore.iq_ref = Speed_loop(g_foccore.omega_con, g_monitor.omega_fb);
                }
            }
        }
        break;
    default:
        break;
    }

    if (g_foccore.run_mode != AUTO_TUNE_CONTROL)
    {
        g_monitor.ud = Magnetic_loop(g_foccore.id_ref, g_monitor.id_fb);
        g_monitor.uq = Current_loop(g_foccore.iq_ref, g_monitor.iq_fb);
        inv_park_transform(g_monitor.ud, g_monitor.uq, g_monitor.theta_elec, &g_monitor.Ualpha, &g_monitor.Ubeta);
    }
    svpwm(g_monitor.Ualpha, g_monitor.Ubeta, g_svpwm);
    smaple_point_change();
}
void foc_disable()
{
    if (foc_enable_flag)
    {
        foc_enable_flag = false;
        DISABLE_PWM();
        Frequency_division_reset();
        loop_reset();
        memset(&g_monitor, 0, sizeof(Monitor_t));
    }
}
void foc_enable()
{
    if (!foc_enable_flag)
    {
        foc_enable_flag = true;
        ENABLE_PWM();
    }
}
bool foc_shutdown()
{
    if (fast_absf(g_monitor.omega_fb) > 0.1)
    {
        g_foccore.loop_mode = SPEED_LOOP_CONTROL;
        g_foccore.omega_con = -g_monitor.omega_fb;
        return false;
    }
    return true;
}
