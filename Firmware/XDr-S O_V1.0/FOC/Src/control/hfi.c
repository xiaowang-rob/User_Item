/**
 * @file    hfi.c
 * @brief   高频注入(HFI)无感位置估算实现
 * @note    核心算法基于αβ轴脉振方波注入 + PLL跟踪
 *          角度单位：全程使用【角度制 (degree)】
 *          纯电气角度系统，不涉及机械角度/极对数转换
 */

#include "hfi.h"
#include "filter.h"
#include "math_fast.h"
#include "drive_parameters.h"
/*================ 全局/静态变量 =================*/
HFI_Handle_t hfi_handle;
tBW_FilterInstance speed_lpf_inst;
tFirstOrderLagFilter speed_lpf;

/**
 * @brief 巴特沃斯低通滤波器系数 (用于速度平滑)
 * @note  由 lpf_coeffs.py 生成，截止频率需覆盖电机最大电角速度变化率
 *        系数顺序: [b0, b1, b2, a1, a2]
 *        对应: y[n] = b0*x[n]+b1*x[n-1]+b2*x[n-2] - a1*y[n-1]-a2*y[n-2]
 */
const float32_t lpf_w_coeffs[5] = {
    0.0020805671f, /* b0 */
    0.0041611343f, /* b1 */
    0.0020805671f, /* b2 */
    1.8147511963f, /* a1 */
    -0.9290728615f /* a2 */
};
/**
 * @brief HFI模块初始化
 */
void fHFI_Init()
{
    // 1. 状态清零
    memset(&hfi_handle, 0, sizeof(HFI_Handle_t));
    hfi_handle.inj_signal = 1; // 注入信号极性 (+1/-1)

    // 3. 初始化速度滤波器
    fButterworthFilter_Init(&speed_lpf_inst, (float *)lpf_w_coeffs);
    fFirstOrderLagInit(&speed_lpf, 0.01f, 0.0f);
}

/**
 * @brief HFI核心步进函数 (每个控制周期调用)
 * @param[in]  ialpha, ibeta   αβ轴电流采样 [A]
 * @param[out] u_alpha_h       注入的高频电压分量α [V]
 * @param[out] u_beta_h        注入的高频电压分量β [V]
 *
 * @note 算法流程对应理论推导:
 *       1. αβ轴直接注入 → 避免dq变换的角度延迟误差
 *       2. 加减分离法 → 提取高频响应 (di/dt)
 *       3. 矢量叉乘解调 → 获取位置误差 f(θ_err) ≈ k*sin(-2θ_err)
 *       4. PLL跟踪 → 收敛到真实电气位置
 *
 * 理论基础 (参考图片推导):
 *   di_αh/dt = [(-1)^k * U_in / (Ld*Lq)] * [(Ld+Lq)/2 * cosθ̂e - (Ld-Lq)/2 * cos(θe+θ̂e)]
 *   di_βh/dt = [(-1)^k * U_in / (Ld*Lq)] * [(Ld+Lq)/2 * sinθ̂e - (Ld-Lq)/2 * sin(θe+θ̂e)]
 *
 *   离散化: i_h(t) - i_h(t-1) ≈ di_h/dt * Ts
 *
 *   误差解耦: -I_αh*sinθ̂e + I_βh*cosθ̂e = k * sin(-2θ̃e)
 *   其中: θ̃e = θe - θ̂e (角度误差), k = [(-1)^k * U_in * Ts * (Ld-Lq)] / (2*Ld*Lq)
 *   前提: Ld < Lq (凸极性)
 */
#ifdef __DEBUG__
volatile static u32 T_hfi = 0;
static u32 T_zero = 0;

volatile float Hfi_Kp = 180;
volatile float Hfi_Ki = 16000;
#endif

void fHFI_Step(float ialpha, float ibeta, float *u_alpha_h, float *u_beta_h)
{
    hfi_handle.ialpha_h[0] = (ialpha - hfi_handle.ialpha_z[0] * 2 + hfi_handle.ialpha_z[1]) / 4;
    hfi_handle.ibeta_h[0] = (ibeta - hfi_handle.ibeta_z[0] * 2 + hfi_handle.ibeta_z[1]) / 4;

    hfi_handle.ialpha_z[1] = hfi_handle.ialpha_z[0];
    hfi_handle.ialpha_z[0] = ialpha;

    hfi_handle.ibeta_z[1] = hfi_handle.ibeta_z[0];
    hfi_handle.ibeta_z[0] = ibeta;

    hfi_handle.i_hf_alpha = (hfi_handle.ialpha_h[0] - hfi_handle.ialpha_h[1]) * hfi_handle.inj_signal;
    hfi_handle.i_hf_beta = (hfi_handle.ibeta_h[0] - hfi_handle.ibeta_h[1]) * hfi_handle.inj_signal;

    hfi_handle.ialpha_h[1] = hfi_handle.ialpha_h[0];
    hfi_handle.ibeta_h[1] = hfi_handle.ibeta_h[0];

    float sin_theta, cos_theta;

    // arm_sin_cos_f32 输入直接为角度制 (degree)
    arm_sin_cos_f32(hfi_handle.theta_e, &sin_theta, &cos_theta);

    hfi_handle.pll_error = hfi_handle.i_hf_alpha * sin_theta -
                           hfi_handle.i_hf_beta * cos_theta;

    float pll_prop = hfi_handle.pll_error * Hfi_Kp;

    // 积分项累加 (离散化: ∫e·dt ≈ Σe·Ts)
    hfi_handle.pll_integrator += hfi_handle.pll_error * Hfi_Ki * T_CON;

    hfi_handle.pll_integrator = CLAMP(hfi_handle.pll_integrator,
                                      -HFI_MAX_OMEGA_E,
                                      HFI_MAX_OMEGA_E);

    // PLL输出: 电气角速度 [degree/s]
    float32_t omega_e = pll_prop + hfi_handle.pll_integrator;

    /*=========================================================================
     * Step 5: 速度滤波 (巴特沃斯低通)
     * 目的: 抑制高频噪声，输出平滑电气速度用于后续控制/观测
     *=========================================================================*/
    hfi_handle.omega_filtered = fButterworthFilter_Process(&speed_lpf_inst, omega_e);
    //  hfi_handle.omega_filtered = fFirstOrderLagFilter(&speed_lpf, hfi_handle.omega_e);
    /*=========================================================================
     * Step 6: 角度更新与归一化
     * 公式: θ[k+1] = θ[k] + ω_e * Ts
     * 注意: 全程保持角度制 [degree]，纯电气角度系统
     *=========================================================================*/
    hfi_handle.theta_e += hfi_handle.omega_filtered * T_CON;
    hfi_handle.theta_e = fNormalizeAngle_0_360(hfi_handle.theta_e); // 归一化到 [0, 360)

    /*=========================================================================
     * Step 1: 更新注入信号 (脉振方波)
     * 原理: s_inj = (-1)^k, 频率 = HFI_INJ_FREQ_HZ
     *=========================================================================*/
    hfi_handle.inj_count++;
    if (hfi_handle.inj_count > 1)
    {
#ifdef __DEBUG__
        if (hfi_handle.inj_signal > 0)
        {
            u32 time = HAL_GetTick_us();
            T_hfi = time - T_zero;
            T_zero = time;
        }
#endif
        hfi_handle.inj_count = 0;
        hfi_handle.inj_signal = -hfi_handle.inj_signal; // 极性翻转
    }
    /*=========================================================================
     * Step 7: 生成注入电压 (αβ轴输出)
     * 原理: 在估计d轴施加方波电压，经Park逆变换到αβ坐标系
     * 公式: [u_α] = [cosθ  -sinθ] [U_inj]
     *       [u_β]   [sinθ   cosθ] [0    ]
     * 注意: 此处θ为估计值，存在误差但高频注入对此不敏感(自适应性)
     *=========================================================================*/
    float ud_inj = hfi_handle.inj_signal * HFI_INJ_VOLT_AMP; // 方波幅值

    // Park逆变换 (uq_inj = 0)
    *u_alpha_h = ud_inj * cos_theta;
    *u_beta_h = ud_inj * sin_theta;
}

/**
 * @brief 初始位置辨识 (磁饱和法)
 * @note    在HFI同频率定时器中运行
 *
 * @param[in]  ialpha    α轴电流采样 [A]
 * @param[in]  ibeta     β轴电流采样 [A]
 * @param[out] ud        d轴电压输出 [V]
 * @param[out] uq        q轴电压输出 [V]
 *
 * @return 0: 辨识成功, -1: 辨识中, -2: 辨识失败
 *
 * @note 极性辨识原理:
 *   1. 凸极电机存在磁饱和效应: d轴正向电流→磁路饱和→等效电感Ld↓→电流响应↑
 *
 *   2. 分阶段注入直流偏置电压，比较电流响应幅值:
 *      - 阶段1: 施加+Ud偏置，等待电流建立，采样|I_pos| = sqrt(ialpha²+ibeta²)
 *      - 阶段2: 撤除电压，等待电流衰减
 *      - 阶段3: 施加-Ud偏置，等待电流建立，采样|I_neg| = sqrt(ialpha²+ibeta²)
 *
 *   3. 判决逻辑:
 *      - 若 |I_pos| > |I_neg| → 正向饱和更明显 → 估计θ与真实θ同向 → θe = 0°
 *      - 若 |I_pos| < |I_neg| → 反向饱和更明显 → 估计θ与真实θ反向 → θe = 180°
 *
 *   4. 解决"倍角收敛"歧义:
 *      PLL可能收敛到θ或θ+180°，此步骤通过磁饱和效应确定唯一极性
 *
 * @note 调用方式:
 *   在HFI同频率定时器中断中周期性调用:
 *   - 每次调用执行一个阶段
 *   - 通过返回值判断状态: 1 2 3 4 5(进行中), 0(完成), 6(失败)
 *   - 需保证调用频率 = HFI_CTRL_FREQ_HZ
 */
static uint16_t detect_timer = 0;
void fHFI_DetectInitialPosition(float ialpha, float ibeta, float *ualpha, float *ubeta)
{
    if (hfi_handle.init_flag)
        return; // 已完成初始位置辨识，跳过

    float id, iq, ud_ref;
    fParkTransform(ialpha, ibeta, hfi_handle.theta_e, &id, &iq);

    detect_timer++;
    hfi_handle.id_h = (id - 2 * hfi_handle.id_z[0] + hfi_handle.id_z[1]) / 4;
    hfi_handle.id_z[1] = hfi_handle.id_z[0];
    hfi_handle.id_z[0] = id;

    if (detect_timer < 400) // 0
    {
        ud_ref = 0.0f;
    }
    else if (detect_timer < 600) // 施加正偏置
    {
        ud_ref = 0.4f;
    }
    else if (detect_timer < 610) // 正偏置 加采样
    {
        ud_ref = 0.4f;
        hfi_handle.init_curr_pos += FABSF(hfi_handle.id_h);
    }
    else if (detect_timer < 1000) // 静置
    {
        ud_ref = 0.0f;
    }
    else if (detect_timer < 1200) // 施加反偏置
    {
        ud_ref = -0.4f;
    }
    else if (detect_timer < 1210) // 反偏置 加采样
    {
        ud_ref = -0.4f;
        hfi_handle.init_curr_neg += FABSF(hfi_handle.id_h);
    }
    else // 计算
    {
        ud_ref = 0.0f;
        if (hfi_handle.init_curr_pos < hfi_handle.init_curr_neg)
        {
            hfi_handle.theta_e += 180.0f;
            hfi_handle.theta_e = fNormalizeAngle_0_360(hfi_handle.theta_e);
        }
        hfi_handle.init_flag = true;
    }

    fInvParkTransform(ud_ref, 0.0f, hfi_handle.theta_e, ualpha, ubeta);
}
bool fHFI_GetStatus(void) { return hfi_handle.init_flag; }
/**
 * @brief 重置初始位置辨识状态
 * @note 在需要重新进行极性辨识时调用
 */
void fHFI_ResetInitialPosition(void)
{
    memset(&hfi_handle, 0, sizeof(HFI_Handle_t));
    hfi_handle.inj_signal = 1; // 注入信号极性 (+1/-1)
    fButterworthFilter_Reset(&speed_lpf_inst);
    // 外部函数无法直接访问HFI_DetectInitialPosition的静态变量
    // 如需重置，建议在HFI_DetectInitialPosition中添加重置参数
}
// 获取电角速度
float fHFI_GetOmegaElec(void)
{
    return hfi_handle.omega_e;
    // return hfi_handle.omega_filtered;
}
// 获取电角度
float fHFI_GetThetaElec(void)
{
    return hfi_handle.theta_e;
}
