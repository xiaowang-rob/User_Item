#ifndef __TUNE_H
#define __TUNE_H

#include "bsp.h"
#include "foc_core.h"
#include "usr_config.h"
#include "protocol_defs.h"

/* ================================= 整定参数配置 ================================= */
// 通用时间转换 (假设 20kHz 中断，1 tick = 50us)

#define TICK_TO_MS(tick) ((tick) * 0.05f)
#define MS_TO_TICK(ms) ((u16)((ms) * 20.0f))

#define TUNE_WAIT_TICKS MS_TO_TICK(1000) // 先静止等待时间

// 电阻整定 (滞环电流控制 + 滤波 + 差分)
#define RS_FREQ_F 10                      // 电阻整定分频系数
#define RS_TIMEOUT_TICKS MS_TO_TICK(2000) // 整定超时时间 (500ms)
#define RS_I_TARGET_1 3.0f                // 第一点目标电流 (A)
#define RS_I_TARGET_2 7.0f                // 第二点目标电流 (A)
#define RS_HYST_BAND 2.0f                 // 滞环带宽 ±1.5A
#define RS_V_LIMIT 2.0f                   // 电压输出限幅 (V)
#define RS_V_HOLD_MAX_TICKS 5             // 误差带内保持最大周期数 (防静差)
#define RS_V_STEP_MIN 0.01f               // 保持超时后微调步长 (V)
#define RS_STEADY_ERR_THR 0.4f            // 稳态电流误差阈值 (A)
#define RS_STEADY_TICKS MS_TO_TICK(7)     // 稳态持续周期数 (7ms@20kHz)
#define RS_TRACK_TIMEOUT MS_TO_TICK(40)   // 单点跟踪超时 (40ms)
#define RS_MIN_DELTA_I 2.0f               // 最小电流变化量 (A)
#define RS_RANGE_MIN 0.02f                // 电阻合理下限 (Ω)
#define RS_RANGE_MAX 0.5f                 // 电阻合理上限 (Ω)
#define RS_DEADTIME_VCOMP 0.04f           // 死区补偿电压 (V)

// 电感整定
#define LS_TIMEOUT_TICKS MS_TO_TICK(500)                // 整定超时时间 (500ms)
#define LS_INJECT_FREQ_TICK 20                          // 注入分频
#define LS_INJECT_AMP_V (LS_INJECT_FREQ_TICK * T_PWM)   // 注入周期
#define LS_INJECT_FREQ_HZ (F_PWM / LS_INJECT_FREQ_TICK) // 注入频率 (Hz)

#define LS_V_START 0.8f     // 起始电压 (V)
#define LS_V_MAX 2.0f       // 最大电压 (V)
#define LS_I_TARGET 3.0f    // 校准电流 (A)
#define LS_I_STEP_MIN 0.04f // 保持超时后微调步长 (A)

// 转子预定位
#define ALIGN_TIME_MS 500       // 定位持续时间(ms)
#define WAIT_AFTER_ALIGN_MS 200 // 定位后等待电流衰减时间(ms)

// DFT测量
#define DFT_AVG_CYCLES 20 // 取10个注入周期的平均

#define LS_MIN_DI_DT 100.0f   // 最小信噪比要求
#define LS_MAX_DI_DT 60000.0f // 最大信噪比要求
#define LS_RANGE_MIN 20e-6f   // 电感合理下限(H)
#define LS_RANGE_MAX 300e-6f  // 电感合理上限 (H)

// 编码器校准
#define EC_FREQ_F 10                // 编码器校准分频系数
#define EC_ALIGN_ms MS_TO_TICK(300) // 编码器校准等待时间
#define EC_OPEN_LOOP_OMEGA 1000.0f  // 开环角速度 (°/s) 对应 7极对数 23rpm 14极对数 12rpm

#define EC_OPEN_LOOP_UQ_MIN 0.4f  // 起始最小施加uq
#define EC_OPEN_LOOP_UQ_MAX 2.0f  // 起始最大施加uq
#define EC_OPEN_LOOP_UQ_STEP 0.2f // 施加uq步长

#define EC_FIT_MAX_ERROR 100.0f // 最大拟合误差
#define EC_MIN_POLE_PAIRS 1     // 最小极对数
#define EC_MAX_POLE_PAIRS 16    // 最大极对数

// 角度偏移 (开环强励磁)
#define THETA_TIMEOUT_TICKS MS_TO_TICK(1000) // 整定超时时间 (1000ms)
#define THETA_VOLT_AMP 0.6f                  // 强励磁电压幅值 (V)
#define THETA_DELTA_MAX 0.05f                // 静止判断阈值 (°)
#define THETA_STEADY_WIN MS_TO_TICK(100)     // 静止等待时间 (100ms)
/* ================================= 电机参数结构(独立存储) ================================= */
typedef struct
{
    // 系统参数
    float dt; // 控制周期 (s)

    // 控制参数

    float BandWidth_Current; // 电流滞环带宽
    float BandWidth_Speed;   // 速度滞环带宽
    float BandWidth_Pos;     // 位置滞环带宽

    float uadc_offset; // adc 偏移
    float vadc_offset;
    float wadc_offset;

    float cur_fiter_alpha; // 电流滤波系数

    float iq_Kp;
    float iq_Ki;
    float id_kp;
    float id_ki;
    float speed_Kp;
    float speed_Ki;
    float mag_Kp;
    float mag_Ki;
    float pos_Kp;
    float pos_Ki;
    float pos_Kd;

    /* ================================= 电机参数 ========== */
    // 电气参数
    float KV; // 电压转化器增益 (V/V) 用于检验参数有效性

    float Rs;    // 定子电阻(Ω)
    float Ld;    // d 轴电感 (H)
    float Lq;    // q 轴电感 (H)
    float Psi_f; // 永磁体磁链 (Wb)
    float Ke;    // 反电动势常数 (V/(rad/s))

    // 机械参数
    float J; // 转动惯量 (kg·m²)
    float B; // 摩擦系数 (N·m·s/rad)

    /* ========== 编码器参数 =====*/
    float theta_offset; // 编码器角度偏移 (rad)
    u8 pole_pairs;      // 极对数
    bool direction;     // 转动方向 (true:逆 false 顺)
    bool theta_elec_need_180;

} tTuneParams;

/* ================================= 整定状态枚举 ================================= */

typedef enum
{
    TUNE_FAULT_NONE = 0,
    TUNE_FAULT_CURRENT_VIBRATION,  // 电流震荡,不稳定
    TUNE_FAULT_POLEPAIRS_MISMATCH, // 极对数不匹配,校准失败
    TUNE_FAULT_MECH_LOCKED,        // 电机堵转

    TUNE_FAULT_RSLS_INVALID,    // 电阻电感校准失败
    TUNE_FAULT_ENCODER_INVALID, // 编码器校准失败
    TUNE_FAULT_ELECTRI_INVALID, // 电气参数校准失败
    TUNE_FAULT_MECH_INVALID,    // 机械参数校准失败

} eTuneFault;

/* ================================= 整定上下文(内部状态) ================================= */
typedef struct
{
    // 通用状态
    eTuneState state;
    eTuneFault fault;
    u8 freq_tick;     // 频率计数
    u32 steady_tick;  // 稳态计数
    u32 timeout_tick; // 超时计数

    // 电阻整定上下文
    struct
    {
        float i_target;
        float v_meas[2];
        float i_meas[2];
        float v_cmd;
        u16 hold_cnt;
        u16 step_ticks;
        u8 step;
    } rs_ctx;

    // 电感整定上下文
    struct
    {
        // 状态机 (0~5)
        uint8_t state;
        bool ready;

        // ----- 电压自适应相关 -----
        float v_inj;    // 最终确定的注入电压幅值
        float i_target; // 目标电流幅值
        float i_meas;   // 当前测量的电流幅值（用于自适应判断）

        // ----- DFT 累加器（无缓冲区，周期累加）-----
        float sum_re;        // 实部累加和
        float sum_im;        // 虚部累加和
        uint16_t sample_cnt; // 当前周期已采样点数

        // ----- 多周期平均 -----
        float amp_sum;     // 多个周期的幅值累加
        uint8_t cycle_cnt; // 已完成的有效周期数

    } ls_ctx;

    // 角度偏移上下文
    struct
    {
        float theta_elec;
        float v_out;

        float theta_e_acc;    // 连续电角度
        float theta_e_raw;    // 上一次电角度
        float theta_m_unwrap; // 解包后的连续机械角度
        float theta_m_start;  // 起始机械角度
        float delta_theta[2];

        float k[2];
        float b[2];
        float err[2];

        float sum_m[2];  // Σθ_m
        float sum_e[2];  // Σθ_e
        float sum_me[2]; // Σ(θ_m·θ_e)
        float sum_mm[2]; // Σ(θ_m²)

        u16 cnt[2]; // 采样点数

        bool forward_done;
        bool backward_done;
        u8 test_step;
        u8 step; // 0对齐 1正向 2反向 3拟合计算 4完成

    } encoder_ctx;

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
    bool temp_flag[4];
} tTuneContext;

/* ================================= 全局变量声明 ================================= */
extern tTuneContext g_tune_ctx;

/* ================================= 公共接口 ================================= */
void fMotorParamTune_Init();
void fMotorParamTune_Reset();
eTuneState fMotorParamTune_Update(tFOC_val foc_val);
u8 fMotorParamTune_GetProgress(void);
eTuneFault fMotorParamTune_GetFault(void);

#endif /* __TUNE_H */