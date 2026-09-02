#ifndef __RGB_DRIVERS_H
#define __RGB_DRIVERS_H

#include <stdint.h>

#include "usr/abs/led.h"
#include "usr/if/pwm_dma_if.h"

// ============================================================
// rgb_drivers.h — RGB 灯芯片驱动统一出口（usr/drv）
//
// 供组装层使用：create(注入 PWM-DMA 接口 + 灯珠数)。
// ============================================================

// ---- WS2812/WS28xx 系列（GRB，PWM 时序驱动） ----
RgbHandle ws28xx_create(const tPwmDmaIf *pwm, uint8_t num_pixels);
void ws28xx_destroy(RgbHandle h);
extern const tRgbDriverOps ws28xx_driver_ops;

#endif // __RGB_DRIVERS_H
