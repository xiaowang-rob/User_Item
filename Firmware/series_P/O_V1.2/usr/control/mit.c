
#include "mit.h"
#include "math_fast.h"

tMIT_HandleTypeDef mit;

void mit_init(float Kp, float Kd, float tau_ff, float tau_max)
{
    mit.Kp = Kp;
    mit.Kd = Kd;
    mit.tau_ff = tau_ff;
    mit.tau_max = tau_max;
}
// MIT控制律计算
float mit_loop_update(float pos_ref, float pos_fb, float vel_ref, float vel_fb)
{

    // 误差计算
    float pos_err = pos_ref - pos_fb;
    float vel_err = vel_ref - vel_fb;

    // 核心公式：τ = Kp·e_p + Kd·e_v + τ_ff
    float tau = mit.Kp * pos_err + mit.Kd * vel_err + mit.tau_ff;

    //  输出限幅
    return CLAMP(tau, -mit.tau_max, mit.tau_max);
}

// 配置静态参数函数，用于更新控制参数
void mit_config_static(float Kp, float Kd)
{
    mit.Kp = Kp;
    mit.Kd = Kd;
}
// 直接配置前馈扭矩
void mit_config_tff(float tau_ff)
{
    mit.tau_ff = tau_ff;
}
// 配置动态轨迹参数函数，用于更新控制参数
void mit_config_dynamic(float alpha_ref, float vel_ref)
{
    // 扭矩前馈可以由 J B 加期望轨迹计算
    // τ_ff = J·α_des + B·vel_des + τ_gravity/other
    mit.tau_ff = mit.J * alpha_ref + mit.B * vel_ref;
}