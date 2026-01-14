#include "svpwm.h"
#include "tim.h"
#include "math_fast.h"
#include "string.h"
#include "system_parameters.h"
#include "device.h"
#include "adcDr.h"

SVPWM_t svpwm = {0};

SVPWM_t *get_svpwm_adr()
{
    return &svpwm;
}

void svpwm_Init(float Vbus)
{
    memset(&svpwm, 0, sizeof(SVPWM_t));

    svpwm.k = sqrt3_2 * (float)ticpwm / Vbus;
}
void pwm_out()
{
    __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_3, svpwm.ticu);
    __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, svpwm.ticv);
    __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, svpwm.ticw);
}

void ENABLE_PWM()
{
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_3);
    HAL_TIMEx_PWMN_Start(&htim8, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&htim8, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Start(&htim8, TIM_CHANNEL_3);
}
void DISABLE_PWM()
{
    HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_3);
    HAL_TIMEx_PWMN_Stop(&htim8, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Stop(&htim8, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Stop(&htim8, TIM_CHANNEL_3);
}
void PWM_POWER_ON()
{
    HAL_GPIO_WritePin(POWER12V_GPIOx, POWER12V_GPIOx_PIN, GPIO_PIN_SET);
    svpwm.power_flag = true;
}
void PWM_POWER_OFF()
{
    HAL_GPIO_WritePin(POWER12V_GPIOx, POWER12V_GPIOx_PIN, GPIO_PIN_RESET);
    svpwm.power_flag = false;
}
void svpwm_run(float ualpha, float ubeta)
{
    float U1 = ubeta;
    float U2 = sqrt3 * ualpha - ubeta;
    float U3 = -U2 - 2 * ubeta;
    u8 A = U1 > 0;
    u8 B = U2 > 0;
    u8 C = U3 > 0;
    u8 vN = 4 * C + 2 * B + A;
    float Tx, Ty, Tzero;
    switch (vN)
    {
    case 0:               // Zero vector (all negative)
    case 7:               // Zero vector (all positive)
        svpwm.sector = 0; // Use special sector 0
        Tx = 0;
        Ty = 0;
        break;
    case 1:
        Tx = -svpwm.k * U3;
        Ty = -2 * svpwm.k * U1;
        svpwm.sector = 2;

        break;
    case 2:
        Tx = svpwm.k * U3;
        Ty = svpwm.k * U2;
        svpwm.sector = 6;

        break;
    case 3:
        Tx = -svpwm.k * U2;
        Ty = -svpwm.k * U3;
        svpwm.sector = 1;

        break;
    case 4:
        svpwm.sector = 4;
        Tx = -2 * svpwm.k * U1;
        Ty = -svpwm.k * U2;
        break;
    case 5:
        Tx = svpwm.k * U2;
        Ty = 2 * svpwm.k * U1;
        svpwm.sector = 3;

        break;
    case 6:
        Tx = 2 * svpwm.k * U1;
        Ty = svpwm.k * U3;
        svpwm.sector = 5;

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
        break;
    case 2: // V2(110), V3(010)
        tu1 = t1;
        tu2 = t2;
        tv1 = t0;
        tv2 = t3;
        break;
    case 3: // V3(010), V4(011)
        tv1 = t0;
        tv2 = t3;
        tw1 = t1;
        tw2 = t2;
        break;
    case 4: // V4(011), V5(001)
        tv1 = t1;
        tv2 = t2;
        tw1 = t0;
        tw2 = t3;
        break;
    case 5: // V5(001), V6(101)
        tu1 = t1;
        tu2 = t2;
        tw1 = t0;
        tw2 = t3;
        break;
    case 6: // V6(101), V1(100)
        tu1 = t0;
        tu2 = t3;
        tw1 = t1;
        tw2 = t2;
        break;
    default:
        tu1 = 0;
        tu2 = ticpwm;
        tv1 = 0;
        tv2 = ticpwm;
        tw1 = 0;
        tw2 = ticpwm;
        break;
    }

    // 计算中心对齐 PWM 的比较值（CCR = (上升沿 + 下降沿) / 2）

    svpwm.ticu = (u16)((tu1 + tu2) / 2.0f);
    svpwm.ticv = (u16)((tv1 + tv2) / 2.0f);
    svpwm.ticw = (u16)((tw1 + tw2) / 2.0f);
    // 更新比较值
    pwm_out();
}
// 电流采样点改变
u8 change_Index = 0;
void smaple_point_change()
{
    switch (svpwm.sector)
    {
    case 1:
    case 4:
        if (svpwm.ticu > ticDT + ticTN)
        {
            if (change_Index != 1)
                change_Index = 1;
            else
                return;
        }
        else if (alltic_tsdttn > 2 * svpwm.ticu)
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
        if (svpwm.ticv > ticDT + ticTN)
        {
            if (change_Index != 1)
                change_Index = 1;
            else
                return;
        }
        else if (alltic_tsdttn > 2 * svpwm.ticv)
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
        if (svpwm.ticw > ticDT + ticTN)
        {
            if (change_Index != 1)
                change_Index = 1;
            else
                return;
        }
        else if (alltic_tsdttn > 2 * svpwm.ticw)
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
        ADC_sample_change(svpwm.ticu - tics);
        break;
    case 3:
        ADC_sample_change(svpwm.ticu + ticDT + ticTN);
        break;
    default:
        break;
    }
}
void fGetPhaseVoltage(float *U, float *V, float *W)
{
    float a = svpwm.ticu * sqrt3 / svpwm.k;
    float b = svpwm.ticv * sqrt3 / svpwm.k;
    float c = svpwm.ticw * sqrt3 / svpwm.k;
    float vN = (a + b + c) / 3.0f;

    *U = a - vN;
    *V = b - vN;
    *W = c - vN;
}

void svpwm_SetVbus(float Vbus)
{
    svpwm.k = sqrt3 * ticpwm / Vbus;
}
u8 svpwm_GetSector()
{
    return svpwm.sector;
}