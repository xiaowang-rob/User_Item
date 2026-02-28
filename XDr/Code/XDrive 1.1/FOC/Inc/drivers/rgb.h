

#ifndef __RGB_H
#define __RGB_H

#include "main.h"

// 单个LED的颜色控制结构体
typedef struct
{
    u16 R; ///< 红色分量，范围0-255
    u16 G; ///< 绿色分量，范围0-255
    u16 B; ///< 蓝色分量，范围0-255
} tRGBColor;

// 预定义颜色常量声明
extern const tRGBColor RED;     ///< 红色
extern const tRGBColor GREEN;   ///< 绿色
extern const tRGBColor BLUE;    ///< 深蓝色
extern const tRGBColor SKY;     ///< 天蓝色
extern const tRGBColor MAGENTA; ///< 粉色
extern const tRGBColor YELLOW;  ///< 黄色
extern const tRGBColor ORANGE;  ///< 橙色
extern const tRGBColor BLACK;   ///< 无颜色
extern const tRGBColor WHITE;   ///< 白色

// 呼吸灯控制结构体
typedef struct
{
    tRGBColor target_color; ///< 目标颜色（最大亮度时）
    u8 sine_index;          ///< 正弦表索引 [0-255]
    tRGBColor last_output;  ///< 上次输出颜色（用于变化检测）
    u32 last_time_ms;       ///< 上次更新时间
} tRGBBreath;

// LED状态枚举
typedef enum
{
    LED_OFF,        ///< 常灭
    LED_SLOW_BLINK, ///< 慢速闪烁
    LED_FAST_BLINK, ///< 快速闪烁
    LED_ON          ///< 常亮
} eLED_State;

// LED闪烁时间常量定义
#define LED_SLOW_BLINK_T_ms 1000 ///< 慢速闪烁周期：1秒
#define LED_FAST_BLINK_T_ms 500  ///< 快速闪烁周期：0.5秒

/* 函数声明 ----------------------------------------------------------------*/

void fRGB_Breathe(tRGBColor Color);
void fLED_Show(eLED_State can_state, eLED_State encoder_state);

#endif /* __RGB_H */