#ifndef __TRAJECTORY_H
#define __TRAJECTORY_H

#include "math_fast.h"
#include "stdbool.h"
#include "arm_math.h"
#include "protocol.h"
/* === 使用 float 32位精度 确保 FPU 优化 === */

/* === 配置参数 === */
typedef struct
{
    float limit_d1;  // 一阶限幅 [unit/s]
    float limit_d2;  // 二阶限幅 [unit/s²]
    float limit_d3;  // 三阶限幅 [unit/s³] (仅 S 型有效)
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
} tTraj_PosState;

typedef struct
{
    float target;  // 目标值
    float current; // 当前规划值 (输出值)
    float accel;   // 当前加速度
    float jerk;    // 当前加加速度 (仅 S 型需要)
    bool busy;
    uint8_t reserved[3];
} tTraj_VelState;

/* === 输出结果 === */
typedef struct
{
    float value; // 核心输出：平滑后的值
    float rate;  // 当前变化率 (用于前馈)
    float accel; // 当前加速度 (仅 S 型有输出) (用于前馈)
    bool done;   // 到达标志
    uint8_t reserved[3];
} tTraj_PosOut;

typedef struct
{
    float value; // 核心输出：平滑后的值
    float accel; // 当前加速度 (用于前馈)
    float jerk;  // 当前加加速度 (仅 S 型有输出) (用于前馈)(一般用不上 除了更高阶的控制)
    bool done;   // 到达标志
    uint8_t reserved[3];
} tTraj_VelOut;

/* === 核心 API  === */
void traj_init(tTraj_Config cfg);
void traj_posreset(float current_value);
void traj_velreset(float current_value);
void traj_set_postarget(float target);
void traj_set_veltarget(float target);
tTraj_PosOut traj_PosUpdate(float dt);
tTraj_VelOut traj_VelUpdate(float dt);

#endif /* __TRAJECTORY_H */