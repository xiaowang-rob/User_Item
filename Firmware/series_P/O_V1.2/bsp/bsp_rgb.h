#ifndef __BSP_RGB_H
#define __BSP_RGB_H

#include "bsp.h"

/* ========== 颜色结构体 ========== */
typedef struct
{
    u8 R;
    u8 G;
    u8 B;
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

void BSP_RGBInit(void);
bool BSP_RGB_SetAllColor(tRGBColor color);
void BSP_RGB_Breathe(tRGBColor Color);

#endif /* __BSP_RGB_H */