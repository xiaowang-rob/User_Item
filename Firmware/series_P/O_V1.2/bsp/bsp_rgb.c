#include "bsp_rgb.h"
#include "tim.h"
#include "config.h"

/* ========== 64点正弦查找表 ========== */
static const u8 SINE_TABLE[64] = {
    128, 140, 153, 165, 177, 188, 199, 209,
    218, 226, 234, 240, 245, 250, 253, 254,
    254, 253, 250, 245, 240, 234, 226, 218,
    209, 199, 188, 177, 165, 153, 140, 128,
    128, 116, 103, 91, 79, 68, 57, 47,
    38, 30, 22, 16, 11, 6, 3, 2,
    2, 3, 6, 11, 16, 22, 30, 38,
    47, 57, 68, 79, 91, 103, 116, 128};

/* ========== 颜色常量 ========== */
const tRGBColor RED = {255, 0, 0};
const tRGBColor GREEN = {0, 255, 0};
const tRGBColor BLUE = {0, 0, 255};

const tRGBColor CHINA_RED = {230, 0, 0};        // 中国红
const tRGBColor KLEIN_BLUE = {0, 47, 167};      // 克莱因蓝
const tRGBColor MARS_GREEN = {6, 128, 67};      // 绿
const tRGBColor PRUSSIAN_BLUE = {0, 49, 83};    // 普鲁士蓝
const tRGBColor TIFFANY_BLUE = {129, 216, 208}; // 蒂夫尼蓝
const tRGBColor SKY = {35, 235, 185};           // 青绿
const tRGBColor ORANGE = {232, 88, 39};         // 品红

#define WS2812_BITS_PER_LED (24u)
#define WS2812_RESET_BITS (100u)
#define WS2812_NUM_LEDS (2u)

// === 关键修复：改用u32作为DMA缓冲区！ ===
#define WS2812_BUF_SIZE ((WS2812_NUM_LEDS * WS2812_BITS_PER_LED) + WS2812_RESET_BITS)

/*
 * 每个元素对应CCR寄存器的比较值
 * 因为HAL_TIM_PWM_Start_DMA需要把每个bit变成一个完整的PWM周期
 * 所以我们这里存储的是占空比值的编码：
 * CODE_1 → 逻辑1的占空比(75)
 * CODE_0 → 逻辑0的占空比(35)
 * 0      → 复位信号的低电平
 */
static u32 ws2812_dma_buf[WS2812_BUF_SIZE];

static tRGBColor led_cache[WS2812_NUM_LEDS];
static volatile bool dma_done = true;

#define BREATHE_INTERVAL_MS (40u)

static bool breath_active = false;
static u8 breath_idx = 0;
static u32 breath_next_ms = 0;
static tRGBColor breath_target = {0};

static void BuildStream(void)
{
    u32 idx = 0;

    // 编码规则：CODE_1=75表示该bit是高电平持续75个计数器周期
    static const u32 D_1 = CODE_1;
    static const u32 D_0 = CODE_0;

    for (u8 led = 0; led < WS2812_NUM_LEDS; led++)
    {
        for (u8 ch = 0; ch < 3; ch++)
        {
            u8 val = (ch == 0) ? led_cache[led].G : (ch == 1) ? led_cache[led].R
                                                              : led_cache[led].B;

            for (int bit = 7; bit >= 0; bit--)
            { // MSB first
                ws2812_dma_buf[idx++] = ((val >> bit) & 0x01) ? D_1 : D_0;
            }
        }
    }

    // 追加重置脉冲
    for (u32 i = 0; i < WS2812_RESET_BITS; i++)
    {
        ws2812_dma_buf[idx++] = 0;
    }
}

static void StartDMA(void)
{
    while (!dma_done)
    {
        __NOP();
    }
    dma_done = false;

    HAL_TIM_PWM_Start_DMA(&RGB_PWM_GET_HTIM, RGB_PWM_CHANNEL1,
                          (u32 *)ws2812_dma_buf, WS2812_BUF_SIZE);
}

void BSP_RGBInit(void)
{
    for (u32 i = 0; i < WS2812_BUF_SIZE; i++)
        ws2812_dma_buf[i] = 0;

    breath_active = false;
    breath_next_ms = HAL_GetTick() + BREATHE_INTERVAL_MS;

    BuildStream();
    StartDMA();
}

bool BSP_RGB_SetAllColor(tRGBColor color)
{
    if (breath_active)
        return false;

    HAL_TIM_PWM_Stop_DMA(&RGB_PWM_GET_HTIM, RGB_PWM_CHANNEL1);

    for (u8 i = 0; i < WS2812_NUM_LEDS; i++)
        led_cache[i] = color;
    BuildStream();
    StartDMA();
    return true;
}

void BSP_RGB_Breathe(tRGBColor Color)
{
    if (!breath_active)
    {
        breath_target = Color;
        breath_idx = 0;
        breath_next_ms = HAL_GetTick() + BREATHE_INTERVAL_MS;
        breath_active = true;
    }

    u32 now = HAL_GetTick();
    if (now >= breath_next_ms)
    {
        breath_next_ms = now + BREATHE_INTERVAL_MS;

        u8 bright = SINE_TABLE[breath_idx & 0x3F];

        tRGBColor cur = {
            .R = (u8)((u32)Color.R * bright / 255),
            .G = (u8)((u32)Color.G * bright / 255),
            .B = (u8)((u32)Color.B * bright / 255)};

        bool changed = (cur.R != led_cache[0].R ||
                        cur.G != led_cache[0].G ||
                        cur.B != led_cache[0].B);

        if (changed)
        {
            for (u8 i = 0; i < WS2812_NUM_LEDS; i++)
                led_cache[i] = cur;
            BuildStream();
            StartDMA();
        }

        breath_idx++;
    }
}
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == RGB_PWM_GET_HTIM.Instance)
    {
        dma_done = true;
        HAL_TIM_PWM_Stop_DMA(htim, RGB_PWM_CHANNEL1);
    }
}