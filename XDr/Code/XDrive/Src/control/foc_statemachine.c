#include "foc_statemachine.h"
#include "stdbool.h"
#include "auto_calibration.h"
#include "tim.h"
#include "foc_core.h"

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM8)
    {
        FOC_StateMachine_updata(g_foccore.state);
        foc_core_run();
    }
}

bool FOC_INIT_event()
{
    // TODO:基本参数写入+初始化
    return false;
}
bool FOC_AUTO_TUNE_event()
{
    foc_enable();
    return auto_calibration_update();
}
void FOC_IDLE_event()
{
    foc_disable();
}
void FOC_RUNNING_event()
{
    foc_enable();
}
bool FOC_SHUTDOWN_event()
{
    return foc_shutdown();
}
void FOC_FAULT_event()
{
    foc_disable();
}

void FOC_StateMachine_updata(FOC_STATE_e state)
{
    switch (state)
    {
    case FOC_INIT:
        if (FOC_INIT_event())
            FOC_CHANGE_STATE(FOC_IDLE);
        break;
    case FOC_AUTO_TUNE:
        if (FOC_AUTO_TUNE_event())
            FOC_CHANGE_STATE(FOC_IDLE);
        break;
    case FOC_IDLE:
        FOC_IDLE_event();
        break;
    case FOC_RUNNING:
        FOC_RUNNING_event();
        break;
    case FOC_SHUTDOWN:
        if (FOC_SHUTDOWN_event())
            FOC_CHANGE_STATE(FOC_FAULT);
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
