#include "foc_main.h"
#include "svpwm.h"

// 开环启动参数
#define OL_START_LOCK_MS    200    // 锁定时间 [ms]
#define OL_START_RAMP_MS    500    // 斜坡时间 [ms]
#define OL_START_RPM        200.0f // 开环目标转速 [rpm]
#define OL_START_CURRENT    1.0f   // 开环电流 [A]
#include "math_fast.h"
#include "parameter_manager.h"
#include "device.h"
#include "smo.h"

#include "bsp_adc.h"
#include "svpwm.h"

// 2-shunt 电流采样：在上溢中断中调用
extern tSvpwm g_svpwm;
extern tFOC_val g_foc_val;
void BSP_CurrentSampleISR(void)
{
    BSP_SampleCurrent2Shunt(g_svpwm.sector, &g_foc_val.ialpha, &g_foc_val.ibeta);
}

FOC_t g_foc = {.core = &g_foc_core, .tun = &g_tune_ctx};

#ifdef __DEBUG__
u32 _time_focit_start = 0;
u32 _time_focit_end = 0;
volatile u32 _time_focit_run = 0;
volatile u32 _time_foc_T = 0;

#endif

// 由于tim8互补波输出的原因 计时器模式被强制改为中心对齐3 导致update回调被重复调用
// 下溢中断
void BSP_FOC_ITCallback()
{
#ifdef __DEBUG__
    _time_focit_start = BSP_GetTick_us();
#endif
    fFocStateMachineMainLoop();
#ifdef __DEBUG__
    _time_foc_T = BSP_GetTick_us() - _time_focit_end;
    _time_focit_end = BSP_GetTick_us();
    _time_focit_run = _time_focit_end - _time_focit_start;
#endif
}
// FOC 主初始化函数
void fFocInit()
{
    g_foc.core->foc_mode->run_mode = OPEN_LOOP;
    g_foc.foc_enable = false;
    g_foc.state = FOC_IDLE;
    foc_core_init();
    BSP_POWER_12V_Control(true);
    g_foc.foc_init = true;
}

// FOC 主循环函数
void fFocStateMachineMainLoop()
{
    if (!g_foc.foc_init)
        return;
    foc_value_update();
    switch (g_foc.state)
    {
    case FOC_IDLE:
        BSP_AdcIdleTrack();
        break;
    case FOC_TUNE:
        if (!g_foc.foc_enable)
        {
            g_foc.foc_enable = true;
            foc_core_reset();
            BSP_PWM_Enable();
        }
        if (fAutoCalibrationUpdate())
            fFocStateUpdate(FOC_DISABLE);
        foc_main_loop_task();
        break;
    case FOC_RESET:
        foc_core_reset();
        fFocStateUpdate(FOC_IDLE);
        g_foc.foc_enable = false;
        BSP_PWM_Disable();
        BSP_POWER_12V_Control(true);

        break;
    case FOC_OPENLOOP:
        // 开环启动：锁定→斜坡→匀速，让SMO建立BEMF后再切闭环
        if (!g_foc.foc_enable) {
            g_foc.foc_enable = true;
            g_foc.ol_start_tick = BSP_GetTick();
            g_foc.ol_angle = 0.0f;
            BSP_PWM_Enable();
        }
        {
            uint32_t elapsed = BSP_GetTick() - g_foc.ol_start_tick;
            float rpm_cmd = 0.0f;
            if (elapsed < OL_START_LOCK_MS) {
                rpm_cmd = 0.0f;  // 锁定
            } else if (elapsed < (OL_START_LOCK_MS + OL_START_RAMP_MS)) {
                float t = (float)(elapsed - OL_START_LOCK_MS) / OL_START_RAMP_MS;
                rpm_cmd = OL_START_RPM * t;  // 斜坡加速
            } else {
                rpm_cmd = OL_START_RPM;  // 匀速
            }
            // 开环电压矢量
            g_foc.ol_angle += rpm_cmd * 6.0f * 0.00005f * g_Param.motor_polepairs;  // deg per tick
            if (g_foc.ol_angle > 360.0f) g_foc.ol_angle -= 360.0f;
            // 输出到 SVM
            float u_alpha = OL_START_CURRENT * cosf(g_foc.ol_angle * 0.0174533f);
            float u_beta = OL_START_CURRENT * sinf(g_foc.ol_angle * 0.0174533f);
            fSvpwmRun(u_alpha, u_beta);

            // SMO 在后台同时运行（在 foc_main_loop_task 中调用）
            // 条件满足时切闭环
            if (elapsed > (OL_START_LOCK_MS + OL_START_RAMP_MS + 200)) {
                if (FABSF(smo_get_omega()) > OL_START_RPM * 0.5f * g_Param.motor_polepairs) {
                    fFocStateUpdate(FOC_RUNNING);
                }
            }
        }
        break;

    case FOC_ENABLE:
        g_foc.foc_enable = true;
        BSP_PWM_Enable();
        fFocStateUpdate(FOC_RUNNING);
        break;
    case FOC_DISABLE:
        g_foc.foc_enable = false;
        BSP_PWM_Disable();
        fFocStateUpdate(FOC_RESET);
        break;
    case FOC_RUNNING:
        foc_main_loop_task();
        break;
    case FOC_SHUTDOWN:
        if (foc_shutdown())
            fFocStateUpdate(FOC_FAULT);
        break;
    case FOC_FAULT:
        if (g_foc.foc_enable)
        {
            g_foc.foc_enable = false;
            BSP_PWM_Disable();
            BSP_POWER_12V_Control(false);
        }
        break;
    default:
        break;
    }
}
// FOC 状态更新函数
void fFocStateUpdate(eFocState state)
{
    // 状态切换条件：
    // 1、故障状态只能通过复位退出
    if (g_foc.state == FOC_FAULT && state != FOC_RESET)
        return;
    // 2、使能状态只能从空闲进入
    if (state == FOC_ENABLE && g_foc.state != FOC_IDLE)
        return; // 只能从DISABLE进入ENABLE
    g_foc.state = state;
}
