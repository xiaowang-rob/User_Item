#ifndef __SYSTEM_STATEMACHINE_H
#define __SYSTEM_STATEMACHINE_H

#include "main.h"

typedef enum
{
    SYSTEM_INIT,
    SYSTEM_RUNNING,
    SYSTEM_ERROR,
} SYSTEM_STATE_e;

voidSystemState_change(SYSTEM_STATE_e new_state);
SYSTEM_STATE_e SystemState_get(void);
void SystemStateMachine_run(void);
#endif