#include "system_statemachine.h"
#include "stream_transmission.h"
#include "adcDr.h"
#include "adaptive_control.h"

SYSTEM_STATE_e system_status = SYSTEM_INIT;

bool system_init_event(void)
{
    if (!parameter_init())
        return false;
    ADC_DR_Init();
    adaptive_control_init();
}
void SystemState_change(SYSTEM_STATE_e new_state)
{
    system_status = new_state;
}
void SystemStateMachine_run(void)
{
    switch (system_status)
    {
    case SYSTEM_INIT:
        if (system_init_event())
            SystemState_change(SYSTEM_RUNNING);
        else
            SystemState_change(SYSTEM_ERROR);
        break;
    case SYSTEM_RUNNING:

        break;
    case SYSTEM_ERROR:

        break;
    }
}
