#ifndef __SMO_H
#define __SMO_H

#include "main.h"
/*****************************************无感SMO观测*********************************** */
// 无感 SMO 结构体
typedef struct
{
    float theta_offset; // 角度偏移
    u8 pole_pairs;      // 极对数
    float Rs;           // 定子电阻
    float Ls;           // 定子电感
    float Psi_f;        // 永磁体磁链
    float Ke;           // 反电动势常数
    float J;            // 转动惯量
    float B;            // 摩擦系数

    float dt;    // 控制周期
    float k_sl;  // 滑模增益
    float k_f;   // 滤波器增益
    float delta; // 边界层厚度
    float max_omega;
    float speed_filter_gain; // 速度滤波增益

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
} smo_t;
smo_t *get_smo_adr();
// 初始化
void smo_init(float Rs, float Ls, float Psi_f, float max_speed, float pole_pairs,
              float Ke, float J, float B);
void smo_reset();
// 更新（输入：电压、电流）
void smo_update(float v_alpha, float v_beta,
                float i_alpha, float i_beta);

// 获取结果
float smo_get_theta();
float smo_get_omega();

// 参数整定状态
typedef enum
{
    PARAM_TUNE_IDLE = 0,
    PARAM_TUNE_THETA_OFFSET, // 角度偏移
    PARAM_TUNE_RS,           // 电阻
    PARAM_TUNE_LS,           // 电感
    PARAM_TUNE_POLE_PAIRS,   // 极对数
    PARAM_TUNE_PK,           // 磁链\反电动势常数
    PARAM_TUNE_JB,           // 转动惯量，摩擦系数
    PARAM_TUNE_COMPLETE      // 完成
} param_tune_state_t;
typedef enum
{
    PARAM_FAULT_NONE = 0, // 无故障

    // 通用故障
    PARAM_FAULT_TIMEOUT,   // 超时
    PARAM_FAULT_LOW,       // 过低
    PARAM_FAULT_HIGH,      // 过高
    PARAM_FAULT_UNBALANCE, // 不平衡

    PARAM_FAULT_POLE_PAIRS_INVALID,  // 极对数无效
    PARAM_FAULT_POLE_PAIRS_MISMATCH, // 极对数不匹配
} param_fault_t;

/*****************************************参数整定*********************************** */

// 参数整定结构体
typedef struct
{
    float dt;
    // 参数整定状态
    param_tune_state_t tune_state;

    u32 time_tic;
    u32 tune_samples;

    float Udc; // 电压
    float theta_mech_prev;

    float steady_i;
    float steady_v;

    bool inject_flag;
    float sum_di_dt_alpha_pos;
    u32 alpha_pos_count;
    float sum_di_dt_alpha_neg;
    u32 alpha_neg_count;
    float sum_di_dt_beta_pos;
    u32 beta_pos_count;
    float sum_di_dt_beta_neg;
    u32 beta_neg_count;

    float theta_elec_con;
    // 故障标志
    bool fault_flag;
    param_fault_t fault_type;
    param_tune_state_t fault_state;
} param_tuning_t;
param_tuning_t *get_tuning_adr();
// 初始化--无感需要准确的极对数
void param_tuning_init(float initial_Rs, float initial_Ls,
                       float initial_Psi_f, float initial_pole_pairs);
// 开始整定
void sensorless_param_tuning_update(float v_alpha, float v_beta,
                                    float i_alpha, float i_beta, float omega_electrical);
void encoder_param_tuning_update(
    float v_alpha_applied, float v_beta_applied,
    float i_alpha, float i_beta,
    float theta_mechanical,
    float omega_mechanical);
bool tune_pole_pairs_correct(float theta_mechanical);
bool tune_resistance_correct(float v_alpha, float v_beta,
                             float i_alpha, float i_beta);
bool tune_inductance_correct(float v_alpha, float v_beta,
                             float i_alpha, float i_beta);
bool tune_flux_correct(float omega_electrical);
bool tune_inertia_correct(float omega_mechanical,
                          float i_alpha, float i_beta, float theta_electrical);
bool tune_friction_correct(float omega_mechanical,
                           float i_alpha, float i_beta);

param_tune_state_t param_tuning_get_state(param_tuning_t *smo);

#endif