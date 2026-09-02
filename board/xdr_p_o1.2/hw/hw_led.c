// ============================================================
// hw_led.c — 板上 GPIO LED 的 tLedDriverOps 实现（xdr_p_o1.2）
//
// 引脚映射与点亮极性只出现在本文件（hw 侧）；业务只面对 tLedDriverOps。
// 板载 LED 低电平点亮（active_low）。
// ============================================================

#include "hw_led.h"

#include "hw_pinmap.h" // LED_CANrx_* / LED_ENCODER_*
#include "stm32f4xx_hal.h"

#define HW_LED_NUM 2U

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    bool active_low; // true=低电平点亮
} tHwLedRes;

// idx0 = CAN 接收灯，idx1 = 编码器灯（与 pinmap 命名一致）
static tHwLedRes g_leds[HW_LED_NUM] = {
    {LED_CANrx_GPIOx, LED_CANrx_GPIOx_PIN, true},
    {LED_ENCODER_GPIOx, LED_ENCODER_GPIOx_PIN, true},
};

// ---- tLedDriverOps 实现 ----

static bool hw_led_init(LedHandle h)
{
    if (!h)
        return false;
    // GPIO 已由 CubeMX 初始化为输出；这里仅校验句柄
    return true;
}

static void hw_led_set(LedHandle h, bool active)
{
    if (!h)
        return;
    tHwLedRes *r = (tHwLedRes *)h;
    // active_low：active=true 输出低电平
    GPIO_PinState level = (active == r->active_low) ? GPIO_PIN_RESET : GPIO_PIN_SET;
    HAL_GPIO_WritePin(r->port, r->pin, level);
}

static void hw_led_toggle(LedHandle h)
{
    if (!h)
        return;
    tHwLedRes *r = (tHwLedRes *)h;
    HAL_GPIO_TogglePin(r->port, r->pin);
}

static const tLedDriverOps g_hw_led_ops = {
    .init = hw_led_init,
    .set = hw_led_set,
    .toggle = hw_led_toggle,
};

const tLedDriverOps *hw_led_ops(void)
{
    return &g_hw_led_ops;
}

LedHandle hw_led_handle(uint8_t idx)
{
    if (idx >= HW_LED_NUM)
        return NULL;
    return (LedHandle)&g_leds[idx];
}
