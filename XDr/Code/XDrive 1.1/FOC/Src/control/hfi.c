#include "hfi.h"

void HFI_Init(HFI_Handle_t *hfi, tBW_FilterInstance *lpf_inst, float32_t ctrl_freq)
{
    hfi->theta_e = 0.0f;
    hfi->omega_e = 0.0f;
    hfi->omega_filtered = 0.0f;
    hfi->pll_integrator = 0.0f;
    hfi->ia_prev = 0.0f;
    hfi->ib_prev = 0.0f;
    hfi->inj_signal = 1.0f;
    hfi->init_done = 0;
    hfi->speed_lpf = lpf_inst; // 绑定外部滤波器实例

    // 计算注入半周期 ticks
    // 方波频率 = InjFreq, 每个半周期切换一次极性
    hfi->inj_period_ticks = (uint32_t)(ctrl_freq / (HFI_INJ_FREQ_HZ * 2.0f));
    if (hfi->inj_period_ticks < 1)
        hfi->inj_period_ticks = 1;
    hfi->inj_counter = 0;
}

void HFI_Step(HFI_Handle_t *hfi, float32_t ia, float32_t ib, float32_t *u_alpha_h, float32_t *u_beta_h)
{
    // 1. 更新注入信号 (脉振方波)
    hfi->inj_counter++;
    if (hfi->inj_counter >= hfi->inj_period_ticks)
    {
        hfi->inj_counter = 0;
        hfi->inj_signal = -hfi->inj_signal;
    }

    // 2. 高频电流分离 (加减分离法)
    // 对应公式中的 di/dt 离散化提取
    float32_t delta_ia = ia - hfi->ia_prev;
    float32_t delta_ib = ib - hfi->ib_prev;

    // 乘以注入极性，对齐高频响应方向
    hfi->i_hf_alpha = delta_ia * hfi->inj_signal;
    hfi->i_hf_beta = delta_ib * hfi->inj_signal;

    hfi->ia_prev = ia;
    hfi->ib_prev = ib;

    // 3. 解调 (矢量叉乘)
    // 基于公式推导的位置误差信号提取
    float32_t sin_theta, cos_theta;
    arm_sin_cos_f32(hfi->theta_e, &sin_theta, &cos_theta);

    // 倍角公式
    float32_t sin_2theta = 2.0f * sin_theta * cos_theta;
    float32_t cos_2theta = cos_theta * cos_theta - sin_theta * sin_theta;

    // 误差信号 (假设 Ld < Lq)
    // 如果电机反转或发散，尝试更改此处符号
    hfi->pll_error = hfi->i_hf_alpha * (-sin_2theta) + hfi->i_hf_beta * (cos_2theta);

    // 4. PLL 跟踪
    float32_t pll_prop = hfi->pll_error * HFI_PLL_KP;
    hfi->pll_integrator += hfi->pll_error * HFI_PLL_KI * (1.0f / HFI_CTRL_FREQ_HZ);
    hfi->pll_integrator = CLAMP(hfi->pll_integrator, -2000.0f, 2000.0f);

    hfi->omega_e = pll_prop + hfi->pll_integrator;

    // 5. 速度滤波 (巴特沃斯滤波器)
    if (hfi->speed_lpf != NULL)
    {
        hfi->omega_filtered = fButterworthFilter_Process(hfi->speed_lpf, hfi->omega_e);
    }
    else
    {
        hfi->omega_filtered = hfi->omega_e;
    }

    // 6. 更新角度
    hfi->theta_e += hfi->omega_e * (1.0f / HFI_CTRL_FREQ_HZ);
    // 角度归一化
    if (hfi->theta_e >= 2.0f * 3.1415926535f)
        hfi->theta_e -= 2.0f * 3.1415926535f;
    if (hfi->theta_e < 0.0f)
        hfi->theta_e += 2.0f * 3.1415926535f;

    // 7. 生成注入电压分量 (仅输出高频分量)
    // 在估计的 d 轴施加方波电压，经 Park 逆变换到 alpha-beta
    float32_t ud_inj = hfi->inj_signal * HFI_INJ_VOLT_AMP;

    // u_alpha = ud * cos - uq * sin (uq=0)
    // u_beta  = ud * sin + uq * cos
    *u_alpha_h = ud_inj * cos_theta;
    *u_beta_h = ud_inj * sin_theta;
}

// 初始位置辨识 (磁饱和法)
int8_t HFI_DetectInitialPosition(HFI_Handle_t *hfi,
                                 float32_t (*get_ia)(void),
                                 float32_t (*get_ib)(void),
                                 void (*apply_volt_dq)(float32_t ud, float32_t uq))
{
    // 1. 施加 +Ud
    apply_volt_dq(HFI_INIT_VOLT, 0.0f);
    for (volatile int i = 0; i < 800; i++)
        ; // 延时等待电流建立
    arm_sqrt_f32(get_ia() * get_ia() + get_ib() * get_ib(), &hfi->init_curr_pos);

    apply_volt_dq(0.0f, 0.0f);
    for (volatile int i = 0; i < 2000; i++)
        ; // 等待衰减

    // 2. 施加 -Ud
    apply_volt_dq(-HFI_INIT_VOLT, 0.0f);
    for (volatile int i = 0; i < 800; i++)
        ;
    arm_sqrt_f32(get_ia() * get_ia() + get_ib() * get_ib(), &hfi->init_curr_neg);

    apply_volt_dq(0.0f, 0.0f);

    // 3. 判断极性
    // 根据磁饱和效应，电流大的一侧电感小，对应磁路饱和方向
    if (hfi->init_curr_pos > hfi->init_curr_neg)
    {
        hfi->theta_e = 0.0f;
    }
    else
    {
        hfi->theta_e = 3.1415926535f;
    }

    hfi->init_done = 1;
    return 0;
}