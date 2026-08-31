#include "led.h"

#include <string.h>

//  64点正弦查找表
static const uint8_t SINE_TABLE[64] = {
    128, 140, 153, 165, 177, 188, 199, 209,
    218, 226, 234, 240, 245, 250, 253, 254,
    254, 253, 250, 245, 240, 234, 226, 218,
    209, 199, 188, 177, 165, 153, 140, 128,
    128, 116, 103, 91, 79, 68, 57, 47,
    38, 30, 22, 16, 11, 6, 3, 2,
    2, 3, 6, 11, 16, 22, 30, 38,
    47, 57, 68, 79, 91, 103, 116, 128};

//  颜色常量
const tRGBColor WHITE = {255, 255, 255};
const tRGBColor BLACK = {0, 0, 0};
const tRGBColor RED = {255, 0, 0};
const tRGBColor GREEN = {0, 255, 0};
const tRGBColor BLUE = {0, 0, 255};
const tRGBColor YELLOW = {255, 255, 0};

const tRGBColor CHINA_RED = {230, 0, 0};        // 中国红
const tRGBColor KLEIN_BLUE = {0, 47, 167};      // 克莱因蓝
const tRGBColor MARS_GREEN = {6, 128, 67};      // 绿
const tRGBColor PRUSSIAN_BLUE = {0, 49, 83};    // 普鲁士蓝
const tRGBColor TIFFANY_BLUE = {129, 216, 208}; // 蒂夫尼蓝
const tRGBColor SKY = {35, 235, 185};           // 青绿
const tRGBColor ORANGE = {232, 88, 39};         // 品红

bool led_init(tLed *led, LedHandle handle, tLedDriverOps *ops)
{
    if (ops == NULL || handle == NULL)
        return false;

    led->ops = ops;
    led->handle = handle;

    led->ops->init(handle);

    led->fast_blink_time = 300; // 快速闪烁时间 (毫秒)
    led->slow_blink_time = 800; // 慢速闪烁时间
    led->state = LED_OFF;
    return true;
}

void led_set_config(tLed *led, uint16_t fast_blink_time, uint16_t slow_blink_time)
{
    led->fast_blink_time = fast_blink_time;
    led->slow_blink_time = slow_blink_time;
}

void led_set_state(tLed *led, eLedState state)
{
    led->state = state;
}

void led_task_loop(tLed *led)
{
    if (led->state == LED_ON)
    {
        led->ops->set(led->handle, true);
    }
    else if (led->state == LED_OFF)
    {
        led->ops->set(led->handle, false);
    }
    else if (led->state == LED_BLINK_SLOW)
    {
        uint32_t current_time = bsp_get_tick();
        if (current_time > led->next_change_time)
        {
            led->ops->toggle(led->handle);
            led->next_change_time = current_time + led->slow_blink_time;
        }
    }
    else if (led->state == LED_BLINK_FAST)
    {
        uint32_t current_time = bsp_get_tick();
        if (current_time > led->next_change_time)
        {
            led->ops->toggle(led->handle);
            led->next_change_time = current_time + led->fast_blink_time;
        }
    }
}

bool rgb_init(tRgb *rgb, RgbHandle handle, tRgbDriverOps *ops)
{
    if (ops == NULL || handle == NULL)
        return false;

    rgb->ops = ops;
    rgb->handle = handle;

    rgb->ops->init(handle);

    rgb->fast_blink_time = 300; // 快速闪烁时间 (毫秒)
    rgb->slow_blink_time = 800; // 慢速闪烁时间
    rgb->breathe_interval = 40; // 呼吸周期
    rgb->state = RGB_OFF;
    return true;
}
void rgb_set_config(tRgb *rgb, uint16_t fast_blink_time, uint16_t slow_blink_time, uint16_t breathe_interval)
{
    rgb->fast_blink_time = fast_blink_time;
    rgb->slow_blink_time = slow_blink_time;
    rgb->breathe_interval = breathe_interval;
}
void rgb_set_state(tRgb *rgb, eRgbState state)
{
    rgb->state = state;
}
void rgb_set_color(tRgb *rgb, tRGBColor color)
{
    rgb->color = color;                    // 设置颜色
    rgb->ops->set_rgb(rgb->handle, color); // 设置RGB颜色
}
void rgb_task_loop(tRgb *rgb)
{
    static bool blink_flag = true;
    if (rgb->state == RGB_ON)
    {
        rgb->ops->set_brightness(rgb->handle, 255);
    }
    else if (rgb->state == RGB_OFF)
    {
        rgb->ops->set_brightness(rgb->handle, 0);
    }
    else if (rgb->state == RGB_BLINK_SLOW)
    {
        uint32_t current_time = bsp_get_tick();

        if (current_time > rgb->next_change_time)
        {
            rgb->ops->set_brightness(rgb->handle, blink_flag ? 255 : 0);
            rgb->next_change_time = current_time + rgb->slow_blink_time;
            blink_flag = !blink_flag; // 反转闪烁状态
        }
    }
    else if (rgb->state == RGB_BLINK_FAST)
    {
        uint32_t current_time = bsp_get_tick();
        if (current_time > rgb->next_change_time)
        {
            rgb->ops->set_brightness(rgb->handle, blink_flag ? 255 : 0);
            rgb->next_change_time = current_time + rgb->fast_blink_time;
            blink_flag = !blink_flag; // 反转闪烁状态
        }
    }
    else if (rgb->state == RGB_BREATHE)
    {
        uint32_t current_time = bsp_get_tick();
        if (current_time > rgb->next_change_time)
        {
            uint8_t brightness = SINE_TABLE[rgb->breath_idx];
            float normalized_brightness = brightness / 255.0f;
            rgb->ops->set_brightness(rgb->handle, normalized_brightness);

            rgb->breath_idx = (rgb->breath_idx + 1) % 64; // 循环索引
            rgb->next_change_time = current_time + rgb->breathe_interval;
        }
    }
}