/**
 ******************************************************************************
 * @file    rgb.c
 * @brief   RGB LED驱动模块实现文件
 * @details 实现WS2812B LED驱动和普通LED控制功能
 *          包括呼吸灯效果、颜色设置和状态显示
 ******************************************************************************
 * @note
 * - WS2812B使用PWM+DMA方式驱动，需要精确的时序控制
 * - 呼吸灯效果基于256点正弦表实现
 * - 支持单个和多个LED的颜色设置
 ******************************************************************************
 */

#include "rgb.h"
#include "tim.h"
#include "stdlib.h"
#include "device.h"
#include "math_fast.h"

/* 预定义颜色常量定义 -------------------------------------------------------*/

const tRGBColor RED = {255, 0, 0};       ///< 红色
const tRGBColor GREEN = {0, 255, 0};     ///< 绿色
const tRGBColor BLUE = {0, 0, 255};      ///< 深蓝色
const tRGBColor SKY = {0, 255, 255};     ///< 天蓝色
const tRGBColor MAGENTA = {255, 0, 220}; ///< 粉色
const tRGBColor YELLOW = {128, 216, 0};  ///< 黄色
const tRGBColor ORANGE = {127, 106, 0};  ///< 橙色
const tRGBColor BLACK = {0, 0, 0};       ///< 无颜色
const tRGBColor WHITE = {255, 255, 255}; ///< 白色

/* 全局变量定义 -----------------------------------------------------------*/

/**
 * @brief  PWM输出缓冲区
 * @note   二维数组，每一行24个数据代表一个LED的24位颜色数据
 *         最后一行24个0用于复位信号，保证>24us的低电平复位时间
 *         [LED数量+1][24] - 额外一行用于复位信号
 */
u32 rgb_pixel_buf[Pixel_NUM + 1][24];

/**
 * @brief  正弦波亮度表（256点）
 * @note   用于呼吸灯效果，预计算的sin(x)值，范围0-255
 *         对应sin(x)*127.5+127.5，中心值128
 */
static const u8 SINE_TABLE[256] = {
    128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 162, 165, 167, 170, 173,
    176, 179, 182, 185, 188, 190, 193, 196, 198, 201, 203, 206, 208, 211, 213, 215,
    218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 238, 240, 241, 243, 244,
    245, 246, 247, 248, 249, 250, 250, 251, 252, 252, 253, 253, 253, 254, 254, 254,
    254, 254, 254, 254, 253, 253, 253, 252, 252, 251, 250, 250, 249, 248, 247, 246,
    245, 244, 243, 241, 240, 238, 237, 235, 234, 232, 230, 228, 226, 224, 222, 220,
    218, 215, 213, 211, 208, 206, 203, 201, 198, 196, 193, 190, 188, 185, 182, 179,
    176, 173, 170, 167, 165, 162, 158, 155, 152, 149, 146, 143, 140, 137, 134, 131,
    128, 124, 121, 118, 115, 112, 109, 106, 103, 100, 97, 93, 90, 88, 85, 82,
    79, 76, 73, 70, 67, 65, 62, 59, 57, 54, 52, 49, 47, 44, 42, 40,
    37, 35, 33, 31, 29, 27, 25, 23, 21, 20, 18, 17, 15, 14, 12, 11,
    10, 9, 8, 7, 6, 5, 5, 4, 3, 3, 2, 2, 2, 1, 1, 1,
    1, 1, 1, 1, 2, 2, 2, 3, 3, 4, 5, 5, 6, 7, 8, 9,
    10, 11, 12, 14, 15, 17, 18, 20, 21, 23, 25, 27, 29, 31, 33, 35,
    37, 40, 42, 44, 47, 49, 52, 54, 57, 59, 62, 65, 67, 70, 73, 76,
    79, 82, 85, 88, 90, 93, 97, 100, 103, 106, 109, 112, 115, 118, 121, 124};

static tRGBBreath rgb_breathe_t;    ///< 呼吸灯控制结构体
static u32 _led_time_zero = 0;      ///< LED状态计时起始时间
static bool half_time_flag = false; ///< 快速闪烁中间切换标志

/* 静态函数声明 -----------------------------------------------------------*/
static void fResetLoad(void);
static void fRGB_SendArray(void);
static void fRGB_Flush(void);
static void fRGB_SetOneColor(u8 LedId, tRGBColor Color);

/**
 * @brief  加载复位信号到缓冲区
 * @note   在缓冲区最后一行填充24个0，产生>24us的低电平复位信号
 *         WS2812B要求RESET信号 >24us，这里为24*1.25=37.5us
 */
static void fResetLoad(void)
{
    for (u8 i = 0; i < 24; i++)
    {
        rgb_pixel_buf[Pixel_NUM][i] = 0; // 占空比为0，产生低电平
    }
}

/**
 * @brief  通过DMA发送PWM数据到定时器
 * @note   发送整个缓冲区数据到定时器的CCR寄存器，控制PWM占空比
 *         使用双通道同时发送，确保同步性
 */
static void fRGB_SendArray(void)
{
    // 启动DMA传输，将缓冲区数据发送到PWM通道
    HAL_TIM_PWM_Start_DMA(&RGB_PWM_GET_HTIM, RGB_PWM_CHANNEL1,
                          (u32 *)rgb_pixel_buf, (Pixel_NUM + 1) * 24);
    HAL_TIM_PWM_Start_DMA(&RGB_PWM_GET_HTIM, RGB_PWM_CHANNEL2,
                          (u32 *)rgb_pixel_buf, (Pixel_NUM + 1) * 24);
}

/**
 * @brief  刷新WS2812B显示
 * @note   先加载复位信号，然后发送所有数据到LED
 *         必须在设置完所有LED颜色后调用此函数才能更新显示
 */
static void fRGB_Flush(void)
{
    fResetLoad();     // 加载复位信号
    fRGB_SendArray(); // 发送数据到LED
}

/**
 * @brief  设置单个LED的颜色
 * @param  LedId: LED序号（0起始）
 * @param  Color: 要设置的颜色
 * @note   将24位RGB颜色数据转换为WS2812B的0码和1码时序
 *         WS2812B数据格式：G7-G0, R7-R0, B7-B0
 *         每个位对应一个PWM周期的占空比
 */
static void fRGB_SetOneColor(u8 LedId, tRGBColor Color)
{
    u8 i;

    // 防止数组越界
    if (LedId > Pixel_NUM)
        return;

    // 转换绿色分量（8位）到PWM占空比数组
    // CODE_1和CODE_0定义对应WS2812B的T1H/T0H时间
    for (i = 0; i < 8; i++)
        rgb_pixel_buf[LedId][i] = ((Color.G & (1 << (7 - i))) ? (CODE_1) : CODE_0);

    // 转换红色分量（8位）
    for (i = 8; i < 16; i++)
        rgb_pixel_buf[LedId][i] = ((Color.R & (1 << (15 - i))) ? (CODE_1) : CODE_0);

    // 转换蓝色分量（8位）
    for (i = 16; i < 24; i++)
        rgb_pixel_buf[LedId][i] = ((Color.B & (1 << (23 - i))) ? (CODE_1) : CODE_0);
}

/**
 * @brief  设置多个连续LED的颜色
 * @param  head: 起始LED序号
 * @param  heal: 结束LED序号
 * @param  color: 要设置的颜色
 * @note   循环调用fRGB_SetOneColor设置指定范围内的所有LED
 */
void fRGB_SetMoreColor(u8 head, u8 heal, tRGBColor color)
{
    u8 i = 0;
    for (i = head; i <= heal; i++)
    {
        fRGB_SetOneColor(i, color);
    }
}

/**
 * @brief  设置呼吸灯效果
 * @param  Color: 呼吸灯最大亮度时的目标颜色
 * @note   使用正弦波查表法实现亮度渐变
 *         每4ms更新一次亮度，减少不必要的刷新
 *         使用定点数运算提高效率
 */
void fRGB_Breathe(tRGBColor Color)
{
    // 限制更新频率为4ms一次（约250Hz）
    if (HAL_GetTick() - rgb_breathe_t.last_time_ms < 10)
        return;
    rgb_breathe_t.last_time_ms = HAL_GetTick();

    // 检查是否需要更新目标颜色（仅当颜色实际变化时）
    if (Color.R != rgb_breathe_t.target_color.R ||
        Color.G != rgb_breathe_t.target_color.G ||
        Color.B != rgb_breathe_t.target_color.B)
    {
        rgb_breathe_t.target_color = Color;
    }

    // 从正弦表获取当前亮度值（0-255）
    // 注意：SINE_TABLE的范围是0-255，但实际亮度曲线是正弦波
    uint8_t brightness = SINE_TABLE[rgb_breathe_t.sine_index];

    // 计算当前颜色 = 目标颜色 × 亮度 / 255
    // 使用32位中间变量防止溢出
    tRGBColor current = {
        .R = (uint8_t)(((uint32_t)rgb_breathe_t.target_color.R * brightness) / 255),
        .G = (uint8_t)(((uint32_t)rgb_breathe_t.target_color.G * brightness) / 255),
        .B = (uint8_t)(((uint32_t)rgb_breathe_t.target_color.B * brightness) / 255)};

    // 仅当颜色实际变化时才刷新LED（优化性能）
    if (current.R != rgb_breathe_t.last_output.R ||
        current.G != rgb_breathe_t.last_output.G ||
        current.B != rgb_breathe_t.last_output.B)
    {
        fRGB_SetOneColor(0, current);        // 设置第0个LED
        fRGB_Flush();                        // 刷新显示
        rgb_breathe_t.last_output = current; // 保存当前输出
    }

    // 更新正弦索引（循环0-255）
    rgb_breathe_t.sine_index = (rgb_breathe_t.sine_index + 1) & 0xFF;
}

/**
 * @brief  翻转编码器LED引脚状态
 * @note   用于实现LED闪烁效果
 */
void fLED_EncoderTogglePin(void)
{
    HAL_GPIO_TogglePin(LED_ENCODER_GPIOx, LED_ENCODER_GPIOx_PIN);
}

/**
 * @brief  翻转CAN LED引脚状态
 * @note   用于实现LED闪烁效果
 */
void fLED_CanTogglePin(void)
{
    HAL_GPIO_TogglePin(LED_CANrx_GPIOx, LED_CANrx_GPIOx_PIN);
}

/**
 * @brief  LED状态显示函数
 * @param  can_led_state: CAN通信LED状态
 * @param  encoder_led_state: 编码器LED状态
 * @note   根据状态控制LED显示模式：
 *         - 常亮/常灭：直接设置GPIO
 *         - 慢速闪烁：1秒周期翻转
 *         - 快速闪烁：0.5秒周期翻转，需要中间切换点
 *         使用定时器控制，保证精确的闪烁周期
 */
void fLED_Show(eLED_State can_led_state, eLED_State encoder_led_state)
{
    // 主计时：达到慢速闪烁周期（1秒）时执行
    if (HAL_GetTick() - _led_time_zero >= LED_SLOW_BLINK_T_ms)
    {
        // 处理CAN LED状态
        switch (can_led_state)
        {
        case LED_OFF:
            HAL_GPIO_WritePin(LED_CANrx_GPIOx, LED_CANrx_GPIOx_PIN, GPIO_PIN_RESET);
            break;
        case LED_ON:
            HAL_GPIO_WritePin(LED_CANrx_GPIOx, LED_CANrx_GPIOx_PIN, GPIO_PIN_SET);
            break;
        case LED_SLOW_BLINK:
            fLED_CanTogglePin(); // 1秒翻转一次
            break;
        case LED_FAST_BLINK:
            fLED_CanTogglePin(); // 这里也会翻转，结合下面的中间切换实现0.5秒周期
            break;
        default:
            break;
        }

        // 处理编码器LED状态
        switch (encoder_led_state)
        {
        case LED_OFF:
            HAL_GPIO_WritePin(LED_ENCODER_GPIOx, LED_ENCODER_GPIOx_PIN, GPIO_PIN_RESET);
            break;
        case LED_ON:
            HAL_GPIO_WritePin(LED_ENCODER_GPIOx, LED_ENCODER_GPIOx_PIN, GPIO_PIN_SET);
            break;
        case LED_SLOW_BLINK:
            fLED_EncoderTogglePin(); // 1秒翻转一次
            break;
        case LED_FAST_BLINK:
            fLED_EncoderTogglePin(); // 这里也会翻转
            break;
        default:
            break;
        }

        // 重置计时器和中间切换标志
        _led_time_zero = HAL_GetTick();
        half_time_flag = false;
    }

    // 中间计时：达到快速闪烁半周期（0.5秒）时执行
    if (HAL_GetTick() - _led_time_zero >= LED_FAST_BLINK_T_ms && !half_time_flag)
    {
        // 快速闪烁需要中间再翻转一次，实现0.5秒周期
        if (encoder_led_state == LED_FAST_BLINK)
            fLED_EncoderTogglePin();
        if (can_led_state == LED_FAST_BLINK)
            fLED_CanTogglePin();

        half_time_flag = true; // 设置标志，防止半周期内重复执行
    }
}