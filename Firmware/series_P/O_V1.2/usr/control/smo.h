#ifndef __SMO_H
#define __SMO_H

#include "bsp.h"
#include "foc_core.h"

/**
 * @brief 无感滑模观测器 (SMO) - 精简版
 * @note 始终运行，外部直接读取结果，所有速度单位为电角速度 (rad/s)
 */

// ========== 配置宏 ==========
#define SMO_USE_CURRENT_OBSERVER 1 // 0=直接用反馈电流测试，1=启用完整电流观测器

// 观测器配置参数
typedef struct
{
    float k_sl_base;         // 基础滑模增益 [10.0~30.0]
    float k_sl_min_ratio;    // 高速最小增益比例 [0.3~0.7]
    float omega_adapt_start; // 自适应起始电角速度 [rad/s]
    float omega_adapt_end;   // 自适应结束电角速度 [rad/s]
    float delta;             // 边界层厚度 [0.05~0.2]
    float min_omega_elec;    // 最低有效电角速度 [rad/s]
    float max_omega_elec;    // 最高有效电角速度 [rad/s]
    float emf_max;           // 反电动势限幅 [V]
} tSMO_Config;

// SMO 主结构体
typedef struct
{
    // === 电机参数 (识别后导入) ===
    float Rs;
    float Ld;
    float Lq;
    float Psi_f;
    float dt;
    float inv_L_eff; // 预计算：1/(L_avg + Rs*dt)

    // === 观测器配置 ===
    tSMO_Config cfg;

    u8 Ts_tick;

    // === 电流观测状态 ===
    float i_alpha_hat;
    float i_beta_hat;

    // === 反电动势状态 ===
    float e_alpha;
    float e_beta;
    float e_alpha_filt;
    float e_beta_filt;

    // === 角度速度输出 ===
    float theta_elec; // 电角度 [0~360) deg
    float theta_prev;
    float omega_elec; // 电角速度 [rad/s]

    // === 内部缓存 ===
    float k_sl_curr;
} tSMO;

extern tSMO smo;

// === 接口函数 ===
void fSMO_Init(tMotor *motor);
void fSMO_Reset(void);
void fSMO_SetConfig(tSMO_Config *cfg);
void fSMO_MainLoop(float v_alpha, float v_beta, float i_alpha, float i_beta);

// === 数据获取 (内联零开销) ===
__STATIC_INLINE float fSMO_GetThetaDeg(void) { return smo.theta_elec; }
__STATIC_INLINE float fSMO_GetOmegaElec(void) { return smo.omega_elec; }

#endif // __SMO_H