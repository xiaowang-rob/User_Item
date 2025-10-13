#ifndef __SMO_H
#define __SMO_H

#include "main.h"
/*****************************************无感SMO观测*********************************** */
// 无感 SMO 结构体
typedef struct
{
    float Rs;   // 电阻
    float Ls;   // 电感
    float dt;   // 控制周期
    float k_sl; // 滑模增益
    float k_f;  // 滤波器增益

    // 状态变量
    float i_alpha_hat;
    float i_beta_hat;
    float e_alpha;
    float e_beta;
    float e_alpha_filtered;
    float e_beta_filtered;

    // 输出
    float theta; // 电角度
    float omega; // 电角速度
    float theta_prev;
} smo_sensorless_t;
// 初始化
void smo_sensorless_init(smo_sensorless_t *smo, float Rs, float Ls, float dt);

// 更新（输入：电压、电流）
void smo_sensorless_update(smo_sensorless_t *smo,
                           float v_alpha, float v_beta,
                           float i_alpha, float i_beta);

// 获取结果
float smo_sensorless_get_theta(smo_sensorless_t *smo);
float smo_sensorless_get_omega(smo_sensorless_t *smo);

typedef enum
{
    ENCODER_TUNE,
    SENSORLESS_TUNE,
} TUNE_MODE_E;
// 参数整定状态
typedef enum
{
    PARAM_TUNE_IDLE = 0,
    PARAM_TUNE_THETA_OFFSET, // 角度偏移
    PARAM_TUNE_POLE_PAIRS,   // 极对数（首先）
    PARAM_TUNE_RS,           // 电阻
    PARAM_TUNE_LS,           // 电感
    PARAM_TUNE_FLUX,         // 磁链
    PARAM_TUNE_INERTIA,      // 惯量
    PARAM_TUNE_FRICTION,     // 摩擦
    PARAM_TUNE_COMPLETE      // 完成
} param_tune_state_t;

/*****************************************参数整定*********************************** */

// 参数整定结构体
typedef struct
{
    // 电机参数（可更新）
    float theta_offset;    // 角度偏移
    float Rs;              // 定子电阻
    float Ls;              // 定子电感
    float Psi_f;           // 永磁体磁链
    float pole_pairs;      // 极对数
    float J;               // 转动惯量
    float B;               // 摩擦系数
    float torque_constant; // 转矩常数
    // 控制参数
    float dt;   // 控制周期
    float k_sl; // SMO 滑模增益
    float k_f;  // 低通滤波增益

    // SMO 状态变量
    float i_alpha_hat;
    float i_beta_hat;
    float e_alpha;
    float e_beta;
    float e_alpha_filtered;
    float e_beta_filtered;

    // 参数整定状态
    TUNE_MODE_E tune_mode;
    param_tune_state_t tune_state;
    u32 tune_samples;

    // 用于参数计算的变量
    // float test_voltage_alpha; // 测试电压 α
    // float test_voltage_beta;  // 测试电压 β
    // float test_current_alpha; // 测试电流 α
    // float test_current_beta;  // 测试电流 β
    // float test_speed_mech;    // 测试机械速度

    // 历史数据
    // float i_history[100];
    // float v_history[100];
    // float speed_history[100];
    // u16 history_index;

    // 更新标志
    bool theta_offset_updated;
    bool Rs_updated;
    bool Ls_updated;
    bool Psi_f_updated;
    bool pole_pairs_updated;
    bool J_updated;
    bool B_updated;
    bool fault_flag; // 故障标志

} param_tuning_t;
// 初始化--无感需要准确的极对数
void param_tuning_init(param_tuning_t *smo,
                       float initial_Rs, float initial_Ls,
                       float initial_Psi_f, float initial_pole_pairs,
                       float dt);
// 开始整定
void sensorless_param_tuning_update(param_tuning_t *smo,
                                    float v_alpha, float v_beta,
                                    float i_alpha, float i_beta, float omega_electrical);
void encoder_param_tuning_update(param_tuning_t *smo,
                                 float v_alpha_applied, float v_beta_applied,
                                 float i_alpha, float i_beta,
                                 float theta_mechanical,
                                 float omega_mechanical);
bool tune_pole_pairs_correct(param_tuning_t *smo, float theta_mechanical);
bool tune_resistance_correct(param_tuning_t *smo, float v_alpha, float v_beta,
                             float i_alpha, float i_beta);
bool tune_inductance_correct(param_tuning_t *smo, float v_alpha, float v_beta,
                             float i_alpha, float i_beta);
bool tune_flux_correct(param_tuning_t *smo, float omega_electrical);
bool tune_inertia_correct(param_tuning_t *smo, float omega_mechanical,
                          float i_alpha, float i_beta, float theta_electrical);
bool tune_friction_correct(param_tuning_t *smo, float omega_mechanical,
                           float i_alpha, float i_beta);

param_tune_state_t param_tuning_get_state(param_tuning_t *smo);
float param_tuning_get_pole_pairs(param_tuning_t *smo);
float param_tuning_get_Rs(param_tuning_t *smo);
float param_tuning_get_Ls(param_tuning_t *smo);
float param_tuning_get_Psi_f(param_tuning_t *smo);
float param_tuning_get_J(param_tuning_t *smo);
float param_tuning_get_B(param_tuning_t *smo);
#endif