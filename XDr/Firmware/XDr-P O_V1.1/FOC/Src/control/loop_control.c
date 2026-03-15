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
void PI_init(tPI *pi, float kp, float ki, float output_limit, float dt)
{
    memset(pi, 0, sizeof(tPI));
    pi->kp = kp;
    pi->dt = dt;
    pi->ki = ki * dt;
    pi->integral_limit = output_limit / pi->ki * 0.9f;

    pi->output_limit = output_limit;
}

// PI更新（带抗积分饱和）
float PI_update(tPI *pi, float ref, float fb)
{
    float error = ref - fb;

    pi->integral += error;
    pi->integral = CLAMP(pi->integral, -pi->integral_limit, pi->integral_limit);

    // 3. 计算最终输出
    pi->output = pi->kp * error + pi->ki * pi->integral;

    pi->output = CLAMP(pi->output, -pi->output_limit, pi->output_limit);
    return pi->output;
}

// 重置PI状态
void PI_reset(tPI *pi)
{
    pi->integral = 0.0f;
    pi->output = 0.0f;
}

// PID初始化
void PID_init(tPID *pid, float kp, float ki_cont, float kd_cont,
              float output_limit, float dt)
{
    memset(pid, 0, sizeof(tPID));

    pid->kp = kp;

    // 离散化转换
    pid->dt = dt;
    pid->ki = ki_cont * pid->dt; // Ki_disc = Ki_cont × Ts
    pid->kd = kd_cont / pid->dt; // Kd_disc = Kd_cont ÷ Ts

    pid->output_limit = output_limit;
    // 积分限幅：与输出量纲对齐（output = ki * integral）
    pid->integral_limit = (pid->ki > 1e-6f) ? (output_limit / pid->ki) : output_limit;
    pid->derivative_limit = output_limit * 3.0f; // 微分限幅可略大

    pid->alpha = 0.15f;
}

// PID更新（含微分滤波）
float PID_update(tPID *pid, float ref, float fb)
{
    float error = ref - fb;

    //  2. 积分项（积分钳位抗饱和）
    pid->integral += error * pid->dt;
    pid->integral = CLAMP(pid->integral, -pid->integral_limit, pid->integral_limit);

    // 🔹 3. 微分项（对反馈微分 + 强滤波，避免设定值突变冲击）
    float derivative_raw = (fb - pid->last_error) / pid->dt; // 对 fb 微分
    pid->derivative = pid->alpha * derivative_raw + (1.0f - pid->alpha) * pid->derivative;

    pid->derivative = CLAMP(pid->derivative, -pid->derivative_limit, pid->derivative_limit);

    // 4. 合成输出 + 硬限幅
    pid->output = pid->kp * error + pid->kp * pid->integral + pid->kd * pid->derivative;

    pid->output = CLAMP(pid->output, -pid->output_limit, pid->output_limit);

    // 🔹 5. 更新状态（存反馈值，下次对 fb 微分）
    pid->last_error = fb;
    return pid->output;
}

// 重置PID状态
void PID_reset(tPID *pid)
{
    pid->integral = 0.0f;
    pid->derivative = 0.0f;
    pid->last_error = 0.0f;
    pid->derivative = 0.0f;
    pid->output = 0.0f;
}

// 环路控制器整体初始化
void fLoopControlInit(tParameter param, float Vmax)
{
    // 先初始化分频器
    fFrequencyDivisionInit(param.freq_current_loop, param.freq_speed_loop, param.freq_position_loop);
    loop_con.max_Vs = Vmax;
    PI_init(&loop_con.PI_iq, param.kp_current, param.ki_current, Vmax, loop_con.fd.Tcur);
    PI_init(&loop_con.PI_id, param.kp_current, param.ki_current, Vmax, loop_con.fd.Tcur);
    PI_init(&loop_con.PI_speed, param.kp_speed, param.ki_speed, param.limit_current, loop_con.fd.Tspd);
    PI_init(&loop_con.PI_weakmag, param.kp_weakmag, param.ki_weakmag, param.limit_current, loop_con.fd.Tspd);
    PID_init(&loop_con.PID_pos, param.kp_position, param.ki_position, param.kd_position, param.limit_omega, loop_con.fd.Tpos);
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
    return PI_update(&loop_con.PI_iq, current_ref, current_fb);
}

// d轴磁链环
float fMagLoopUpdate(float id_ref, float id_fb)
{
    return PI_update(&loop_con.PI_id, id_ref, id_fb);
}

// 弱磁控制：电压超限时通过负Id削弱磁链
float fWeakMagLoopUpdate(float ud, float uq)
{
    float vout;
    arm_sqrt_f32((ud * ud + uq * uq), &vout);
    float error = loop_con.max_Vs - vout;
    if (error > 0)
        return 0; // 未超限，无需弱磁
    return PI_update(&loop_con.PI_weakmag, loop_con.max_Vs, vout);
}

// 速度环
float fSpeedLoopUpdate(float speed_ref, float speed_fb)
{
    return PI_update(&loop_con.PI_speed, speed_ref, speed_fb);
}

// 相对位置环（带指令限幅）
float fPositionRelLoopUpdate(float position_ref, float position_fb)
{
    if (position_ref > loop_con.position_max)
        position_ref = loop_con.position_max;
    if (position_ref < loop_con.position_min)
        position_ref = loop_con.position_min;
    return PID_update(&loop_con.PID_pos, position_ref, position_fb);
}
