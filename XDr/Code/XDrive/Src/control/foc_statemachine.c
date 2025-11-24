#include "foc_statemachine.h"
#include "stdbool.h"
#include "auto_calibration.h"
#include "tim.h"
#include "foc_core.h"
#include "svpwm.h"
#include "math_fast.h"


#ifdef __DEBUG__
u32 _time_focit_start = 0;
u32 _time_focit_end = 0;
u32 _time_focit_run = 0;
u32 _time_foc_T=0;
#endif
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM8)
    {
#ifdef __DEBUG__
        _time_focit_start = HAL_GetTick_us();
			
#endif
        FOC_StateMachine_updata(g_foccore.state);
#ifdef __DEBUG__
			_time_foc_T=HAL_GetTick_us()-_time_focit_end;
        _time_focit_end = HAL_GetTick_us();
       _time_focit_run = _time_focit_end - _time_focit_start;
#endif
    }
}

void FOC_INIT_event()
{
    if (foc_core_init())
        FOC_CHANGE_STATE(FOC_IDLE);
}
void FOC_AUTO_TUNE_event()
{
    foc_core_run();
    if (auto_calibration_update())
        FOC_CHANGE_STATE(FOC_IDLE);
}
void FOC_RESET_event()
{
    foc_core_reset();
    FOC_CHANGE_STATE(FOC_IDLE);
}
void FOC_enable_event()
{
    g_foccore.enable = true;
    ENABLE_PWM();
    FOC_CHANGE_STATE(FOC_RUNNING);
}
void FOC_disable_event()
{
    g_foccore.enable = false;
    DISABLE_PWM();
    FOC_CHANGE_STATE(FOC_RESET);
}
void FOC_RUNNING_event()
{
    foc_core_run();
}
void FOC_SHUTDOWN_event()
{
    if (fast_absf(g_monitor.omega_fb) > 0.1)
    {
        g_foccore.loop_mode = SPEED_LOOP_CONTROL;
        g_foccore.omega_con = -g_monitor.omega_fb;
        return;
    }
    FOC_CHANGE_STATE(FOC_FAULT);
}
void FOC_FAULT_event()
{
    if (g_foccore.enable)
    {
        g_foccore.enable = false;
        DISABLE_PWM();
    }
}

void FOC_StateMachine_updata(FOC_STATE_e state)
{
    switch (state)
    {
    case FOC_INIT:
        FOC_INIT_event();
        break;
    case FOC_AUTO_TUNE:
        FOC_AUTO_TUNE_event();
        break;
    case FOC_RESET:
        FOC_RESET_event();
        break;
    case FOC_IDLE:
        break;
    case FOC_ENABLE:
        FOC_enable_event();
        break;
    case FOC_DISABLE:
        FOC_disable_event();
        break;
    case FOC_RUNNING:
        FOC_RUNNING_event();
        break;
    case FOC_SHUTDOWN:
        FOC_SHUTDOWN_event();
        break;
    case FOC_FAULT:
        FOC_FAULT_event();
        break;
    default:
        break;
    }
}
void FOC_CHANGE_STATE(FOC_STATE_e state)
{
    g_foccore.state = state;
}
FOC_STATE_e FOC_Get_state()
{
    return g_foccore.state;
}
