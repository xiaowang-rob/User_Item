#ifndef __TIME_IF_H
#define __TIME_IF_H

#include <stdint.h>

// ============================================================
// time_if.h — 时间基准接口（usr/if 契约）
//
// 用途：业务层 / 驱动层唯一的统一时钟来源，替代 bsp_get_tick / HAL_GetTick
//       等各板私有时钟 API。
//
// 实现者：board/<b>/hw
//   - HAL 板  ：基于 SysTick / HAL_GetTick，一行转发
//   - SPL 板  ：hw/base 自建 SysTick 毫秒计数（标准库无现成 tick）
//   - host 单测：stub 一个可手动拨动的时钟
//
// 规则：
//   - 本文件禁止 include 任何非标准头（可被 hw 干净反向 include）
//   - 返回值可回绕，调用方一律用差值比较（(uint32_t)(now - last)）
//   - ctx 由 hw 持有并解释，对调用方完全不透明
// ============================================================

typedef struct
{
    void *ctx;               // hw 侧时钟资源（对调用方不透明）

    uint32_t (*get_ms)(void *ctx); // 毫秒 tick
    uint32_t (*get_us)(void *ctx); // 微秒 tick
} tTimeIf;

#endif // __TIME_IF_H
