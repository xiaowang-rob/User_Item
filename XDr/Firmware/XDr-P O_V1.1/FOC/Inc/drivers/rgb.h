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
extern const tRGBColor SKY;
extern const tRGBColor MAGENTA;
extern const tRGBColor YELLOW;
extern const tRGBColor ORANGE;
extern const tRGBColor BLACK;
extern const tRGBColor WHITE;

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
void fRGB_Breathe(tRGBColor Color); // 原API，无period_ms参数
void fRGB_Stop(void);
void fLED_Show(eLED_State can_state, eLED_State encoder_state);

#endif /* __RGB_H */