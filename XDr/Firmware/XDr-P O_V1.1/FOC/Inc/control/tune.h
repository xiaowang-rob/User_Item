#ifndef __TUNE_H
#define __TUNE_H

#include "main.h"
#include "foc_core.h"

/* ================================= 电机参数结构(独立存储) ================================= */
typedef struct
{
    // 电气参数
    float KV;    // 电压转化器增益 (V/V)
    float Rs;    // 定子电阻(Ω)
    float Ld;    // d 轴电感 (H)
    float Lq;    // q 轴电感 (H)
    float Psi_f; // 永磁体磁链 (Wb)
    float Ke;    // 反电动势常数 (V/(rad/s))

    // 机械参数
    float J; // 转动惯量 (kg·m²)
    float B; // 摩擦系数 (N·m·s/rad)

    // 配置参数
    u8 pole_pairs;       // 极对数
    short wire_sequence; // 线序：1=正序，-1=反序
    float theta_offset;  // 编码器角度偏移 (rad)

    // 系统参数
    float Udc; // 母线电压 (V)
    float dt;  // 控制周期 (s)

    // 有效性标志
    bool Rs_valid;
    bool L_valid;
    bool offset_valid;
    bool wire_valid;
    bool pole_valid;
    bool psi_valid;
    bool mech_valid;
} tMotorParams;

/* ================================= 整定状态枚举 ================================= */
typedef enum
{
    TUNE_STATE_IDLE = 0,
    TUNE_STATE_RS,
    TUNE_STATE_LS,
    TUNE_STATE_THETA_OFFSET,
    TUNE_STATE_WIRE_SEQ,
    TUNE_STATE_POLE_PAIRS,
    TUNE_STATE_PSI_F,
    TUNE_STATE_JB,
    TUNE_STATE_COMPLETE,
    TUNE_STATE_FAULT
} eTuneState;

typedef enum
{
    TUNE_FAULT_NONE = 0,
    TUNE_FAULT_TIMEOUT,
    TUNE_FAULT_OVERCURRENT,
    TUNE_FAULT_PARAM_INVALID,
    TUNE_FAULT_SIGNAL_WEAK,
    TUNE_FAULT_MECH_LOCKED,
    TUNE_FAULT_ENCODER_ERROR
} eTuneFault;

/* ================================= 整定上下文(内部状态) ================================= */
typedef struct
{
    // 通用状态
    eTuneState state;
    eTuneFault fault;
    u32 tick_count;
    u32 timeout_tick;

    // 电阻整定上下文
    struct
    {
        float target_i[2];
        float v_meas[2];
        float i_meas[2];
        u16 sample_cnt[2];
        u8 step;
        float v_cmd_last;
    } rs_ctx;

    // 电感整定上下文
    struct
    {
        float v_inj;
        float di_dt_sum[2][2];
        u16 cnt[2][2];
        float i_peak[2][2];
        bool axis;
        u16 inject_period;
        u16 inject_cnt;
    } ls_ctx;

    // 角度偏移上下文
    struct
    {
        float theta_sum;
        u16 valid_cnt;
        float offset_hist[3];
        u8 hist_idx;
        bool steady_flag;
    } theta_ctx;

    // 线序上下文
    struct
    {
        u8 step;
        u32 step_start_tick;
        u32 wait_ticks;
        s32 enc_start;
        s32 enc_delta_fwd;
        s32 enc_delta_rev;
        u8 test_iter;
        bool enc_ready;
    } wire_ctx;

    // 极对数上下文
    struct
    {
        float omega_ref;
        float sum_ratio;
        u16 valid_cnt;
        bool steady_flag;
    } pole_ctx;

    // 磁链上下文
    struct
    {
        float sum_e_mag;
        float sum_omega;
        u16 valid_cnt;
        bool ready;
    } psi_ctx;

    // 机械参数上下文
    struct
    {
        float omega_start;
        float sum_torque;
        float sum_accel;
        u16 sample_cnt;
        bool accel_phase;
        bool ready;
    } jb_ctx;

    // 临时变量
    float temp_val[4];
    u16 temp_cnt[4];
    bool temp_flag[4];
} tTuneContext;

/* ================================= 全局变量声明 ================================= */
extern tTuneContext g_tune_ctx;

/* ================================= 公共接口 ================================= */
void fMotorParamTune_Init();
void fMotorParamTune_Reset(void);
eTuneState fMotorParamTune_Update(tFOC_val foc_val);
u8 fMotorParamTune_GetProgress(void);
eTuneFault fMotorParamTune_GetFault(void);
void fMotorParamTune_ForceSave(void);

#endif /* __TUNE_H */