#ifndef __LED_H
#define __LED_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint8_t R;
    uint8_t G;
    uint8_t B;
} tRGBColor;

typedef enum
{
    LED_OFF,
    LED_ON,
    LED_BLINK_SLOW,
    LED_BLINK_FAST,
} eLedState;

typedef enum
{
    RGB_OFF,
    RGB_ON,
    RGB_BLINK_SLOW,
    RGB_BLINK_FAST,
    RGB_BREATHE
} eRgbState;

// 预定义颜色
const tRGBColor WHITE;
const tRGBColor BLACK;
extern const tRGBColor RED;
extern const tRGBColor GREEN;
extern const tRGBColor BLUE;
extern const tRGBColor YELLOW;

extern const tRGBColor CHINA_RED;     // 中国红
extern const tRGBColor KLEIN_BLUE;    // 克莱因蓝
extern const tRGBColor MARS_GREEN;    // 马尔斯绿
extern const tRGBColor PRUSSIAN_BLUE; // 普鲁士蓝
extern const tRGBColor TIFFANY_BLUE;  // 蒂夫尼蓝
extern const tRGBColor ORANGE;        // 橙色

// 定义LED设备的操作句柄 (不透明指针)
typedef void *LedHandle;
typedef void *RgbHandle;

// 定义LED驱动操作函数表 (核心抽象)
typedef struct
{
    // 初始化LED硬件
    bool (*init)(LedHandle handle);
    // 设置普通LED的开关状态
    void (*set)(LedHandle handle, bool active);
    // 切换普通LED的开关状态
    void (*toggle)(LedHandle handle);

} tLedDriverOps;

typedef struct
{
    tLedDriverOps *ops;
    LedHandle handle;

    uint16_t fast_blink_time;  // 快速闪烁时间 (毫秒)
    uint16_t slow_blink_time;  // 慢速闪烁时间 (毫秒)
    uint32_t next_change_time; // 下一次切换时间 (毫秒)

    volatile eLedState state; // 当前状态

} tLed;

// 核心LED操作函数
bool led_init(tLed *led, LedHandle handle, tLedDriverOps *ops);
void led_set_config(tLed *led, uint16_t fast_blink_time, uint16_t slow_blink_time);
void led_set_state(tLed *led, eLedState state);
void led_task_loop(tLed *led);

// 定义RGB LED驱动操作函数表 (核心抽象)
typedef struct
{
    // 初始化LED硬件
    bool (*init)(RgbHandle handle);
    // 设置RGB LED的颜色 (通过颜色结构体)
    void (*set_rgb)(RgbHandle handle, tRGBColor color);
    // 设置RGB LED的亮度，0-255
    void (*set_brightness)(RgbHandle handle, uint8_t brightness);
    // 刷新RGB LED的显示
    void (*refresh)(RgbHandle handle);
} tRgbDriverOps;

typedef struct
{
    tRgbDriverOps *ops;
    RgbHandle handle;

    volatile eRgbState state; // 当前状态
    tRGBColor color;          // 当前颜色

    uint16_t fast_blink_time;  // 快速闪烁时间 (毫秒)
    uint16_t slow_blink_time;  // 慢速闪烁时间 (毫秒)
    uint16_t breathe_interval; // 呼吸周期 (毫秒)
    uint8_t breath_idx;        // 呼吸索引 (0~255)
    uint32_t next_change_time; // 下一次切换时间 (毫秒)

} tRgb;

bool rgb_init(tRgb *rgb, RgbHandle handle, tRgbDriverOps *ops);
void rgb_set_config(tRgb *rgb, uint16_t fast_blink_time, uint16_t slow_blink_time, uint16_t breathe_interval);
void rgb_set_state(tRgb *rgb, eRgbState state);
void rgb_set_color(tRgb *rgb, tRGBColor color);
void rgb_task_loop(tRgb *rgb);

#endif // __LED_H