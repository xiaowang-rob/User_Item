#include "status_feedback.h"
#include "foc_core.h"
#include "rgb.h"
#include "encoder.h"
#include "canDr.h"

static u16 _tic = 0;
static u8 _encoder_state = 0;
static u8 _canrx_tic = 0;
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
    if (GET_ENCODER_STATUS())
    {
        _encoder_state = 1;
        if (g_foccore.run_mode == ENCODER_CONTROL)
        {
            if (!GET_ENCODER_NO_MAG_FLAG() && !GET_ENCODER_COMMUNICATION_ERROR())
                _encoder_state = 2;
            else
                _encoder_state = 3;
        }
    }
    else
        _encoder_state = 0;
    switch (_encoder_state)
    {
    case 1: // 有编码器
        LED_ENCODER_EN();
        break;
    case 2: // 编码器通信正常
        _tic++;
        if (_tic == 1000)
        {
            LED_ENCODER_DIS();
        }
        if (_tic == 2000)
        {
            _tic = 0;
            LED_ENCODER_EN();
        }
        break;
    case 3: // 通讯异常
        _tic++;
        if (_tic == 100)
        {
            LED_ENCODER_DIS();
        }
        if (_tic == 200)
        {
            _tic = 0;
            LED_ENCODER_EN();
        }
        break;
    default: // 无编码器
        LED_ENCODER_DIS();
        break;
    }
    // todo: 增加CAN通信状态显示
    // if ( == 1)
    // {
    //     _canrx_tic++;
    //     if (_canrx_tic == 100)
    //     {
    //         LED_ENCODER_DIS();
    //     }
    //     if (_canrx_tic == 200)
    //     {
    //         _canrx_tic = 0;
    //         LED_ENCODER_EN();
    //     }
    // }
    // else
    //     LED_CANrx_DIS();
}
void System_Fault_feedback()
{
    rgb_3_alternate(RED, GREEN, BLUE); // 红绿蓝交替
}