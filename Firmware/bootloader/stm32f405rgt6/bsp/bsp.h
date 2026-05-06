/**
 * @file    bsp.h
 * @brief   BSP接口声明 - 板级支持包
 * @note    供app层调用，bsp实现层提供具体功能
 */

#ifndef __BSP_H
#define __BSP_H

#include <stdint.h>
#include <stdbool.h>

#ifndef __weak
#define __weak __attribute__((weak))
#endif

// 基础类型定义
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;

void BSP_enable_irq();
void BSP_disable_irq();
// 中断向量表偏移
void BSP_SetVectorTableOffset(u32 offset);

u32 BSP_GetTick(void);
u32 BSP_GetTick_us(void);
void BSP_Delay(u32 ms);
void BSP_SystemReset(void);

#endif /* __BSP_H */