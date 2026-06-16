#include "trajectory.h"

/* === 常量定义 (放 Flash) === */
static const float TRAJ_EPSILON = 1.0e-6f;
static const float TRAJ_TOL_DEFAULT = 0.001f;

/* === 全局静态变量  === */
static tTraj_Config traj_cfg = {0};
static tTraj_State traj_state = {0};

/* === 初始化 === */
void traj_init(tTraj_Config cfg)
{
    traj_cfg = cfg;
}

/* === 重置状态 === */
void traj_reset(float current_value)
{
    traj_state.target = current_value;
    traj_state.current = current_value;
    traj_state.rate = 0.0f;
    traj_state.accel = 0.0f;
    traj_state.busy = false;
}

/* === 设置目标 === */
void traj_set_target(float target)
{
    traj_state.target = target;
    float err = FABSF(target - traj_state.current);
    traj_state.busy = (err > TRAJ_TOL_DEFAULT);
}

void traj_set_rate(float rate)
{
    traj_state.rate = rate;
}
/* === 核心更新函数 (保持你的结构) === */
tTraj_Out fTraj_Update(float dt)
{
    tTraj_Out out = {0};

    if (traj_cfg.type == TRAJ_DISABLE)
    {
        out.value = traj_state.target;
        out.rate = 0.0f;
        out.done = true;
        return out;
    }

    /* === 1. 误差计算 === */
    float error = traj_state.target - traj_state.current;
    float dist = FABSF(error);
    float dir = FSIGN(error);

    /* === 2. 刹车距离计算 (rate² / 2a) === */
    float rate_abs = FABSF(traj_state.rate);
    float stop_dist = (traj_cfg.max_acc > TRAJ_EPSILON)
                          ? (rate_abs * rate_abs) / (2.0f * traj_cfg.max_acc)
                          : 0.0f;
    /* === 3. 计算允许的最大变化率 === */
    float rate_limit;
    if (dist <= stop_dist)
    {
        /* 减速段：使用 ARM 优化 sqrt */
        arm_sqrt_f32(2.0f * traj_cfg.max_acc * dist, &rate_limit);
    }
    else
    {
        rate_limit = traj_cfg.max_rate;
    }

    /* === 4. 分支：T 型 vs S 型 === */
    if (traj_cfg.type == TRAJ_TRAPEZOID)
    {
        /* === 梯形轨迹 === */
        float target_rate = rate_limit * dir;
        float rate_diff = target_rate - traj_state.rate;
        float max_rate_step = traj_cfg.max_acc * dt;

        /* 变化率斜坡 (分支消除) */
        float rate_step = (FABSF(rate_diff) > max_rate_step) ? FSIGN(rate_diff) * max_rate_step : rate_diff;

        traj_state.rate += rate_step;
    }
    else
    {
        /* === S 型轨迹 === */
        float target_rate = rate_limit * dir;
        float rate_err = target_rate - traj_state.rate;

        /* 目标加速度 */
        float target_accel = (dt > TRAJ_EPSILON) ? (rate_err / dt) : 0.0f;
        target_accel = CLAMP(target_accel, -traj_cfg.max_acc, traj_cfg.max_acc);

        /* Jerk 限制 */
        float accel_err = target_accel - traj_state.accel;
        float max_accel_step = traj_cfg.max_jerk * dt;

        float accel_step = (FABSF(accel_err) > max_accel_step) ? FSIGN(accel_err) * max_accel_step : accel_err;

        traj_state.accel += accel_step;
        traj_state.rate += traj_state.accel * dt;
    }

    /* === 5. 积分得到规划值 === */
    traj_state.current += traj_state.rate * dt;

    /* === 6. 输出赋值 === */
    out.value = traj_state.current; // 核心输出
    out.rate = traj_state.rate;     // 可选：变化率前馈

    /* === 7. 到达判断 === */
    float tol = (traj_cfg.tolerance > 0.0f) ? traj_cfg.tolerance : TRAJ_TOL_DEFAULT;
    bool done = (dist <= tol) && (FABSF(traj_state.rate) <= tol);

    if (done)
    {
        traj_state.current = traj_state.target; // 修正累积误差
        traj_state.rate = 0.0f;
        traj_state.accel = 0.0f;
        traj_state.busy = false;
        out.done = true;
    }
    else
    {
        traj_state.busy = true;
        out.done = false;
    }

    return out;
}