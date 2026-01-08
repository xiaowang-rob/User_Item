#include "system_statemachine.h"
#include "foc_statemachine.h"
#include "status_feedback.h"
#include "log.h"
#include "protection_manager.h"
#include "drive_state.h"
#include "port_mapping.h"

SYSTEM_STATE_e system_status = SYSTEM_INIT;

bool system_init_event(void)
{
    // 驱动层初始化
    drive_init(); // 顺便启动ADC数据刷新和foc定时器

    // 服务层初始化 首先参数 然后保护 最后日志
    if (!Param_init())
        return false;
    protection_manager_init();
    log_init();

    // 通讯层初始化
    communication_init();
    //  控制层初始化
    FOC_INIT();
    return true;
}

void SystemState_change(SYSTEM_STATE_e new_state)
{
    system_status = new_state;
}
SYSTEM_STATE_e SystemState_get(void)
{
    return system_status;
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
        // 通讯层运行
        communication_run();
        // 控制层由定时器驱动
        // 服务层运行
        protection_manager_run();
        status_feedback();
        break;
    case SYSTEM_ERROR:
        FOC_CHANGE_STATE(FOC_FAULT);
        System_Fault_feedback();
        break;
    }
}
