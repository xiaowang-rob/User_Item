#include "foc_main.h"
#include "math_fast.h"
#include "parameter_manager.h"
#include "device.h"

#include "bsp_adc.h"

FOC_t g_foc = {.core = &g_foc_core, .loop_con = &g_loop_con, .hfi = &g_hfi, .smo = &g_smo, .tun = &g_tune_ctx, .svpwm = &g_svpwm};

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
    fFocCoreInit();
    BSP_POWER_12V_Control(true);
    g_foc.foc_init = true;
}

// FOC 主循环函数
void fFocStateMachineMainLoop()
{
    if (!g_foc.foc_init)
        return;
    fFocValueUpdate();
    switch (g_foc.state)
    {
    case FOC_IDLE:
        break;
    case FOC_TUNE:
        if (!g_foc.foc_enable)
        {
            g_foc.foc_enable = true;
            fFocCoreReset();
            BSP_PWM_Enable();
        }
        if (fAutoCalibrationUpdate())
            fFocStateUpdate(FOC_DISABLE);
        fFocMainLoopTask();
        break;
    case FOC_RESET:
        fFocCoreReset();
        fFocStateUpdate(FOC_IDLE);
        BSP_POWER_12V_Control(true);
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
        fFocMainLoopTask();
        break;
    case FOC_SHUTDOWN:
        if (fFocShutdown())
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
