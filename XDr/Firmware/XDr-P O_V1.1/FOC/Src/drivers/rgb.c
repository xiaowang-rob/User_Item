/***************************************************************************************************
 * @file    rgb.c
 * @brief   RGB LED 驱动 (修正版 - 使用uint32_t DMA缓冲区)
 ***************************************************************************************************/

#include "rgb.h"
#include "tim.h"

/* ========== 64点正弦查找表 ========== */
static const uint8_t SINE_TABLE[64] = {
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
const tRGBColor SKY = {0, 255, 255};
const tRGBColor MAGENTA = {255, 0, 220};
const tRGBColor YELLOW = {128, 216, 0};
const tRGBColor ORANGE = {127, 106, 0};
const tRGBColor BLACK = {0, 0, 0};
const tRGBColor WHITE = {255, 255, 255};

#define WS2812_BITS_PER_LED (24u)
#define WS2812_RESET_BITS (100u)
#define WS2812_NUM_LEDS (2u)

// === 关键修复：改用uint32_t作为DMA缓冲区！ ===
#define WS2812_BUF_SIZE ((WS2812_NUM_LEDS * WS2812_BITS_PER_LED) + WS2812_RESET_BITS)

/*
 * 每个元素对应CCR寄存器的比较值
 * 因为HAL_TIM_PWM_Start_DMA需要把每个bit变成一个完整的PWM周期
 * 所以我们这里存储的是占空比值的编码：
 * CODE_1 → 逻辑1的占空比(75)
 * CODE_0 → 逻辑0的占空比(35)
 * 0      → 复位信号的低电平
 */
static uint32_t ws2812_dma_buf[WS2812_BUF_SIZE];

static tRGBColor led_cache[WS2812_NUM_LEDS];
static volatile bool dma_done = true;

#define BREATHE_INTERVAL_MS (40u)

static bool breath_active = false;
static uint8_t breath_idx = 0;
static uint32_t breath_next_ms = 0;
static tRGBColor breath_target = {0};

static void BuildStream(void)
{
    uint32_t idx = 0;

    // 编码规则：CODE_1=75表示该bit是高电平持续75个计数器周期
    static const uint32_t D_1 = CODE_1;
    static const uint32_t D_0 = CODE_0;

    for (uint8_t led = 0; led < WS2812_NUM_LEDS; led++)
    {
        for (uint8_t ch = 0; ch < 3; ch++)
        {
            uint8_t val = (ch == 0) ? led_cache[led].G : (ch == 1) ? led_cache[led].R
                                                                   : led_cache[led].B;

            for (int bit = 7; bit >= 0; bit--)
            { // MSB first
                ws2812_dma_buf[idx++] = ((val >> bit) & 0x01) ? D_1 : D_0;
            }
        }
    }

    // 追加重置脉冲
    for (uint32_t i = 0; i < WS2812_RESET_BITS; i++)
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
                          (uint32_t *)ws2812_dma_buf, WS2812_BUF_SIZE);
}

void fRGB_Init(void)
{
    for (uint32_t i = 0; i < WS2812_BUF_SIZE; i++)
        ws2812_dma_buf[i] = 0;

    // 先设绿色测试
    for (uint8_t i = 0; i < WS2812_NUM_LEDS; i++)
        led_cache[i] = GREEN;

    breath_active = false;
    breath_next_ms = HAL_GetTick() + BREATHE_INTERVAL_MS;

    BuildStream();
    StartDMA();
}

bool fRGB_SetAllColor(tRGBColor color)
{
    if (breath_active)
        return false;

    HAL_TIM_PWM_Stop_DMA(&RGB_PWM_GET_HTIM, RGB_PWM_CHANNEL1);

    for (uint8_t i = 0; i < WS2812_NUM_LEDS; i++)
        led_cache[i] = color;
    BuildStream();
    StartDMA();
    return true;
}

void fRGB_Breathe(tRGBColor Color)
{
    if (!breath_active)
    {
        breath_target = Color;
        breath_idx = 0;
        breath_next_ms = HAL_GetTick() + BREATHE_INTERVAL_MS;
        breath_active = true;
    }

    uint32_t now = HAL_GetTick();
    if (now >= breath_next_ms)
    {
        breath_next_ms = now + BREATHE_INTERVAL_MS;

        uint8_t bright = SINE_TABLE[breath_idx & 0x3F];

        tRGBColor cur = {
            .R = (uint8_t)((uint32_t)Color.R * bright / 255),
            .G = (uint8_t)((uint32_t)Color.G * bright / 255),
            .B = (uint8_t)((uint32_t)Color.B * bright / 255)};

        bool changed = (cur.R != led_cache[0].R ||
                        cur.G != led_cache[0].G ||
                        cur.B != led_cache[0].B);

        if (changed)
        {
            for (uint8_t i = 0; i < WS2812_NUM_LEDS; i++)
                led_cache[i] = cur;
            BuildStream();
            StartDMA();
        }

        breath_idx++;
    }
}

void fRGB_Stop(void)
{
    breath_active = false;
    fRGB_SetAllColor(BLACK);
}

/* ========== GPIO LED ========== */
void fLED_CanTogglePin(void)
{
    HAL_GPIO_TogglePin(LED_CANrx_GPIOx, LED_CANrx_GPIOx_PIN);
}

void fLED_EncoderTogglePin(void)
{
    HAL_GPIO_TogglePin(LED_ENCODER_GPIOx, LED_ENCODER_GPIOx_PIN);
}

void fLED_Show(eLED_State can_state, eLED_State encoder_state)
{
    static uint32_t led_base_time = 0;
    static bool half_blink_flag = false;

    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - led_base_time;

    if (elapsed >= 500)
    {
        switch (can_state)
        {
        case LED_OFF:
            HAL_GPIO_WritePin(LED_CANrx_GPIOx, LED_CANrx_GPIOx_PIN, GPIO_PIN_RESET);
            break;
        case LED_ON:
            HAL_GPIO_WritePin(LED_CANrx_GPIOx, LED_CANrx_GPIOx_PIN, GPIO_PIN_SET);
            break;
        default:
            fLED_CanTogglePin();
            break;
        }
        led_base_time = now;
        half_blink_flag = false;
    }

    if (encoder_state == LED_FAST_BLINK && elapsed >= 200 && !half_blink_flag)
    {
        fLED_EncoderTogglePin();
        half_blink_flag = true;
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