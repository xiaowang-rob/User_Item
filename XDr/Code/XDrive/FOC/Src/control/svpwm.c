#include "svpwm.h"
#include "tim.h"
#include "math_fast.h"
#include "string.h"
#include "system_parameters.h"
#include "device.h"
__IO u16 tim8_ch_compare[3] = {0};

void svpwm_Init(SVPWM_t svpwm, float Vbus)
{
    memset(&svpwm, 0, sizeof(SVPWM_t));
    svpwm.k = sqrt3 * ticpwm / Vbus;
}
void pwm_out(u8 channel, u16 compare)
{
    tim8_ch_compare[channel - 1] = compare;
}

void ENABLE_PWM()
{
    HAL_TIM_PWM_Start_DMA(&htim8, TIM_CHANNEL_1, (u32 *)&tim8_ch_compare[0], 1);
    HAL_TIM_PWM_Start_DMA(&htim8, TIM_CHANNEL_2, (u32 *)&tim8_ch_compare[1], 1);
    HAL_TIM_PWM_Start_DMA(&htim8, TIM_CHANNEL_3, (u32 *)&tim8_ch_compare[2], 1);
    pwm_out(1, 0);
    pwm_out(2, 0);
    pwm_out(3, 0);
}
void DISABLE_PWM()
{
    HAL_TIM_PWM_Stop_DMA(&htim8, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop_DMA(&htim8, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop_DMA(&htim8, TIM_CHANNEL_3);
}
void PWM_POWER_ON()
{
    HAL_GPIO_WritePin(POWER12V_GPIOx, POWER12V_GPIOx_PIN, GPIO_PIN_SET);
}
void PWM_POWER_OFF()
{
    HAL_GPIO_WritePin(POWER12V_GPIOx, POWER12V_GPIOx_PIN, GPIO_PIN_RESET);
}
void svpwm_run(float ualpha, float ubeta, SVPWM_t svpwm)
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
        svpwm.sector = 2;
        Tx = -2 * svpwm.k * U1;
        Ty = -svpwm.k * U3;
        break;
    case 2:
        svpwm.sector = 6;
        Tx = -svpwm.k * U3;
        Ty = -2 * svpwm.k * U1;
        break;
    case 3:
        svpwm.sector = 1;
        Tx = svpwm.k * U2;
        Ty = 2 * svpwm.k * U1;
        break;
    case 4:
        svpwm.sector = 4;
        Tx = -2 * svpwm.k * U1;
        Ty = -svpwm.k * U2;
        break;
    case 5:
        svpwm.sector = 3;
        Tx = 2 * svpwm.k * U1;
        Ty = svpwm.k * U3;
        break;
    case 6:
        svpwm.sector = 5;
        Tx = svpwm.k * U3;
        Ty = svpwm.k * U2;
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

    switch (svpwm.sector)
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
    svpwm.ticu = (u16)((tu1 + tu2) / 2.0f);
    svpwm.ticv = (u16)((tv1 + tv2) / 2.0f);
    svpwm.ticw = (u16)((tw1 + tw2) / 2.0f);
    // 更新比较值
    pwm_out(1, svpwm.ticu);
    pwm_out(2, svpwm.ticv);
    pwm_out(3, svpwm.ticw);
}
void svpwm_SetVbus(SVPWM_t svpwm, float Vbus)
{
    svpwm.k = sqrt3 * ticpwm / Vbus;
}