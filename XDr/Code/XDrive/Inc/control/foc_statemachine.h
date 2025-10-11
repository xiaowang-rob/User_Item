#ifndef __FOC_STATEMACHINE_H
#define __FOC_STATEMACHINE_H

typedef enum
{
    FOC_INIT,
    FOC_AUTO_TUNE,
    FOC_IDLE,
    FOC_RUNNING,
    FOC_SHUTDOWN,
    FOC_FAULT
} FOC_STATE_e;

void FOC_StateMachine_updata(FOC_STATE_e state);
void FOC_CHANGE_STATE(FOC_STATE_e state);
#endif