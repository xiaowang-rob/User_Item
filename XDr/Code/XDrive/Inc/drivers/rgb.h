#ifndef __RGB_H
#define __RGB_H

#include "main.h"

// 0码和1码的定义，设置的时CCR寄存器的值
// 由于使用的思PWM输出模式1，计数值<CCR时，输出有效电平-高电平（CubeMX配置默认有效电平为高电平）
#define CODE_1 (68) // 1码定时器计数次数，控制占空比为84/125 = 66%
#define CODE_0 (34) // 0码定时器计数次数，控制占空比为42/125 = 33%

// 单个LED的颜色控制结构体
typedef struct
{
    u8 R;
    u8 G;
    u8 B;
} RGB_Color_TypeDef;

#define Pixel_NUM 2 // LED数量宏定义

static void Reset_Load(void); // 该函数用于将数组最后24个数据变为0，代表RESET_code

// 发送最终数组
static void RGB_SendArray(void);

static void RGB_Flush(void); // 刷新RGB显示

void RGB_SetOne_Color(u8 LedId, RGB_Color_TypeDef Color); // 给一个LED装载24个颜色数据码（0码和1码）

// 控制多个LED显示相同的颜色
void RGB_SetMore_Color(u8 head, u8 heal, RGB_Color_TypeDef color);

void RGB_Show_64(void); // RGB写入函数

#endif
