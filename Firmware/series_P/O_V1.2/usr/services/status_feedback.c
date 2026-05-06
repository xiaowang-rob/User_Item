#include "status_feedback.h"
#include "foc_main.h"
#include "bsp_rgb.h"
#include "protection_manager.h"
#include "device.h"

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
    fLED_Control(g_pro_manager.drive_state->can_state, g_pro_manager.drive_state->encoder_state);
}
void fSystemFaultFeedback()
{
    BSP_RGB_Breathe(RED);
    BSP_Delay(500);
    BSP_RGB_Breathe((tRGBColor){0, 0, 0});
    BSP_Delay(500);
}
