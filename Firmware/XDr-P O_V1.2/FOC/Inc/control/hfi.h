#ifndef __HFI_H
#define __HFI_H
#include "filter.h"

// ================= 配置参数 =================
// 具体数值依据见后文参数说明

// 直接以载波频率运行
#define HFI_OMEGA_E_BANDWIDTH 20.0f // 电转速带宽 (Hz)

#define HFI_INJ_VOLT_AMP 0.5f // 注入电压幅值 (V)
#define HFI_PLL_KP 50.0f      // PLL 比例增益
#define HFI_PLL_KI 1000.0f    // PLL 积分增益

#define HFI_INIT_VOLT 0.4f     // 初始辨识电压
#define HFI_MAX_OMEGA_E 150.0f // 最大电转速 (deg/s) 划分 HFI和SMO的界限

// ================= 数据结构 =================
typedef struct
{
    // 注入状态
    int8_t inj_signal; // 当前注入极性 (+1/-1)
    uint8_t inj_count;
    uint8_t freq_ticks;

    bool init_flag; // 初始位置标志位
    // 信号分离
    float ialpha_z[2];
    float ibeta_z[2];
    float ialpha_h[2];
    float ibeta_h[2];
    float i_hf_alpha, i_hf_beta;

    // PLL
    float theta_e; // 电角度 (rad)
    float omega_e; //
    float pll_error;
    float pll_integrator;
    float omega_filtered; // 滤波后角速度

    // 初始位置
    float id_h;
    float id_z[2];
    float init_curr_pos; /**<  +Ud 脉冲响应电流幅值 [A] */
    float init_curr_neg; /**<  -Ud 脉冲响应电流幅值 [A] */

} tHFI_Handle;

extern tHFI_Handle g_hfi;
// ================= 函数声明 =================
void fHFI_Init();
void fHFI_Step(float ialpha, float ibeta, float *u_alpha_h, float *u_beta_h);
void fHFI_DetectInitialPosition(float ialpha, float ibeta, float *ualpha, float *ubeta);

bool fHFI_GetStatus(void);
void fHFI_ResetInitialPosition(void);
float fHFI_GetOmegaElec(void);
float fHFI_GetThetaElec(void);
#endif // __HFI_H