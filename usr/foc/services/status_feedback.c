#include "status_feedback.h"
#include "foc_main.h"
#include "bsp_interface.h"
#include "protection_manager.h"

void fStatusFeedbackMainLoop()
{
    switch (g_foc.state)
    {
    case FOC_AUTO_TUNE:
        BSP_RGB_Breathe(TIFFANY_BLUE);
        break;
    case FOC_IDLE:
        BSP_RGB_Breathe(KLEIN_BLUE);
        break;
    case FOC_RUNNING:
        BSP_RGB_Breathe(MARS_GREEN);
        break;
    case FOC_FAULT:
        BSP_RGB_Breathe(CHINA_RED);
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
        encoder_state = LED_FAST_BLINK;
        break;
    case RUNNING:
        encoder_state = LED_SLOW_BLINK;
        break;
    default:
        break;
    }
    fLED_Control(can_state, encoder_state);
}
void fSystemFaultFeedback()
{
    BSP_RGB_Breathe(RED);
    BSP_Delay(500);
    BSP_RGB_Breathe((tRGBColor){0, 0, 0});
    BSP_Delay(500);
}
