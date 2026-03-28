#ifndef __SYSTEM_STATEMACHINE_H
#define __SYSTEM_STATEMACHINE_H

#include "main.h"

typedef enum
{
    SYSTEM_INIT,
    SYSTEM_RUNNING,
    SYSTEM_ERROR,
} eSystemStatus;

void fSystemStateUpdata(eSystemStatus new_state);
eSystemStatus fSystemStateGet(void);
void SystemStateMachine_MainLoop(void);

#endif