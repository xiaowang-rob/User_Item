
#include "device.h"

void fLED_Control(eDeviceStatus can_state, eDeviceStatus encoder_state)
{
    static u32 led_base_time = 0;
    static bool half_blink_flag = false;

    u32 now = BSP_GetTick();
    u32 elapsed = now - led_base_time;

    if (elapsed >= 300)
    {
        if (!half_blink_flag)
        { // 快闪
            if (can_state == RUNNING)
                BSP_LED_CanTogglePin();
            if (encoder_state == RUNNING)
                BSP_LED_EncoderTogglePin();
            half_blink_flag = true;
        }
        if (elapsed > 600)
        { // 慢闪
            switch (can_state)
            {
            case OFFLINE:
                BSP_LED_CanSetPin(false);
                break;
            case ONLINE:
                BSP_LED_CanSetPin(true);
                break;
            default:
                BSP_LED_CanTogglePin();
                break;
            }
            switch (encoder_state)
            {
            case OFFLINE:
                BSP_LED_EncoderSetPin(false);
                break;
            case ONLINE:
                BSP_LED_EncoderSetPin(true);
                break;
            default:
                BSP_LED_EncoderTogglePin();
                break;
            }
            led_base_time = now;
            half_blink_flag = false;
        }
    }
}