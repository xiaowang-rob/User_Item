#include "loop_control.h"
#include "string.h"
#include "drive_parameters.h"
#include "math_fast.h"

tLoopControl loop_con = {0};

// 初始化分频系数并计算各环周期
void fFrequencyDivisionInit(u8 fd_cur, u8 fd_speed, u8 fd_pos)
{
    memset(&loop_con.fd, 0, sizeof(tFrequencyDivision));
    loop_con.fd.current_update_steps = fd_cur;
    loop_con.fd.speed_update_steps = fd_speed;
    loop_con.fd.position_update_steps = fd_pos;
    loop_con.fd.Tcur = Tpwm * loop_con.fd.current_update_steps;
    loop_con.fd.Tspd = loop_con.fd.Tcur * loop_con.fd.speed_update_steps;
    loop_con.fd.Tpos = loop_con.fd.Tspd * loop_con.fd.position_update_steps;
}

// 每PWM周期调用：更新分频计数器，置位各环更新标志
void fFrequencyDivisionUpdate(void)
{
    loop_con.fd.current_update = false;
    loop_con.fd.speed_update = false;
    loop_con.fd.position_update = false;
    loop_con.fd.tic++;

    if (loop_con.fd.tic >= loop_con.fd.current_update_steps)
    {
        loop_con.fd.tic = 0;
        loop_con.fd.current_update = true;
        loop_con.fd.current_steps++;

        if (loop_con.fd.current_steps >= loop_con.fd.speed_update_steps)
        {
            loop_con.fd.current_steps = 0;
            loop_con.fd.speed_update = true;
            loop_con.fd.speed_steps++;

            if (loop_con.fd.speed_steps >= loop_con.fd.position_update_steps)
            {
                loop_con.fd.speed_steps = 0;
                loop_con.fd.position_update = true;
            }
        }
    }
}

// PI初始化
void PI_init(tPI *pi, float kp, float ki, float output_limit)
{
    memset(pi, 0, sizeof(tPI));
    pi->kp = kp;
    pi->ki = ki;
    pi->integral_limit = output_limit;
    pi->output_limit = output_limit;
}

// PI更新（带抗积分饱和）
float PI_update(tPI *pi, float ref, float fb, float dt)
{
    float error = ref - fb;

    if (pi->enable_integral)
    {
        pi->integral += error * dt;
        // 积分限幅
        if (pi->integral > pi->integral_limit)
            pi->integral = pi->integral_limit;
        else if (pi->integral < -pi->integral_limit)
            pi->integral = -pi->integral_limit;
    }

    pi->output = pi->kp * error + pi->ki * pi->integral;
    pi->enable_integral = true;

    // 输出限幅 + 遇限削弱积分
    if (pi->output > pi->output_limit)
    {
        pi->output = pi->output_limit;
        pi->enable_integral = false;
    }
    else if (pi->output < -pi->output_limit)
    {
        pi->output = -pi->output_limit;
        pi->enable_integral = false;
    }
    return pi->output;
}

// 重置PI状态
void PI_reset(tPI *pi)
{
    pi->integral = 0.0f;
    pi->output = 0.0f;
    pi->enable_integral = false;
}

// PID初始化
void PID_init(tPID *pid, float kp, float ki, float kd, float output_limit)
{
    memset(pid, 0, sizeof(tPID));
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->integral_limit = output_limit;
    pid->output_limit = output_limit;
    pid->derivative_filter = 0.2f; // 微分滤波系数（0.1~0.3）
    pid->derivative_limit = output_limit * 10.0f;
}

// PID更新（含微分滤波）
float PID_update(tPID *pid, float ref, float fb, float dt)
{
    float error = ref - fb;

    // 积分项
    if (pid->enable_integral)
    {
        pid->integral += error * dt;
        if (pid->integral > pid->integral_limit)
            pid->integral = pid->integral_limit;
        else if (pid->integral < -pid->integral_limit)
            pid->integral = -pid->integral_limit;
    }

    // 微分项（带一阶滤波）
    float derivative = (error - pid->last_error) / dt;
    pid->derivative = pid->derivative_filter * derivative +
                      (1.0f - pid->derivative_filter) * pid->last_derivative;

    // 微分限幅
    if (pid->derivative > pid->derivative_limit)
        pid->derivative = pid->derivative_limit;
    else if (pid->derivative < -pid->derivative_limit)
        pid->derivative = -pid->derivative_limit;

    pid->last_derivative = pid->derivative;
    pid->output = pid->kp * error + pid->ki * pid->integral + pid->kd * pid->derivative;
    pid->enable_integral = true;

    // 输出限幅 + 遇限削弱积分
    if (pid->output > pid->output_limit)
    {
        pid->output = pid->output_limit;
        pid->enable_integral = false;
    }
    else if (pid->output < -pid->output_limit)
    {
        pid->output = -pid->output_limit;
        pid->enable_integral = false;
    }

    pid->last_error = error;
    return pid->output;
}

// 重置PID状态
void PID_reset(tPID *pid)
{
    pid->integral = 0.0f;
    pid->derivative = 0.0f;
    pid->last_error = 0.0f;
    pid->last_derivative = 0.0f;
    pid->output = 0.0f;
    pid->enable_integral = false;
}

// 环路控制器整体初始化
void fLoopControlInit(tParameter param, float Vmax)
{
    fFrequencyDivisionInit(param.freq_current_loop, param.freq_speed_loop, param.freq_position_loop);
    loop_con.max_Vs = Vmax;
    PI_init(&loop_con.PI_iq, param.kp_current, param.ki_current, Vmax);
    PI_init(&loop_con.PI_id, param.kp_current, param.ki_current, Vmax);
    PI_init(&loop_con.PI_speed, param.kp_speed, param.ki_speed, param.limit_current);
    PI_init(&loop_con.PI_weakmag, param.kp_weakmag, param.ki_weakmag, param.limit_current);
    PID_init(&loop_con.PID_pos, param.kp_position, param.ki_position, param.kd_position, param.limit_omega);
    loop_con.position_min = param.limit_position_min;
    loop_con.position_max = param.limit_position_max;
}

// 重置所有控制器状态
void fLoopReset(void)
{
    PI_reset(&loop_con.PI_id);
    PI_reset(&loop_con.PI_iq);
    PI_reset(&loop_con.PI_speed);
    PI_reset(&loop_con.PI_weakmag);
    PID_reset(&loop_con.PID_pos);
}

// q轴电流环
float fCurrentLoopUpdate(float current_ref, float current_fb)
{
    return PI_update(&loop_con.PI_iq, current_ref, current_fb, loop_con.fd.Tcur);
}

// d轴磁链环
float fMagLoopUpdate(float id_ref, float id_fb)
{
    return PI_update(&loop_con.PI_id, id_ref, id_fb, loop_con.fd.Tcur);
}

// 弱磁控制：电压超限时通过负Id削弱磁链
float fWeakMagLoopUpdate(float ud, float uq)
{
    float vout;
    arm_sqrt_f32((ud * ud + uq * uq), &vout);
    float error = loop_con.max_Vs - vout;
    if (error > 0)
        return 0; // 未超限，无需弱磁
    return PI_update(&loop_con.PI_weakmag, loop_con.max_Vs, vout, loop_con.fd.Tspd);
}

// 速度环
float fSpeedLoopUpdate(float omega_ref, float omega_fb)
{
    return PI_update(&loop_con.PI_speed, omega_ref, omega_fb, loop_con.fd.Tspd);
}

// 绝对位置环（无指令限幅）
float fPositionAbsLoopUpdate(float position_ref, float position_fb)
{
    return PID_update(&loop_con.PID_pos, position_ref, position_fb, loop_con.fd.Tpos);
}

// 相对位置环（带指令限幅）
float fPositionRelLoopUpdate(float position_ref, float position_fb)
{
    if (position_ref > loop_con.position_max)
        position_ref = loop_con.position_max;
    if (position_ref < loop_con.position_min)
        position_ref = loop_con.position_min;
    return PID_update(&loop_con.PID_pos, position_ref, position_fb, loop_con.fd.Tpos);
}

// PVT模式：动态设置位置环输出限幅（即最大速度）
void fPVT_SetOmega(float omega)
{
    loop_con.PID_pos.output_limit = omega;
}