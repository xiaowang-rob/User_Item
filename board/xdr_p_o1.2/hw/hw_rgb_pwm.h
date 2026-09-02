#ifndef __HW_RGB_PWM_H
#define __HW_RGB_PWM_H

#include "usr/if/pwm_dma_if.h"

// ============================================================
// hw_rgb_pwm.h — 板上 RGB(WS2812) 的 PWM+DMA 推流资源（board/<b>/hw）
//
// 提供 tPwmDmaIf 契约实例：TIM PWM + DMA 连续推送 CCR 序列，
// 由 hw 自持 PulseFinished 中断并转发"推完"事件（不形成通用回调层）。
// ============================================================

const tPwmDmaIf *hw_rgb_pwm_get(void);

// 板上 RGB 灯珠数量（组装层据此创建驱动实例）
uint8_t hw_rgb_pixel_num(void);

#endif // __HW_RGB_PWM_H
