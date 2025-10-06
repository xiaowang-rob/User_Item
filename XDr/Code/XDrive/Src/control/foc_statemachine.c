#include "foc_statemachine.h"
#include "stdbool.h"
#include "auto_calibration.h"

bool FOC_INIT_event()
{
    // TODO: add code here
}
bool FOC_AUTO_TUNE_event()
{
    return auto_calibration_update();
}
void FOC_IDLE_event()
{
}
void FOC_RUNNING_event()
{
}
bool FOC_SHUTDOWN_event()
{
}
void FOC_FAULT_event()
{
}

void FOC_StateMachine_updata(FOC_STATE_e *state)
{
    switch (*state)
    {
    case FOC_INIT:
        if (FOC_INIT_event())
            *state = FOC_IDLE;
        break;
    case FOC_AUTO_TUNE:
        if (FOC_AUTO_TUNE_event())
            *state = FOC_IDLE;
        break;
    case FOC_IDLE:
        FOC_IDLE_event();
        break;
    case FOC_RUNNING:
        FOC_RUNNING_event();
        break;
    case FOC_SHUTDOWN:
        if (FOC_SHUTDOWN_event())
            *state = FOC_FAULT;
        break;
    case FOC_FAULT:
        FOC_FAULT_event();
        break;
    default:
        break;
    }
}