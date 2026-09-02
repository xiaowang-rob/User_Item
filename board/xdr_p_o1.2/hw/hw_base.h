#ifndef __HW_BASE_H
#define __HW_BASE_H

#include "usr/if/time_if.h"

// ============================================================
// hw_base.h — 板级基础服务（board/<b>/hw 适配层入口）
//
// 本头是"hw → usr/if 反向依赖"的示范点：
// hw 作为 usr/if 接口的实现者，include 接口头是唯一允许的 hw→usr 依赖
// （对应分层文档：依赖铁律第 4 条的唯一例外）。
//
// 后续板级服务（irq / vector / reset / jump / 存储分区表）在此暴露。
// ============================================================

// 板级时间基准：返回 usr/if 契约的 tTimeIf 实现实例
const tTimeIf *hw_time_get(void);

// 板级基础初始化（使能 DWT 周期计数等）；在 HAL_Init 之后、装配设备前调用一次
void hw_base_init(void);

#endif // __HW_BASE_H
