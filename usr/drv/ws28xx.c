#include "led.h"
#include "bsp_rgb.h"
#include <string.h>

// RGB 配置表
#define WS28xx_NUM (2u)
#define WS28xx_BITS_PER_LED (24u)
#define WS28xx_RESET_BITS (100u)

#define WS28xx_BUF_SIZE ((WS28xx_NUM * WS28xx_BITS_PER_LED) + WS28xx_RESET_BITS)
static uint32_t ws28xx_dma_buf[WS28xx_BUF_SIZE] = {0};
static volatile bool dma_done = true;

typedef struct
{
    volatile bool breath_active;
    uint8_t rgb_idx;
    tRGBColor color;
    float brightness; // 亮度控制

    uint32_t code_1;
    uint32_t code_0;

} tWs28xx_ctx;
// 回调函数
static void pwm_finished_cb(void)
{
    dma_done = true;
    bsp_rgb_pwm_stop_dma();
}

// 内部函数
static inline void BuildStream(tWs28xx_ctx *ctx)
{
    uint32_t idx = 0;
    const uint16_t reset_bit_start = WS28xx_NUM * WS28xx_BITS_PER_LED;
    for (uint8_t ch = 0; ch < 3; ch++)
    {
        uint8_t val = (ch == 0) ? ctx->color.G * ctx->brightness : (ch == 1) ? ctx->color.R * ctx->brightness
                                                                             : ctx->color.B * ctx->brightness;

        for (int bit = 7; bit >= 0; bit--)
        { // MSB first
            ws28xx_dma_buf[idx++] = ((val >> bit) & 0x01) ? ctx->code_1 : ctx->code_0;
        }
    }

    // 追加重置脉冲
    memset(&ws28xx_dma_buf[reset_bit_start], 0, WS28xx_RESET_BITS);
}

static inline void StartDMA(void)
{
    if (!dma_done)
        return; // DMA正在运行，不重复启动
    dma_done = false;

    bsp_rgb_pwm_start_dma(ws28xx_dma_buf, WS28xx_BUF_SIZE);
}

// 函数操作表
static bool ws28xx_init(RgbHandle handle);
static void ws28xx_set_rgb(RgbHandle handle, tRGBColor color);
static void ws28xx_set_brightness(RgbHandle handle, uint8_t brightness);
static void ws28xx_refresh(RgbHandle handle);

// 每个元素对应CCR寄存器的比较值
// 因为HAL_TIM_PWM_Start_DMA需要把每个bit变成一个完整的PWM周期
// 所以我们这里存储的是占空比值的编码：
// CODE_1 → 逻辑1的占空比(75)
// CODE_0 → 逻辑0的占空比(35)
// 0      → 复位信号的低电平

tRgbDriverOps rgb_ops = {
    .init = ws28xx_init,
    .set_rgb = ws28xx_set_rgb,
    .set_brightness = ws28xx_set_brightness,
    .refresh = ws28xx_refresh,
};

RgbHandle ws28xx_create(uint8_t rgb_idx)
{
    tWs28xx_ctx *handle = (tWs28xx_ctx *)malloc(sizeof(tWs28xx_ctx));
    if (NULL == handle)
        return NULL;
    if (WS28xx_NUM <= rgb_idx)
        return NULL;           // 超出范围了
    handle->rgb_idx = rgb_idx; // 保存分配的索引
    return handle;
}
void ws28xx_destroy(RgbHandle handle)
{
    if (NULL != handle)
        free(handle);
    handle = NULL;
}

static bool ws28xx_init(RgbHandle handle)
{
    tWs28xx_ctx *ctx = (tWs28xx_ctx *)handle;
    if (NULL == ctx)
        return false;
    // 初始化DMA缓冲区
    ctx->breath_active = false;
    ctx->code_1 = CODE_1;
    ctx->code_0 = CODE_0;
    ctx->color.B = 0;
    ctx->color.G = 0;
    ctx->color.R = 0;
    ctx->rgb_idx = 0;

    for (uint32_t i = ctx->rgb_idx * WS28xx_BITS_PER_LED; i < (ctx->rgb_idx + 1) * WS28xx_BITS_PER_LED; i++)
        ws28xx_dma_buf[i] = 0;

    bsp_rgb_register_callback(pwm_finished_cb);
    return true; // 初始化成功
}
static void ws28xx_set_rgb(RgbHandle handle, tRGBColor color)
{
    tWs28xx_ctx *ctx = (tWs28xx_ctx *)handle;
    if (NULL == ctx)
        return;
    ctx->color = color;
}
static void ws28xx_set_brightness(RgbHandle handle, uint8_t brightness)
{
    tWs28xx_ctx *ctx = (tWs28xx_ctx *)handle;
    if (NULL == ctx)
        return;
    ctx->brightness = (float)brightness / 255.0;
}
static void ws28xx_refresh(RgbHandle handle)
{
    tWs28xx_ctx *ctx = (tWs28xx_ctx *)handle;
    if (NULL == ctx)
        return;
    BuildStream(ctx);
    StartDMA();
}
