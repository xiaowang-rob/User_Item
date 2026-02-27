#ifndef __TRAJECTORY_H
#define __TRAJECTORY_H

#include "math_fast.h"
#include "stdbool.h"
#include "arm_math.h"

/* === 使用 float 32位精度 确保 FPU 优化 === */

/* === 轨迹类型枚举 === */
typedef enum
{
    TRAJ_TYPE_DISABLE, // 禁用
    TRAJ_TYPE_TRAP,    // 梯形
    TRAJ_TYPE_SCURVE   // S 型
} eTrajType;

/* === 配置参数 === */
typedef struct
{
    float max_rate;  // 最大变化率 [unit/s]
    float max_acc;   // 最大加速度 [unit/s²]
    float max_jerk;  // 最大加加速度 [unit/s³] (仅 S 型有效)
    float tolerance; // 到达容差 [unit]
    eTrajType type;
    uint8_t reserved[3];
} tTraj_Config;

/* === 运行状态 (全局静态) === */
typedef struct
{
    float target;  // 目标值
    float current; // 当前规划值 (输出值)
    float rate;    // 当前变化率
    float accel;   // 当前加速度 (仅 S 型需要)
    bool busy;
    uint8_t reserved[3];
} tTraj_State;

/* === 输出结果 === */
typedef struct
{
    float value; // 核心输出：平滑后的值
    float rate;  // 当前变化率 (可选，用于前馈)
    bool done;   // 到达标志
    uint8_t reserved[3];
} tTraj_Out;

/* === 内联优化函数 (保持你的命名) === */
__STATIC_FORCEINLINE float traj_abs(float x)
{
    return __builtin_fabsf(x);
}

__STATIC_FORCEINLINE float traj_sign(float x)
{
    return (x > 0.0f) - (x < 0.0f); // 分支消除
}

__STATIC_FORCEINLINE float traj_clamp(float x, float min, float max)
{
    return (x < min) ? min : ((x > max) ? max : x);
}

/* === 核心 API (保持你的函数签名) === */
void fTraj_Init(tTraj_Config cfg);
void fTraj_Reset(float current_value);
void fTraj_SetTarget(float target);
void fTraj_SetRate(float rate);
tTraj_Out fTraj_Update(float dt);

#endif /* __TRAJECTORY_H */