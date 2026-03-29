#ifndef __HFI_H
#define __HFI_H
#include "filter.h"
#include "drive_parameters.h"

// ================= 配置参数 =================
// 具体数值依据见后文参数说明
#define HFI_INJ_FREQ_HZ 500.0f  // 注入频率
#define HFI_INJ_VOLT_AMP 0.3f   // 注入电压幅值 (V)
#define HFI_PLL_KP 0.8f         // PLL 比例增益
#define HFI_PLL_KI 15.0f        // PLL 积分增益
#define HFI_CTRL_FREQ_HZ fpwm   // 控制中断频率
#define HFI_INIT_VOLT 0.4f      // 初始辨识电压
#define HFI_MAX_OMEGA_E 1000.0f // 最大电转速 (deg/s) 划分 HFI和SMO的界限

// ================= 数据结构 =================
typedef struct
{
    // 注入状态
    float32_t inj_signal; // 当前注入极性 (+1/-1)
    u32 inj_counter;
    u32 inj_period_ticks;

    // 信号分离
    float32_t ialpha_prev, ibeta_prev;
    float32_t i_hf_alpha, i_hf_beta;

    // PLL
    float32_t theta_e; // 电角度 (rad)
    float32_t omega_e; // 原始角速度
    float32_t pll_error;
    float32_t pll_integrator;
    float32_t omega_filtered; // 滤波后角速度

    // 初始位置
    u8 detect_state;
    float32_t init_curr_pos; /**<  +Ud 脉冲响应电流幅值 [A] */
    float32_t init_curr_neg; /**<  -Ud 脉冲响应电流幅值 [A] */
} HFI_Handle_t;

// ================= 函数声明 =================
void HFI_Init();
void HFI_Step(float32_t ialpha, float32_t ibeta, float32_t *u_alpha_h, float32_t *u_beta_h);
int8_t HFI_DetectInitialPosition(float32_t ialpha, float32_t ibeta,
                                 float32_t *ud, float32_t *uq);
#endif // __HFI_H