#ifndef __PROTECTION_MANAGER_H
#define __PROTECTION_MANAGER_H

#include "device.h"
#include "port_mapping.h"
#include "protocol.h"
#include "parameter_manager.h"

typedef struct
{
    float temperature;
    eFaultState fault;
    eWarningState warning;
    bool fault_flag;
    bool warning_flag;
    float max_current;
    float max_omega;
    float min_position;
    float max_position;
    float tolerance_time_ms;
    float tolerance_limit;

    tCommunicationState *com_state;
    tDeviceStatus *drive_state;
} tProtectionManager;
extern tProtectionManager g_pro_manager;

// functions
void fProManagerInit(tParameter *param);
void fProManagerClearFalg();
void fProManagerMainLoop();

void fProSetLimitPosition(float min_position, float max_position);

#endif /* __PROTECTION_MANAGER_H */