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

/*================ 全局/静态变量 =================*/
static HFI_Handle_t hfi_handle;
static tBW_FilterInstance speed_lpf_inst;

/**
 * @brief 巴特沃斯低通滤波器系数 (用于速度平滑)
 * @note  由 lpf_coeffs.py 生成，截止频率需覆盖电机最大电角速度变化率
 *        系数顺序: [b0, b1, b2, a1, a2]
 *        对应: y[n] = b0*x[n]+b1*x[n-1]+b2*x[n-2] - a1*y[n-1]-a2*y[n-2]
 */
static const float32_t lpf_coeffs[5] = {
    0.0009446918f, 0.0018893837f, 0.0009446918f, -1.9111970674f, 0.9149758348f};

/**
 * @brief HFI模块初始化
 */
void HFI_Init()
{
    // 1. 状态清零
    hfi_handle.theta_e = 0.0f;        // 电气角度 [degree]
    hfi_handle.omega_e = 0.0f;        // 电气角速度 [degree/s]
    hfi_handle.omega_filtered = 0.0f; // 滤波后电气速度
    hfi_handle.pll_integrator = 0.0f; // PLL积分项
    hfi_handle.ialpha_prev = 0.0f;    // 上一拍电流采样
    hfi_handle.ibeta_prev = 0.0f;
    hfi_handle.inj_signal = 1.0f; // 注入信号极性 (+1/-1)
    hfi_handle.detect_state = 1;  // 初始位置辨识标志

    // 2. 计算注入信号半周期计数
    // 原理: 方波频率 = InjFreq, 每半周期切换一次极性以产生高频激励
    // 公式: T_half_ticks = f_pwm / (f_inj * 2)
    hfi_handle.inj_period_ticks = (uint32_t)(HFI_CTRL_FREQ_HZ / (HFI_INJ_FREQ_HZ * 2.0f));
    if (hfi_handle.inj_period_ticks < 1)
        hfi_handle.inj_period_ticks = 1; // 防止除零或过小
    hfi_handle.inj_counter = 0;

    // 3. 初始化速度滤波器
    fButterworthFilter_Init(&speed_lpf_inst, (float32_t *)lpf_coeffs);
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
#endif
void HFI_Step(float32_t ialpha, float32_t ibeta, float32_t *u_alpha_h, float32_t *u_beta_h)
{
    /*=========================================================================
     * Step 1: 更新注入信号 (脉振方波)
     * 原理: s_inj = (-1)^k, 频率 = HFI_INJ_FREQ_HZ
     *=========================================================================*/
    hfi_handle.inj_counter++;
    if (hfi_handle.inj_counter >= hfi_handle.inj_period_ticks)
    {
#ifdef __DEBUG__
        if (hfi_handle.inj_signal > 0)
        {
            u32 time = HAL_GetTick_us();
            T_hfi = time - T_zero;
            T_zero = time;
        }
#endif
        hfi_handle.inj_counter = 0;
        hfi_handle.inj_signal = -hfi_handle.inj_signal; // 极性翻转
    }

    /*=========================================================================
     * Step 2: 高频电流分离 (加减分离法)
     * 原理: 利用低频基波电流变化缓慢(Δi_lf≈0), 高频响应正负交替的特性
     * 公式: i_hf ≈ (i[k] - i[k-1]) * s_inj[k]
     *       对应离散化 di/dt 提取，消除基波分量
     *
     * 理论对应:
     *   i_αh(t) - i_αh(t-1) = [(-1)^k * U_in * Ts / (Ld*Lq)] * [...]
     *   i_βh(t) - i_βh(t-1) = [(-1)^k * U_in * Ts / (Ld*Lq)] * [...]
     *=========================================================================*/
    float32_t delta_ialpha = ialpha - hfi_handle.ialpha_prev;
    float32_t delta_ibeta = ibeta - hfi_handle.ibeta_prev;

    // 乘以注入极性，对齐高频响应方向 (解调前置处理)
    // 对应: I_αh = (i_αh(t) - i_αh(t-1)) * (-1)^k
    hfi_handle.i_hf_alpha = delta_ialpha * hfi_handle.inj_signal;
    hfi_handle.i_hf_beta = delta_ibeta * hfi_handle.inj_signal;

    hfi_handle.ialpha_prev = ialpha;
    hfi_handle.ibeta_prev = ibeta;

    /*=========================================================================
     * Step 3: 解调 (矢量叉乘提取位置误差)
     *
     * 理论推导 (参考图片3):
     *   -I_αh*sinθ̂e + I_βh*cosθ̂e
     *   = [(-1)^k * U_in * Ts * (Ld-Lq) / (2*Ld*Lq)] * sin(-2θ̃e)
     *   ≈ k * (-2θ̃e)  (当θ̃e较小时，sin(-2θ̃e) ≈ -2θ̃e)
     *
     * 其中:
     *   - θ̂e: 估计电气角度
     *   - θe: 真实电气角度
     *   - θ̃e = θe - θ̂e: 角度误差
     *   - k = [(-1)^k * U_in * Ts * (Ld-Lq)] / (2*Ld*Lq)
     *
     * 前提条件: Ld < Lq (凸极性，保证k为负值，误差信号负反馈)
     *=========================================================================*/
    float32_t sin_theta, cos_theta;

    // arm_sin_cos_f32 输入直接为角度制 (degree)
    arm_sin_cos_f32(hfi_handle.theta_e, &sin_theta, &cos_theta);

    // 倍角公式: sin2θ = 2sinθcosθ, cos2θ = cos²θ - sin²θ
    // 用于构造包含2θ分量的误差信号
    float32_t sin_2theta = 2.0f * sin_theta * cos_theta;
    float32_t cos_2theta = cos_theta * cos_theta - sin_theta * sin_theta;

    // 计算位置误差信号 (假设凸极率 Lq > Ld)
    // 对应理论: -I_αh*sinθ̂e + I_βh*cosθ̂e
    // 代码实现采用倍角形式，等价于提取sin(-2θ̃e)分量
    hfi_handle.pll_error = hfi_handle.i_hf_alpha * (-sin_2theta) +
                           hfi_handle.i_hf_beta * (cos_2theta);

    /*=========================================================================
     * Step 4: PLL 锁相环跟踪
     * 结构: 比例-积分型观测器
     * 输出: omega_e [degree/s], theta_e [degree]
     *
     * 参数设计原则:
     *   ω_n (自然频率): 决定带宽，建议 2π*10~50 rad/s (电角度)
     *   ζ (阻尼比): 0.707 最佳阻尼
     *   Kp = 2*ζ*ω_n, Ki = ω_n²
     *   注意: 代码中 Ki 需乘以 Ts = 1/HFI_CTRL_FREQ_HZ 进行离散化
     *=========================================================================*/
    float32_t pll_prop = hfi_handle.pll_error * HFI_PLL_KP;

    // 积分项累加 (离散化: ∫e·dt ≈ Σe·Ts)
    hfi_handle.pll_integrator += hfi_handle.pll_error * HFI_PLL_KI * (1.0f / HFI_CTRL_FREQ_HZ);

    // 积分限幅: 防止积分饱和，限幅值 = 最大电气角速度 [degree/s]
    // 纯电气角度系统，直接限幅电气转速
    // 示例: 最大电气转速 12000 degree/s (对应200Hz电频率)
    hfi_handle.pll_integrator = CLAMP(hfi_handle.pll_integrator,
                                      -HFI_MAX_OMEGA_E,
                                      HFI_MAX_OMEGA_E);

    // PLL输出: 电气角速度 [degree/s]
    hfi_handle.omega_e = pll_prop + hfi_handle.pll_integrator;

    /*=========================================================================
     * Step 5: 速度滤波 (巴特沃斯低通)
     * 目的: 抑制高频噪声，输出平滑电气速度用于后续控制/观测
     *=========================================================================*/
    hfi_handle.omega_filtered = fButterworthFilter_Process(&speed_lpf_inst, hfi_handle.omega_e);

    /*=========================================================================
     * Step 6: 角度更新与归一化
     * 公式: θ[k+1] = θ[k] + ω_e * Ts
     * 注意: 全程保持角度制 [degree]，纯电气角度系统
     *=========================================================================*/
    hfi_handle.theta_e += hfi_handle.omega_e * (1.0f / HFI_CTRL_FREQ_HZ);
    hfi_handle.theta_e = fNormalizeAngle_0_360(hfi_handle.theta_e); // 归一化到 [0, 360)

    /*=========================================================================
     * Step 7: 生成注入电压 (αβ轴输出)
     * 原理: 在估计d轴施加方波电压，经Park逆变换到αβ坐标系
     * 公式: [u_α] = [cosθ  -sinθ] [U_inj]
     *       [u_β]   [sinθ   cosθ] [0    ]
     * 注意: 此处θ为估计值，存在误差但高频注入对此不敏感(自适应性)
     *=========================================================================*/
    float32_t ud_inj = hfi_handle.inj_signal * HFI_INJ_VOLT_AMP; // 方波幅值

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
int8_t HFI_DetectInitialPosition(float32_t ialpha, float32_t ibeta,
                                 float32_t *ud, float32_t *uq)
{

    // 状态机实现，适应定时器周期性调用
    switch (hfi_handle.detect_state)
    {
    /*--- 初始化 ---*/
    case 1:
        detect_timer = 0;
        hfi_handle.detect_state = 2;
        break;
    /*--- 施加 +Ud 偏置 (磁饱和方向) ---*/
    case 2:
        *ud = HFI_INIT_VOLT;
        *uq = 0.0f;
        detect_timer++;

        // 等待电流建立 (例如: 800个控制周期 @10kHz = 80ms)
        if (detect_timer >= 800)
        {
            // 采样电流幅值: |I| = sqrt(ialpha² + ibeta²)
            arm_sqrt_f32(ialpha * ialpha + ibeta * ibeta, &hfi_handle.init_curr_pos);
            detect_timer = 0;
            hfi_handle.detect_state = 2;
        }
        break;

    /*--- 阶段2: 撤除电压，等待衰减 ---*/
    case 3:
        *ud = 0.0f;
        *uq = 0.0f;
        detect_timer++;

        // 等待电流衰减 (例如: 2000个控制周期 @10kHz = 200ms)
        if (detect_timer >= 2000)
        {
            detect_timer = 0;
            hfi_handle.detect_state = 3;
        }
        break;

    /*--- 阶段3: 施加 -Ud 偏置 (退磁方向) ---*/
    case 4:
        *ud = -HFI_INIT_VOLT;
        *uq = 0.0f;
        detect_timer++;

        // 等待电流建立 (例如: 800个控制周期)
        if (detect_timer >= 800)
        {
            // 采样电流幅值
            arm_sqrt_f32(ialpha * ialpha + ibeta * ibeta, &hfi_handle.init_curr_neg);
            detect_timer = 0;
            hfi_handle.detect_state = 4;
        }
        break;

    /*--- 阶段4: 撤除电压，完成辨识 ---*/
    case 5:
        *ud = 0.0f;
        *uq = 0.0f;

        /*--- 极性判决 ---*/
        // 比较幅值: 电流大的一侧对应电感小(饱和)，即真实磁极方向
        // 增加滞回比较(5%)防止临界点抖动
        if (hfi_handle.init_curr_pos > hfi_handle.init_curr_neg * 1.05f)
        {
            // 正向饱和更明显 → 估计角度正确
            hfi_handle.theta_e = 0.0f;
        }
        else if (hfi_handle.init_curr_neg > hfi_handle.init_curr_pos * 1.05f)
        {
            // 反向饱和更明显 → 估计角度反向，需+180°矫正
            hfi_handle.theta_e = 180.0f;
        }
        else
        {
            // 幅值差异过小，辨识不可靠
            hfi_handle.detect_state = 1; // 重置状态
        }

        hfi_handle.detect_state = 0; // 重置状态，便于下次调用

    default:
        return 0;
    }
    return hfi_handle.detect_state;
}

/**
 * @brief 重置初始位置辨识状态
 * @note 在需要重新进行极性辨识时调用
 */
void HFI_ResetInitialPosition(void)
{
    hfi_handle.detect_state = 1;
    fButterworthFilter_Reset(&speed_lpf_inst);
    // 外部函数无法直接访问HFI_DetectInitialPosition的静态变量
    // 如需重置，建议在HFI_DetectInitialPosition中添加重置参数
}