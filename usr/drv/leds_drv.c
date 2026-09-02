#include "device.h"
#include "bsp_led.h"

// LED 配置表

#define LED_NUM 2
#define LED0_ACTIVE 0 // 0表示低电平点亮，1表示高电平点亮
#define LED1_ACTIVE 0

// LED 驱动上下文
typedef struct
{
    uint8_t led_id;
} tLed_ctx;

// LED 驱动操作函数 声明
static bool led_init(LedHandle handle);
static void led_set(LedHandle handle, bool active);
static void led_toggle(LedHandle handle);

tLedDriverOps led_ops = {
    .init = led_init,
    .set = led_set,
    .toggle = led_toggle,
};
// LED 句柄创建销毁
LedHandle led_create(uint8_t led_id)
{
    tLed_ctx *handle = (tLed_ctx *)calloc(1, sizeof(tLed_ctx));
    if (NULL == handle)
        return NULL;
    if (led_id >= LED_NUM)
        return NULL; // 检查LED ID是否在有效范围内
    handle->led_id = led_id;
    return handle;
}
void led_destroy(LedHandle handle)
{
    if (NULL != handle)
        free(handle);
    handle = NULL;
}

// LED 驱动操作函数 定义
static bool led_init(LedHandle handle)
{
    if (NULL == handle)
        return false;

    tLed_ctx *ctx = (tLed_ctx *)handle;
    if (ctx->led_id >= LED_NUM)
    {
        return false;
    } // 检查LED ID是否在有效范围内
    // 初始化LED硬件-但是HAL库下 通过cubemx GPIO_Init()函数已经初始化了
    return true;
}

static void led_set(LedHandle handle, bool active)
{
    if (NULL == handle)
        return false;
    tLed_ctx *ctx = (tLed_ctx *)handle;

    switch (ctx->led_id)
    {
    case 0:
        bsp_led_0_set_pin(active ^= !LED0_ACTIVE);
        break;
    case 1:
        bsp_led_1_set_pin(active ^= !LED1_ACTIVE);
        break;
    default:
        break;
    }
}

static void led_toggle(LedHandle handle)
{
    if (NULL == handle)
        return false;
    tLed_ctx *ctx = (tLed_ctx *)handle;

    switch (ctx->led_id)
    {
    case 0:
        bsp_led_0_toggle_pin();
        break;
    case 1:
        bsp_led_1_toggle_pin();
        break;
    default:
        break;
    }
}
