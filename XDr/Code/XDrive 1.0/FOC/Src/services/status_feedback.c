#include "status_feedback.h"
#include "foc_statemachine.h"
#include "rgb.h"
#include "protection_manager.h"


void status_feedback()
{
    switch (g_foc.state)
    {
    case FOC_AUTO_TUNE:
        fRGB_Breathe(MAGENTA); // 粉色
        break;
    case FOC_IDLE:
        fRGB_Breathe(BLUE); // 蓝色
        break;
    case FOC_RUNNING:
        fRGB_Breathe(GREEN); // 绿色
        break;
    case FOC_FAULT:
        fRGB_Breathe(RED); // 红色
        break;
    case FOC_WARNING:
        fRGB_Breathe(YELLOW); // 黄色
        break;
    default:
        break;
    }
    eLED_State can_state;
    eLED_State encoder_state;
    switch (g_pro_manager.drive_state->can_state)
    {
    case OFFLINE:
        can_state = LED_OFF;
        break;
    case ONLINE:
        can_state = LED_ON;
        break;
    case RUN_ERROR:
        can_state = LED_FAST_BLINK;
        break;
    case RUNNING:
        can_state = LED_SLOW_BLINK;
        break;
    default:
        break;
    }
    switch (g_pro_manager.drive_state->encoder_state)
    {
    case OFFLINE:
        encoder_state = LED_OFF;
        break;
    case ONLINE:
        encoder_state = LED_ON;
        break;
    case RUN_ERROR:
        encoder_state = LED_SLOW_BLINK;
        break;
    case RUNNING:
        encoder_state = LED_FAST_BLINK;
        break;
    default:
        break;
    }
    fLED_Show(can_state, encoder_state);
}
void System_Fault_feedback()
{
    fRGB_Breathe(WHITE); // 白色
}
