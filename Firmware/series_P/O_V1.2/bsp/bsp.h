// BSP接口声明 - 板级支持包
// 供app层调用，bsp实现层提供具体功能

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

void bsp_enable_irq();
void bsp_disable_irq();
// 中断向量表偏移
void bsp_set_vector_table_offset(u32 offset);

u32 bsp_get_tick(void);
u32 bsp_get_tick_us(void);
void bsp_delay(u32 ms);
void bsp_system_reset(void);

#endif // __BSP_H