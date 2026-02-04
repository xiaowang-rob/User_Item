#include "svpwm.h"
#include "tim.h"
#include "math_fast.h"
#include "string.h"
#include "drive_parameters.h"
#include "device.h"
#include "adc_dr.h"
// mos管 死区 采样 造势时间
const u16 ticTs = Tsample_us * ticpwm / (Tpwm * 1000000);
const u16 ticTd = Tdeath_us * ticpwm / (Tpwm * 1000000);
const u16 ticTn = Tnoise_us * ticpwm / (Tpwm * 1000000);
const u16 all_sdc = ticTs + ticTd + ticTn; // 总计数值

tSvpwm svpwm = {0};

void fSvpwmInit(float Vbus)
{
    memset(&svpwm, 0, sizeof(tSvpwm));

    svpwm.k = MATH_SQRT3 * (float)ticpwm / Vbus;
}
void inline pwm_out()
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
    PWM_POWER_ON();
}
void DISABLE_PWM()
{
    HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_3);
    HAL_TIMEx_PWMN_Stop(&htim8, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Stop(&htim8, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Stop(&htim8, TIM_CHANNEL_3);
    PWM_POWER_OFF();
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
void fSvpwmRun(float ualpha, float ubeta)
{
    float U1 = ubeta;
    float U2 = MATH_SQRT3_2 * ualpha - 0.5 * ubeta;
    float U3 = -U2 - ubeta;
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
        svpwm.sector = 2;
        Tx = -svpwm.k * U2;
        Ty = -svpwm.k * U3;
        break;
    case 2:
        svpwm.sector = 6;
        Tx = -svpwm.k * U3;
        Ty = -svpwm.k * U1;
        break;
    case 3:
        svpwm.sector = 1;
        Tx = svpwm.k * U2;
        Ty = svpwm.k * U1;
        break;
    case 4:
        svpwm.sector = 4;
        Tx = -svpwm.k * U1;
        Ty = -svpwm.k * U2;
        break;
    case 5:
        svpwm.sector = 3;
        Tx = svpwm.k * U1;
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

    float t0 = Tzero / 2;   // V0作用起点零向量（0，0，0）
    float t1 = ticpwm - t0; // V1作用
    float t2 = t1 - Tx;     // V2作用
    float t3 = t2 - Ty;     // V3作用

    float tu = t3; // 默认全低
    float tv = t3;
    float tw = t3;

    switch (svpwm.sector)
    {
    case 0: // V0(000)
        tu = 0;
        tv = 0;
        tw = 0;
        break;
    case 1: // V1(100), V2(110)
        tu = t1;
        tv = t2;
        break;
    case 2: // V2(110), V3(010)
        tv = t1;
        tu = t2;
        break;
    case 3: // V3(010), V4(011)
        tv = t1;
        tw = t2;
        break;
    case 4: // V4(011), V5(001)
        tw = t1;
        tv = t2;
        break;
    case 5: // V5(001), V6(101)
        tw = t1;
        tu = t2;
        break;
    case 6: // V6(101), V1(100)
        tu = t1;
        tw = t2;
        break;
    default:         // (1,1,1)
        tu = ticpwm; //
        tv = ticpwm;
        tw = ticpwm;
        break;
    }

    // 计算中心对齐 PWM 的比较值（CCR = (上升沿 + 下降沿) / 2）
    svpwm.ticu = (u16)tu;
    svpwm.ticv = (u16)tv;
    svpwm.ticw = (u16)tw;
    // 更新比较值
    pwm_out();
}
// 电流采样点改变
u8 change_Index = 0;
void fSamplePointCalibration()
{
    switch (svpwm.sector)
    {
    case 0:
    case 7:
        change_Index = 1;
        break;
    case 1:
    case 4:
        if (svpwm.ticu > ticTd + ticTn)
        {
            if (change_Index != 1)
                change_Index = 1;
            else
                return;
        }
        else if (all_sdc > 2 * svpwm.ticu)
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
        if (svpwm.ticv > ticTd + ticTn)
        {
            if (change_Index != 1)
                change_Index = 1;
            else
                return;
        }
        else if (all_sdc > 2 * svpwm.ticv)
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
    default: // 3,6
        if (svpwm.ticw > ticTd + ticTn)
        {
            if (change_Index != 1)
                change_Index = 1;
            else
                return;
        }
        else if (all_sdc > 2 * svpwm.ticw)
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
    case 1: // 低调制 25us开始采样
        fAdcSampleChange(ticpwm - 50);
        break;
    case 2: // 中调制 u相高打开前 ts（采样时间）开始采样
        fAdcSampleChange(svpwm.ticu + ticTs);
        break;
    case 3: // 高调制 u相高开启之后 ts tn 开始采样
        fAdcSampleChange(svpwm.ticu - ticTd - ticTn);
        break;
    default:
        break;
    }
}
float fGetVoltage_u()
{
    return svpwm.ticu * MATH_SQRT3 / svpwm.k;
}
float fGetVoltage_v()
{
    return svpwm.ticv * MATH_SQRT3 / svpwm.k;
}
float fGetVoltage_w()
{
    return svpwm.ticw * MATH_SQRT3 / svpwm.k;
}

void fSvpwmSetVbus(float Vbus)
{
    svpwm.k = MATH_SQRT3 * ticpwm / Vbus;
}
u8 fSvpwmGetSector()
{
    return svpwm.sector;
}