#include "foc_main.h"
#include "svpwm.h"

// 开环启动参数
#define OL_START_LOCK_MS 200  // 锁定时间 [ms]
#define OL_START_RAMP_MS 500  // 斜坡时间 [ms]
#define OL_START_RPM 200.0f   // 开环目标转速 [rpm]
#define OL_START_CURRENT 1.0f // 开环电流 [A]
#include "math_fast.h"
#include "parameter_manager.h"
#include "device.h"
#include "smo.h"

#include "bsp_adc.h"
#include "svpwm.h"

FOC_t g_foc;

// 主循环为pwm 20khz
// 加两个低频循环：中频1khz 低频100hz
static void loop_main_update_task();
static void loop_middle_update_task();
static void loop_low_update_task();

static u8 loop_count = 0;
static bool loop_middle_update = false;
static bool loop_low_update = false;

#ifdef __DEBUG__
u32 _time_focit_start = 0;
u32 _time_focit_end = 0;
volatile u32 _time_focit_run = 0;
volatile u32 _time_foc_T = 0;

#endif

// 下溢中断
void bsp_foc_it_callback()
{
#ifdef __DEBUG__
    _time_focit_start = BSP_GetTick_us();
#endif

    loop_main_update_task();

    loop_count++;
    if (loop_count % 20 == 0)
    {
        loop_middle_update = true;
        if (loop_count >= 200)
        {
            loop_low_update = true;
            loop_count = 0;
        }
    }
    if (loop_middle_update)
    {
        loop_middle_update_task();
        loop_middle_update = false;
    }
    if (loop_low_update)
    {
        loop_low_update_task();
        loop_low_update = false;
    }

#ifdef __DEBUG__
    _time_foc_T = BSP_GetTick_us() - _time_focit_end;
    _time_focit_end = BSP_GetTick_us();
    _time_focit_run = _time_focit_end - _time_focit_start;
#endif
}
// FOC 主初始化函数
void foc_init()
{
    g_foc.foc_enable = false;
    g_foc.state = FOC_IDLE;
    foc_core_init();
    BSP_POWER_12V_Control(true);
    g_foc.foc_init = true;
}

// FOC 主循环函数
static void loop_main_update_task()
{
    if (!g_foc.foc_init)
        return;
    foc_value_update();
    switch (g_foc.state)
    {
    case FOC_IDLE:
        BSP_AdcIdleTrack();
        break;
    case FOC_TUNE:
        if (!g_foc.foc_enable)
        {
            g_foc.foc_enable = true;
            foc_core_reset();
            BSP_PWM_Enable();
        }
        if (auto_calibration_update())
            foc_state_update(FOC_DISABLE);
        foc_main_loop_task();
        break;
    case FOC_RESET:
        pro_manager_clear_flag();
        foc_core_reset();
        foc_state_update(FOC_IDLE);
        g_foc.foc_enable = false;
        BSP_PWM_Disable();
        BSP_POWER_12V_Control(true);

        break;

    case FOC_ENABLE:
        g_foc.foc_enable = true;
        BSP_PWM_Enable();
        foc_state_update(FOC_RUNNING);
        break;
    case FOC_DISABLE:
        g_foc.foc_enable = false;
        BSP_PWM_Disable();
        foc_state_update(FOC_RESET);
        break;
    case FOC_RUNNING:
        foc_main_loop_task();
        break;
    case FOC_SHUTDOWN:
        if (foc_shutdown())
            foc_state_update(FOC_FAULT);
        break;
    case FOC_FAULT:
        if (g_foc.foc_enable)
        {
            g_foc.foc_enable = false;
            BSP_PWM_Disable();
            BSP_POWER_12V_Control(false);
        }
        break;
    default:
        break;
    }
}
// 1khz 中频循环
static void loop_middle_update_task()
{
}

// 100hz 低频循环
static void loop_low_update_task()
{
}

// FOC 状态更新函数
void foc_state_update(eFocState state)
{
    // 状态切换条件：
    // 1、故障状态只能通过复位退出
    if (g_foc.state == FOC_FAULT && state != FOC_RESET)
        return;
    // 2、使能状态只能从空闲进入
    if (state == FOC_ENABLE && g_foc.state != FOC_IDLE)
        return; // 只能从DISABLE进入ENABLE
    g_foc.state = state;
}
