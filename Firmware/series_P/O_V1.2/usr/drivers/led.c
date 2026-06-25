
#include "device.h"

void led_control(eDeviceStatus can_state, eDeviceStatus encoder_state)
{
    static u32 led_base_time = 0;
    static bool half_blink_flag = false;

    u32 now = bsp_get_tick();
    u32 elapsed = now - led_base_time;

    if (elapsed >= 300)
    {
        if (!half_blink_flag)
        { // 快闪
            if (can_state == RUNNING)
                bsp_led_can_toggle_pin();
            if (encoder_state == RUNNING)
                bsp_led_encoder_toggle_pin();
            half_blink_flag = true;
        }
        if (elapsed > 600)
        { // 慢闪
            switch (can_state)
            {
            case OFFLINE:
                bsp_led_can_set_pin(false);
                break;
            case ONLINE:
                bsp_led_can_set_pin(true);
                break;
            default:
                bsp_led_can_toggle_pin();
                break;
            }
            switch (encoder_state)
            {
            case OFFLINE:
                bsp_led_encoder_set_pin(false);
                break;
            case ONLINE:
                bsp_led_encoder_set_pin(true);
                break;
            default:
                bsp_led_encoder_toggle_pin();
                break;
            }
            led_base_time = now;
            half_blink_flag = false;
        }
    }
}