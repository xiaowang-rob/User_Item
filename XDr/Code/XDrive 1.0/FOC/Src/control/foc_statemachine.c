#include "foc_statemachine.h"
#include "tim.h"
#include "math_fast.h"
#include "parameter_manager.h"
#include "encoder.h"

FOC_t g_foc = {0};

#ifdef __DEBUG__
u32 _time_focit_start = 0;
u32 _time_focit_end = 0;
u32 _time_focit_run = 0;
u32 _time_foc_T = 0;

#endif

// 由于tim8互补波输出的原因 计时器模式被强制改为中心对齐3 导致update回调被重复调用
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM8)
    {
        if (TIM8->CR1 & TIM_CR1_DIR)
        {
            // ========== 上溢中断 ==========
            return;
        }
        else
        {
            // ========== 下溢中断 ==========
#ifdef __DEBUG__
            _time_focit_start = HAL_GetTick_us();

#endif
            fFOC_StateMachineMainLoop();
#ifdef __DEBUG__
            _time_foc_T = HAL_GetTick_us() - _time_focit_end;
            _time_focit_end = HAL_GetTick_us();
            _time_focit_run = _time_focit_end - _time_focit_start;
#endif
            return;
        }
    }
}
// FOC 主初始化函数
void fFOC_Init()
{
    g_foc.state = FOC_IDLE;
    g_foc.core = &foc_core;
    g_foc.loop_con = &loop_con;
    g_foc.smo = &smo;
    g_foc.tun = &tun;
    g_foc.svpwm = &svpwm;
    fFOC_CoreInit();
}
// FOC 主循环函数
void fFOC_StateMachineMainLoop()
{
    fFOC_ValueUpdate();
    switch (g_foc.state)
    {
    case FOC_IDLE:
        break;
    case FOC_AUTO_TUNE:
        if (!g_foc.foc_enable)
        {
            ENABLE_PWM();
            g_foc.foc_enable = true;
        }
        if (fAutoCalibrationUpdate())
            fFOC_StateUpdate(FOC_DISABLE);
        fFOC_MainLoopTask();
        break;
    case FOC_RESET:
        fFOC_CoreReset();
        fFOC_StateUpdate(FOC_IDLE);
        break;
    case FOC_ENABLE:
        g_foc.foc_enable = true;
        fFOC_CoreReset();
        ENABLE_PWM();
        fFOC_StateUpdate(FOC_RUNNING);
        break;
    case FOC_DISABLE:
        g_foc.foc_enable = false;
        DISABLE_PWM();
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
            DISABLE_PWM();
        }
        break;
    default:
        break;
    }
}
// FOC 状态更新函数
void fFOC_StateUpdate(eFOC_Status state)
{
    if (g_foc.state == FOC_FAULT && state != FOC_IDLE)
        return;
    g_foc.state = state;
}
