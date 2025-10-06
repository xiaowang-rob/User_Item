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

#endif