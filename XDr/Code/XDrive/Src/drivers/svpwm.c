#include "svpwm.h"
#include "tim.h"
#include "math_fast.h"
#include "string.h"
#include "system_parameters.h"
SVPWM_t svpwm_g = {0};
void svpwm_Init(float Vbus)
{
    memset(&svpwm_g, 0, sizeof(SVPWM_t));
    svpwm_g.k = sqrt3 * ticpwm / Vbus;
}
void pwm_out(u8 channel, u16 compare)
{
    switch (channel)
    {
    case 1:
        __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, compare);
        break;
    case 2:
        __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, compare);
        break;
    case 3:
        __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_3, compare);
        break;
    default:
        break;
    }
}
void svpwm(float ualpha, float ubeta)
{
    float U1 = ubeta;
    float U2 = sqrt3 * ualpha - ubeta;
    float U3 = -U2 - 2 * ubeta;
    u8 A = U1 > 0;
    u8 B = U2 > 0;
    u8 C = U3 > 0;
    u8 N = 4 * C + 2 * B + A;
    float Tx, Ty, Tzero;
    switch (N)
    {
    case 1:
        svpwm_g.sector = 2;
        Tx = -2 * svpwm_g.k * U1;
        Ty = -svpwm_g.k * U3;
        break;
    case 2:
        svpwm_g.sector = 6;
        Tx = -svpwm_g.k * U3;
        Ty = -2 * svpwm_g.k * U1;
        break;
    case 3:
        svpwm_g.sector = 1;
        Tx = svpwm_g.k * U2;
        Ty = 2 * svpwm_g.k * U1;
        break;
    case 4:
        svpwm_g.sector = 4;
        Tx = -2 * svpwm_g.k * U1;
        Ty = -svpwm_g.k * U2;
        break;
    case 5:
        svpwm_g.sector = 3;
        Tx = 2 * svpwm_g.k * U1;
        Ty = svpwm_g.k * U3;
        break;
    case 6:
        svpwm_g.sector = 5;
        Tx = svpwm_g.k * U3;
        Ty = svpwm_g.k * U2;
        break;
    default:
        break;
    }
    if (Tx + Ty > ticpwm)
    {
        Tx = Tx * ticpwm / (Tx + Ty);
        Ty = Ty * ticpwm / (Tx + Ty);
        Tzero = 0;
    }
    else
    {
        Tzero = ticpwm - Tx - Ty;
    }
    // 以七段式开关序列方式输出--更小的电流纹波和中心对称性（5段 可以减小开关次数）
    float t0 = Tzero / 2; // V0作用起点零向量（0，0，0）
    float t1 = t0 + Tx;   // V1作用
    float t2 = t1 + Ty;   // V2作用
    float t3 = t2 + t0;   // V7作用中间零向量（1，1，1）

    float tu1 = 0, tu2 = ticpwm; // 默认全低
    float tv1 = 0, tv2 = ticpwm;
    float tw1 = 0, tw2 = ticpwm;

    switch (svpwm_g.sector)
    {
    case 1: // V1(100), V2(110)
        tu1 = t0;
        tu2 = t3;
        tv1 = t1;
        tv2 = t2;
        tw1 = 0;
        tw2 = ticpwm;
        break;
    case 2: // V2(110), V3(010)
        tu1 = t1;
        tu2 = t2;
        tv1 = t0;
        tv2 = t3;
        tw1 = 0;
        tw2 = ticpwm;
        break;
    case 3: // V3(010), V4(011)
        tu1 = 0;
        tu2 = ticpwm;
        tv1 = t0;
        tv2 = t3;
        tw1 = t1;
        tw2 = t2;
        break;
    case 4: // V4(011), V5(001)
        tu1 = 0;
        tu2 = ticpwm;
        tv1 = t1;
        tv2 = t2;
        tw1 = t0;
        tw2 = t3;
        break;
    case 5: // V5(001), V6(101)
        tu1 = t1;
        tu2 = t2;
        tv1 = 0;
        tv2 = ticpwm;
        tw1 = t0;
        tw2 = t3;
        break;
    case 6: // V6(101), V1(100)
        tu1 = t0;
        tu2 = t3;
        tv1 = 0;
        tv2 = ticpwm;
        tw1 = t1;
        tw2 = t2;
        break;
    default:
        break;
    }

    // 计算中心对齐 PWM 的比较值（CCR = (上升沿 + 下降沿) / 2）
    svpwm_g.ticu = (u16)((tu1 + tu2) / 2.0f);
    svpwm_g.ticv = (u16)((tv1 + tv2) / 2.0f);
    svpwm_g.ticw = (u16)((tw1 + tw2) / 2.0f);
    // 更新比较值
    pwm_out(1, svpwm_g.ticu);
    pwm_out(2, svpwm_g.ticv);
    pwm_out(3, svpwm_g.ticw);
}
void svpwm_SetVbus(float Vbus)
{
    svpwm_g.k = sqrt3 * ticpwm / Vbus;
}