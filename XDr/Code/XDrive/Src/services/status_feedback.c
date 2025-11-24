#include "status_feedback.h"
#include "foc_core.h"
#include "rgb.h"
#include "encoder.h"
#include "canDr.h"

static u16 _encoder_tic = 0;
static u16 _canrx_tic = 0;

void status_feedback()
{
    switch (g_foccore.state)
    {
    case FOC_INIT:
        rgb_breathe(MAGENTA); // 粉色
        break;
    case FOC_AUTO_TUNE:
        rgb_breathe(OEANGE); // 橘色
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

    switch (GET_ENCODER_STATUS())
    {
    case 0: // OK
        led_flash(ENCODER);
        break;
    case 1: // 无编码器
    case 3: // 弱磁
        led_off(ENCODER);
        break;
    case 2: // 通讯异常
        led_on(ENCODER);
        break;
    default:
        break;
    }
    switch (CAN_STATE_get())
    {
    case 0: // OK
        led_flash(CAN);
        break;
    case 1: // 初始化失败
        led_off(CAN);
        break;
    case 2: // 通讯异常
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