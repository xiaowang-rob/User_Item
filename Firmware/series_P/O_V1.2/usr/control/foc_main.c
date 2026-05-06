#include "foc_main.h"
#include "math_fast.h"
#include "parameter_manager.h"
#include "device.h"

#include "bsp_adc.h"

FOC_t g_foc = {.core = &foc_core, .loop_con = &loop_con, .hfi = &g_hfi, .smo = &smo, .tun = &g_tune_ctx, .svpwm = &svpwm};

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
    fFOC_StateMachineMainLoop();
#ifdef __DEBUG__
    _time_foc_T = BSP_GetTick_us() - _time_focit_end;
    _time_focit_end = BSP_GetTick_us();
    _time_focit_run = _time_focit_end - _time_focit_start;
#endif
}
// FOC 主初始化函数
void fFOC_Init()
{
    g_foc.core->foc_mode->runmode = OPEN_LOOP;
    g_foc.foc_enable = false;
    BSP_PWM_Enable();
    BSP_POWER_12V_Control(true);
    g_foc.state = FOC_IDLE;
    fFOC_CoreInit();
    g_foc.foc_init = true;
    // 校准电流零点
    g_foc.state = FOC_ENABLE;
    BSP_AdcRecalibrateCurrent();
    while (false == BSP_AdcRecalibrateDone())
        ;
    g_foc.state = FOC_DISABLE;
}

// FOC 主循环函数
void fFOC_StateMachineMainLoop()
{
    if (!g_foc.foc_init)
        return;
    fFOC_ValueUpdate();
    switch (g_foc.state)
    {
    case FOC_IDLE:
        break;
    case FOC_AUTO_TUNE:
        if (!g_foc.foc_enable)
        {
            g_foc.foc_enable = true;
            fFOC_CoreReset();
            BSP_PWM_Enable();
        }
        if (fAutoCalibrationUpdate())
            fFOC_StateUpdate(FOC_DISABLE);
        fFOC_MainLoopTask();
        break;
    case FOC_RESET:
        fFOC_CoreReset();
        fFOC_StateUpdate(FOC_IDLE);
        BSP_POWER_12V_Control(false);
        break;
    case FOC_ENABLE:
        g_foc.foc_enable = true;
        BSP_PWM_Enable();
        fFOC_StateUpdate(FOC_RUNNING);
        break;
    case FOC_DISABLE:
        g_foc.foc_enable = false;
        BSP_PWM_Disable();
        fFOC_StateUpdate(FOC_RESET);
        break;
    case FOC_RUNNING:
        fFOC_MainLoopTask();
        break;
    case FOC_SHUTDOWN:
        if (fFOC_Shutdown())
            fFOC_StateUpdate(FOC_FAULT);
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
void fFOC_StateUpdate(eFOC_Status state)
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
