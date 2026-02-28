#ifndef __SMO_H
#define __SMO_H

#include "main.h"
#include "foc_core.h"
/*****************************************无感SMO观测*********************************** */
// 无感 SMO 结构体
typedef struct
{
    u8 pole_pairs;       // 极对数
    float theta_offset;  // 角度偏移
    short wire_sequence; // 线序
    float Rs;            // 定子电阻
    float Ls;            // 定子电感
    float Psi_f;         // 永磁体磁链
    float Ke;            // 反电动势常数
    float J;             // 转动惯量
    float B;             // 摩擦系数

    float dt;  // 控制周期
    float Udc; // 电压
    // 状态变量
    float i_alpha_hat;
    float i_beta_hat;
    float e_alpha;
    float e_beta;
    float e_alpha_filtered;
    float e_beta_filtered;
    float theta;
    float theta_prev;
    float omega;

    bool is_aligned;         // 是否完成初始对齐
    uint32_t alignment_time; // 对齐时间计数
    float startup_gain;      // 启动增益（从0逐渐增加）

    float integrator_alpha; // 电流观测器积分增益
    float integrator_limit; // 积分器限幅

    // 参数
    float k_sl;
    float delta;
    float k_f;
    float max_omega;
} tSMO;
extern tSMO smo;
// 初始化
void fSMO_Init(tMotor motor);
void fSMO_Reset();
// 更新（输入：电压、电流）
void fSMO_MainLoop(float v_alpha, float v_beta,
                   float i_alpha, float i_beta);

// 获取结果
float fSMO_GetTheta();
float fSMO_GetOmega();

// 参数整定状态
typedef enum
{
    PARAM_TUNE_IDLE = 0,
    PARAM_TUNE_THETA_OFFSET, // 角度偏移
    PARAM_TUNE_WireS,        // 线序整定
    PARAM_TUNE_RS,           // 电阻
    PARAM_TUNE_LS,           // 电感
    PARAM_TUNE_POLE_PAIRS,   // 极对数
    PARAM_TUNE_PK,           // 磁链\反电动势常数
    PARAM_TUNE_JB,           // 转动惯量，摩擦系数
    PARAM_TUNE_WRITE_FLASH,  // 写入参数
    PARAM_TUNE_COMPLETE      // 完成

} eParameterTuneStatus;
typedef enum
{
    PARAM_FAULT_NONE = 0, // 无故障

    // 通用故障
    PARAM_FAULT_TIMEOUT,   // 超时
    PARAM_FAULT_LOW,       // 过低
    PARAM_FAULT_HIGH,      // 过高
    PARAM_FAULT_UNBALANCE, // 不平衡

    PARAM_FAULT_WS_LOCKED,           // 电机堵转
    PARAM_FAULT_POLE_PAIRS_INVALID,  // 极对数无效
    PARAM_FAULT_POLE_PAIRS_MISMATCH, // 极对数不匹配
} eParamterTuneFault;

/*****************************************参数整定*********************************** */

// 参数整定结构体
typedef struct
{
    // 参数整定状态
    eParameterTuneStatus tune_state;

    u8 num_test_wire;

    u32 time_tic;
    u32 tune_samples;
    u32 steady_samples;

    float theta_elec_prev;
    float cur_iq_id[2];
    float cur_uq_ud[2];
    float ualpha, ubeta;

    float omega_ref;
    float steady_i;
    float steady_v;

    bool inject_flag;
    bool alpha_beta_flag;
    float L_alpha;
    float L_beta;
    float sum_di_dt_alpha_pos;
    u32 alpha_pos_count;
    float sum_di_dt_alpha_neg;
    u32 alpha_neg_count;
    float sum_di_dt_beta_pos;
    u32 beta_pos_count;
    float sum_di_dt_beta_neg;
    u32 beta_neg_count;

    float omega_mech_prev;
    float theta_elec_con;

    float sum_e_mag;
    float sum_speed;

    bool start_smp_flag;
    float sum_accel;
    float sum_iq;

    // 故障标志
    bool fault_flag;
    eParamterTuneFault fault_type;
    eParameterTuneStatus fault_state;
} tParameterTune;
extern tParameterTune tun;

void fParamTuneReset();
// 开始整定

eParameterTuneStatus fParamTuneUpdate(tFOC_val foc_value);

#endif