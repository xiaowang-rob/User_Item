/***************************************************************************************************
 * @file    rgb.h
 * @brief   RGB LED 驱动 (WS2812B呼吸+GPIO状态LED)
 ***************************************************************************************************/

#ifndef __RGB_H
#define __RGB_H

#include "device.h"
#include <stdbool.h>

/* ========== 颜色结构体 ========== */
typedef struct
{
    uint8_t R;
    uint8_t G;
    uint8_t B;
} tRGBColor;

/* ========== 预定义颜色 ========== */
extern const tRGBColor RED;
extern const tRGBColor GREEN;
extern const tRGBColor BLUE;

extern const tRGBColor CHINA_RED;     // 中国红
extern const tRGBColor KLEIN_BLUE;    // 克莱因蓝
extern const tRGBColor MARS_GREEN;    // 马尔斯绿
extern const tRGBColor PRUSSIAN_BLUE; // 普鲁士蓝
extern const tRGBColor TIFFANY_BLUE;  // 蒂夫尼蓝
extern const tRGBColor ORANGE;        // 橙色

/* ========== LED状态枚举 ========== */
typedef enum
{
    LED_OFF,
    LED_SLOW_BLINK,
    LED_FAST_BLINK,
    LED_ON
} eLED_State;

/* ========== 函数声明 ========== */
void fRGB_Init(void);
bool fRGB_SetAllColor(tRGBColor color);
void fRGB_Breathe(tRGBColor Color);
void fLED_Show(eLED_State can_state, eLED_State encoder_state);

#endif /* __RGB_H */