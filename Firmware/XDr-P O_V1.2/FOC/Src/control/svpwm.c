#include "svpwm.h"
#include "tim.h"
#include "math_fast.h"
#include "string.h"
#include "drive_parameters.h"
#include "device.h"
#include "adc_dr.h"
// mos管 死区 采样 造势时间
const u16 ticTs = T_SAMPLE_us * TIC_PWM / (T_PWM * 1000000);
const u16 ticTd = T_DEATH_us * TIC_PWM / (T_PWM * 1000000);
const u16 ticTn = T_NOISE_us * TIC_PWM / (T_PWM * 1000000);
const u16 all_sdc = ticTs + ticTd + ticTn; // 总计数值

tSvpwm svpwm = {0};

void fSvpwmInit(float Vbus)
{
    memset(&svpwm, 0, sizeof(tSvpwm));
    svpwm.k = MATH_SQRT3 * (float)TIC_PWM / Vbus;
    DISABLE_PWM();

    PWM_POWER_ON();
}

__STATIC_INLINE void pwm_out()
{
    __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, svpwm.ticw);
    __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, svpwm.ticv);
    __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_3, svpwm.ticu);
}

void ENABLE_PWM()
{
    //    PWM_POWER_ON();
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_3);

    HAL_TIMEx_PWMN_Start(&htim8, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&htim8, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Start(&htim8, TIM_CHANNEL_3);
}
void DISABLE_PWM()
{
    //    PWM_POWER_OFF();
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

void fSvpwmRun(float ualpha, float ubeta)
{
    // 反clark变换，不是标准的，只是为了方便判断扇区
    float U1 = ubeta;
    float U2 = MATH_SQRT3_2 * ualpha - 0.5f * ubeta;
    float U3 = -U2 - ubeta;

    u8 A = U1 > 0;
    u8 B = U2 > 0;
    u8 C = U3 > 0;
    u8 vN = 4 * C + 2 * B + A;

    // 计算三相电压时间分量，超前当前转子三轴90°，定子的三轴需要输出的电压，转化为计数值
    float X = svpwm.k * U1;
    float Y = svpwm.k * U3;
    float Z = svpwm.k * U2;

    float T1, T2, T0; // T1:第一相作用时间 T2:第二相作用时间 T0:零向量作用时间
    // 分配时间和扇区（扇区是转子的扇区）
    switch (vN)
    {
    case 0:               // Zero vector (all negative)
    case 7:               // Zero vector (all positive)
        svpwm.sector = 0; // Use special sector 0
        T1 = 0;
        T2 = 0;
        break;
    case 1:
        svpwm.sector = 2; //+--
        T1 = -Y;
        T2 = -Z;
        break;
    case 2:
        svpwm.sector = 6; //--+
        T1 = -X;
        T2 = -Y;
        break;
    case 3:
        svpwm.sector = 1; //+-+
        T1 = X;
        T2 = Z;
        break;
    case 4:
        svpwm.sector = 4; //-+-
        T1 = -Z;
        T2 = -X;
        break;
    case 5:
        svpwm.sector = 3; //++-
        T1 = Y;
        T2 = X;
        break;
    case 6:
        svpwm.sector = 5; //-++
        T1 = Z;
        T2 = Y;
        break;
    default:
        break;
    }

    if (T1 + T2 > TIC_PWM)
    {
        float ratio = TIC_PWM / (T1 + T2);
        T1 *= ratio;
        T2 *= ratio;
        T0 = 0;
    }
    else
    {
        T0 = TIC_PWM - T1 - T2;
    }
    // 以七段式开关序列方式输出--更小的电流纹波和中心对称性（5段 可以减小开关次数）

    float t0 = T0 * 0.5f; // V0作用起点零向量（0，0，0）
    float t1 = t0 + T1;   // V1作用
    float t2 = t1 + T2;   // V2作用

    switch (svpwm.sector)
    {
    case 1: // V1(100), V2(110)
        svpwm.ticu = (u16)t2;
        svpwm.ticv = (u16)t1;
        svpwm.ticw = (u16)t0;
        break;
    case 2: // V2(110), V3(010)
        svpwm.ticu = (u16)t1;
        svpwm.ticv = (u16)t2;
        svpwm.ticw = (u16)t0;
        break;
    case 3: // V3(010), V4(011)
        svpwm.ticu = (u16)t0;
        svpwm.ticv = (u16)t2;
        svpwm.ticw = (u16)t1;
        break;
    case 4: // V4(011), V5(001)
        svpwm.ticu = (u16)t0;
        svpwm.ticv = (u16)t1;
        svpwm.ticw = (u16)t2;
        break;
    case 5: // V5(001), V6(101)
        svpwm.ticu = (u16)t1;
        svpwm.ticv = (u16)t0;
        svpwm.ticw = (u16)t2;
        break;
    case 6: // V6(101), V1(100)
        svpwm.ticu = (u16)t2;
        svpwm.ticv = (u16)t0;
        svpwm.ticw = (u16)t1;
        break;
    default: // (1,1,1)
        svpwm.ticu = (u16)t0;
        svpwm.ticv = (u16)t0;
        svpwm.ticw = (u16)t0;
        break;
    }

    // 更新比较值
    pwm_out();
}
// 电流采样点改变
u8 change_Index = 0;
void fSamplePointCalibration()
{
    u16 tic_ref; // 当前扇区的参考相计数

    // 根据扇区确定参考相，并保存tic值
    switch (svpwm.sector)
    {
    case 0:
    case 7:
        tic_ref = TIC_PWM / 2;
        goto evaluate;
        break;
    case 1:
    case 6:
        tic_ref = svpwm.ticu;
        goto evaluate;
    case 2:
    case 3:
        tic_ref = svpwm.ticv;
        goto evaluate;
    default: // 45
        tic_ref = svpwm.ticw;
        goto evaluate;
    }

evaluate:
    // 根据参考相占空比判断调制深度
    if (tic_ref > ticTd + ticTn)
    {
        change_Index = 1; // 低调制
    }
    else if (all_sdc > 2 * tic_ref)
    {
        change_Index = 3; // 高调制
    }
    else
    {
        change_Index = 2; // 中调制
    }

    // 设置ADC采样触发点
    switch (change_Index)
    {
    case 1:
        //        fAdcSampleChange(ticpwm - 1);
        fAdcSampleChange(tic_ref - ticTs);
        break;
    case 2:
        //        fAdcSampleChange(tic_ref + ticTs);
        fAdcSampleChange(tic_ref - ticTs);
        break;
    case 3:
        // 确保减后不溢出（可根据实际需求加限幅）
        if (tic_ref >= ticTd + ticTn)
            fAdcSampleChange(tic_ref - ticTd - ticTn);
        else
            fAdcSampleChange(0);
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
    svpwm.k = MATH_SQRT3 * TIC_PWM / Vbus;
}

u8 fSvpwmGetSector()
{
    return svpwm.sector;
}