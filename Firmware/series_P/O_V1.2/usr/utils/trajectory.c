#include "trajectory.h"

//  === 常量定义 (放 Flash) === 
static const float TRAJ_EPSILON = 1.0e-6f;
static const float TRAJ_TOL_DEFAULT = 0.001f;

//  === 全局静态变量  === 
static tTraj_Config traj_cfg = {0};
static tTraj_PosState traj_posstate = {0};
static tTraj_VelState traj_velstate = {0};

//  === 初始化 === 
void traj_init(tTraj_Config cfg)
{
    traj_cfg = cfg;
}

//  === 重置状态 === 
void traj_posreset(float current_value)
{
    traj_posstate.target = current_value;
    traj_posstate.current = current_value;
    traj_posstate.rate = 0.0f;
    traj_posstate.accel = 0.0f;
    traj_posstate.busy = false;
}

void traj_velreset(float current_value)
{
    traj_velstate.target = current_value;
    traj_velstate.current = current_value;
    traj_velstate.accel = 0.0f;
    traj_velstate.jerk = 0.0f;
    traj_velstate.busy = false;
}

//  === 设置目标 === 
void traj_set_postarget(float target)
{
    traj_posstate.target = target;
    float err = FABSF(target - traj_posstate.current);
    traj_posstate.busy = (err > TRAJ_TOL_DEFAULT);
}

void traj_set_veltarget(float target)
{
    traj_velstate.target = target;
    float err = FABSF(target - traj_velstate.current);
    traj_velstate.busy = (err > TRAJ_TOL_DEFAULT);
}

//  === 核心更新函数  === 
tTraj_PosOut traj_PosUpdate(float dt)
{
    tTraj_PosOut out = {0};

    if (traj_cfg.type == TRAJ_DISABLE)
    {
        out.value = traj_posstate.target;
        out.rate = 0.0f;
        out.accel = 0.0f;
        out.done = true;
        return out;
    }

    //  === 1. 误差计算 === 
    float error = traj_posstate.target - traj_posstate.current;
    float dist = FABSF(error);
    float dir = FSIGN(error);

    //  === 2. 刹车距离计算 (rate² / 2a) === 
    float rate_abs = FABSF(traj_posstate.rate);
    float stop_dist = (traj_cfg.limit_d2 > TRAJ_EPSILON)
                          ? (rate_abs * rate_abs) / (2.0f * traj_cfg.limit_d2)
                          : 0.0f;
    //  === 3. 计算允许的最大变化率 === 
    float rate_limit;
    if (dist <= stop_dist)
    {
        //  减速段：使用 ARM 优化 sqrt 
        arm_sqrt_f32(2.0f * traj_cfg.limit_d2 * dist, &rate_limit);
    }
    else
    {
        rate_limit = traj_cfg.limit_d1;
    }

    //  === 4. 分支：T 型 vs S 型 === 
    if (traj_cfg.type == TRAJ_TRAPEZOID)
    {
        //  === 梯形轨迹 === 
        float target_rate = rate_limit * dir;
        float rate_diff = target_rate - traj_posstate.rate;
        float max_rate_step = traj_cfg.limit_d2 * dt;

        //  变化率斜坡 (分支消除) 
        float rate_step = (FABSF(rate_diff) > max_rate_step) ? FSIGN(rate_diff) * max_rate_step : rate_diff;

        traj_posstate.rate += rate_step;
    }
    else
    {
        //  === S 型轨迹 === 
        float target_rate = rate_limit * dir;
        float rate_err = target_rate - traj_posstate.rate;

        //  目标加速度 
        float target_accel = (dt > TRAJ_EPSILON) ? (rate_err / dt) : 0.0f;
        target_accel = CLAMP(target_accel, -traj_cfg.limit_d2, traj_cfg.limit_d2);

        //  Jerk 限制 
        float accel_err = target_accel - traj_posstate.accel;
        float max_accel_step = traj_cfg.limit_d3 * dt;

        float accel_step = (FABSF(accel_err) > max_accel_step) ? FSIGN(accel_err) * max_accel_step : accel_err;

        traj_posstate.accel += accel_step;
        traj_posstate.rate += traj_posstate.accel * dt;
    }

    //  === 5. 积分得到规划值 === 
    traj_posstate.current += traj_posstate.rate * dt;

    //  === 6. 输出赋值 === 
     out.value = traj_posstate.current; // 核心输出
     out.rate = traj_posstate.rate;     // 可选：变化率前馈
     out.accel = traj_posstate.accel;   // 可选：加速度前馈
    //  === 7. 到达判断 === 
    float tol = (traj_cfg.tolerance > 0.0f) ? traj_cfg.tolerance : TRAJ_TOL_DEFAULT;
    bool done = (dist <= tol) && (FABSF(traj_posstate.rate) <= tol);

    if (done)
    {
         traj_posstate.current = traj_posstate.target; // 修正累积误差
        traj_posstate.rate = 0.0f;
        traj_posstate.accel = 0.0f;
        traj_posstate.busy = false;
        out.done = true;
    }
    else
    {
        traj_posstate.busy = true;
        out.done = false;
    }

    return out;
}

tTraj_VelOut traj_VelUpdate(float dt)
{
    tTraj_VelOut out = {0};

    if (traj_cfg.type == TRAJ_DISABLE)
    {
        out.value = traj_velstate.target;
        out.accel = 0.0f;
        out.jerk = 0.0f;
        out.done = true;
        return out;
    }

    // 1. 速度误差
    float vel_err = traj_velstate.target - traj_velstate.current;
    float vel_err_abs = FABSF(vel_err);

    // 2. 到达判定
    float tol = (traj_cfg.tolerance > 0.0f) ? traj_cfg.tolerance : TRAJ_TOL_DEFAULT;
    if (vel_err_abs <= tol && FABSF(traj_velstate.accel) <= tol)
    {
        traj_velstate.current = traj_velstate.target;
        traj_velstate.accel = 0.0f;
        traj_velstate.jerk = 0.0f;
        traj_velstate.busy = false;
        out.value = traj_velstate.current;
        out.accel = 0.0f;
        out.jerk = 0.0f;
        out.done = true;
        return out;
    }

    // 3. 速度域刹车距离 (对偶位置模式: a²/(2*limit_d2))
    float accel_abs = FABSF(traj_velstate.accel);
    float stop_vel = (traj_cfg.limit_d2 > TRAJ_EPSILON)
                         ? (accel_abs * accel_abs) / (2.0f * traj_cfg.limit_d2)
                         : 0.0f;

    // 4. 计算目标加速度
    float max_accel = traj_cfg.limit_d1; // 速度模式下 limit_d1 = 最大加速度
    float target_accel = 0.0f;

    if (vel_err_abs <= stop_vel)
    {
        // 进入刹车区：撤加速度
        target_accel = 0.0f;
    }
    else
    {
        // 正常加速区
        float needed_accel = vel_err / dt;
        target_accel = CLAMP(needed_accel, -max_accel, max_accel);
    }

    // 5. 梯形 vs S型
    if (traj_cfg.type == TRAJ_TRAPEZOID)
    {
        // 梯形：无加加速度限制，加速度直接跳变
        traj_velstate.accel = target_accel;
        traj_velstate.jerk = 0.0f;
    }
    else
    {
        // S型：用 limit_d2 限制加加速度 (速度模式下 limit_d2 = 最大加加速度)
        float accel_err = target_accel - traj_velstate.accel;
        float max_accel_step = traj_cfg.limit_d2 * dt; // 本周期最大加速度变化量
        float accel_step = (FABSF(accel_err) > max_accel_step)
                               ? FSIGN(accel_err) * max_accel_step
                               : accel_err;

        traj_velstate.accel += accel_step;
        traj_velstate.jerk = accel_step / dt;
    }

    // 6. 更新速度
    traj_velstate.current += traj_velstate.accel * dt;

    // 7. 离散化防过冲钳位
    if ((vel_err > 0 && traj_velstate.current > traj_velstate.target) ||
        (vel_err < 0 && traj_velstate.current < traj_velstate.target))
    {
        traj_velstate.current = traj_velstate.target;
        traj_velstate.accel = 0.0f;
        traj_velstate.jerk = 0.0f;
    }

    // 8. 输出
    out.value = traj_velstate.current;
    out.accel = traj_velstate.accel;
    out.jerk = traj_velstate.jerk;

    // 9. 到达判断
    float err_now = FABSF(traj_velstate.target - traj_velstate.current);
    bool done = (err_now <= tol) && (FABSF(traj_velstate.accel) <= tol);
    if (done)
    {
        traj_velstate.current = traj_velstate.target;
        traj_velstate.accel = 0.0f;
        traj_velstate.jerk = 0.0f;
        traj_velstate.busy = false;
        out.done = true;
    }
    else
    {
        traj_velstate.busy = true;
        out.done = false;
    }

    return out;
}