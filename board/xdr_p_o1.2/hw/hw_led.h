#ifndef __HW_LED_H
#define __HW_LED_H

#include <stdint.h>

#include "usr/abs/led.h"

// ============================================================
// hw_led.h — 板上 GPIO LED 资源（board/<b>/hw）
//
// 提供普通 LED（GPIO 亮灭）的 tLedDriverOps 实现：
//   hw_led_handle(idx) 取板上某颗 LED 的句柄（idx 0=CAN 灯，1=编码器灯）
//   hw_led_ops()        取共享驱动 ops 表
// 引脚映射与极性只存在于本文件（hw 侧）。
// ============================================================

LedHandle hw_led_handle(uint8_t idx);
const tLedDriverOps *hw_led_ops(void);

#endif // __HW_LED_H
