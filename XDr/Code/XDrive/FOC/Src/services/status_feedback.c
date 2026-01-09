#include "status_feedback.h"
#include "foc_statemachine.h"
#include "rgb.h"
#include "drive_state.h"
#include "protection_manager.h"

static u16 _encoder_tic = 0;
static u16 _canrx_tic = 0;

void status_feedback()
{
    switch (g_foc.state)
    {
    case FOC_AUTO_TUNE:
        rgb_breathe(YELLOW); // 黄色
        break;
    case FOC_IDLE:
        rgb_breathe(BLUE); // 蓝色
        break;
    case FOC_RUNNING:
        rgb_breathe(GREEN); // 绿色
        break;
    case FOC_SHUTDOWN:
        rgb_alternate(GREEN, RED); // 绿色红色交替
        break;
    case FOC_FAULT:
        rgb_breathe(RED); // 红色
        break;
    default:
        break;
    }

    switch (ENCODER_state_get())
    {
    case ONLINE: // OK
        led_flash(ENCODER);
        break;
    case OFFLINE:       // 无编码器
    case SINGNAL_ERROR: // 弱磁
        led_off(ENCODER);
        break;
    case RUN_ERROR: // 通讯异常
        led_on(ENCODER);
        break;
    default:
        break;
    }
    switch (g_pro_manager.com_state->can_state)
    {
    case ONLINE: // OK
        led_flash(CAN);
        break;
    case INIT_ERROR: // 初始化失败
        led_off(CAN);
        break;
    case RUN_ERROR: // 通讯异常
        break;
        led_on(CAN);
    default:
        break;
    }
}
void System_Fault_feedback()
{
    rgb_3_alternate(RED, GREEN, BLUE); // 红绿蓝交替
}