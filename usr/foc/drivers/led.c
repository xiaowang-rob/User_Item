#include "bsp_interface.h"
#include "device.h"

void fLED_Control(eLED_State can_state, eLED_State encoder_state)
{
    static u32 led_base_time = 0;
    static bool half_blink_flag = false;

    u32 now = BSP_GetTick();
    u32 elapsed = now - led_base_time;

    if (elapsed >= 300)
    {
        if (!half_blink_flag)
        {
            if (can_state == LED_FAST_BLINK)
                BSP_LED_CanTogglePin();
            if (encoder_state == LED_FAST_BLINK)
                BSP_LED_EncoderTogglePin();
            half_blink_flag = true;
        }
        if (elapsed > 600)
        {
            switch (can_state)
            {
            case LED_OFF:
                BSP_LED_CanSetPin(false);
                break;
            case LED_ON:
                BSP_LED_CanSetPin(true);
                break;
            default:
                BSP_LED_CanTogglePin();
                break;
            }
            switch (encoder_state)
            {
            case LED_OFF:
                BSP_LED_EncoderSetPin(false);
                break;
            case LED_ON:
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