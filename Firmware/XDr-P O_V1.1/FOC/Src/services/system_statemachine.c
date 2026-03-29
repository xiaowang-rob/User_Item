#include "system_statemachine.h"
#include "foc_statemachine.h"
#include "status_feedback.h"
#include "log.h"
#include "protection_manager.h"
#include "port_mapping.h"
#include "adc_dr.h"
#include "flashDr.h"
#include "rgb.h"
#include "encoder.h"

eSystemStatus system_status = SYSTEM_INIT;

static bool _SystemInitEvent(void)
{
    /*
    这里的顺序不能乱，因为里面有一些初始化函数，如果顺序不对，会导致一些变量没有初始化，导致程序出错
    参数必须尽早出现 flash在其之前，初始化的时候最好不要有其他中断干涉，所有adc得往后放，但是foc初始化必须得有adc值，故最后
    正确的顺序应该是：
    flash和参数一起-通讯-保护-日志-adc -foc初始化
    */

    fFLASH_Init();
    if (!fParamInit())
        return false;
    fCommunicateInit();
    fProManagerInit();
    fLogInit();
    fAdcDrInit(); // 这里就启动了foc的定时器
    fFOC_Init();

    return true;
}

void fSystemStateUpdata(eSystemStatus new_state)
{
    system_status = new_state;
}

eSystemStatus fSystemStateGet(void)
{
    return system_status;
}
// 整个驱动的主循环
void SystemStateMachine_MainLoop(void)
{
    switch (system_status)
    {
    case SYSTEM_INIT:
        if (_SystemInitEvent())
            fSystemStateUpdata(SYSTEM_RUNNING);
        else
            fSystemStateUpdata(SYSTEM_ERROR);
        break;
    case SYSTEM_RUNNING:
        //	编码器主循环
        fEncoderMainLoopTask();
        // 通讯层运行
        fCommunicateMainLoop();
        // 控制层由定时器驱动
        // 服务层运行
        fProManagerMainLoop();
        fStatusFeedbackMainLoop();
        break;
    case SYSTEM_ERROR:
        fFOC_StateUpdate(FOC_FAULT);
        fSystemFaultFeedback();
        break;
    }
}
