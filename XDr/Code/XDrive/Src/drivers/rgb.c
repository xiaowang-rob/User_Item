#include "rgb.h"
#include "tim.h"
#include "stdlib.h"
#include "base_parameters.h"

/*Some Static Colors------------------------------*/
const RGB_Color_TypeDef RED = {255, 0, 0};       // 红色
const RGB_Color_TypeDef GREEN = {0, 255, 0};     // 绿色
const RGB_Color_TypeDef BLUE = {0, 0, 255};      // 深蓝色
const RGB_Color_TypeDef SKY = {0, 255, 255};     // 天蓝色
const RGB_Color_TypeDef MAGENTA = {255, 0, 220}; // 粉色
const RGB_Color_TypeDef YELLOW = {128, 216, 0};  // 黄色
const RGB_Color_TypeDef OEANGE = {127, 106, 0};  // 橘色
const RGB_Color_TypeDef BLACK = {0, 0, 0};       // 无颜色
const RGB_Color_TypeDef WHITE = {255, 255, 255}; // 白色

// 将好看的颜色封装成数组，便于集中管理和访问
RGB_Color_TypeDef table[16] =
    {
        {254, 67, 101},
        {76, 0, 10},
        {249, 15, 173},
        {128, 0, 32},
        {158, 46, 36},
        {184, 206, 142},
        {227, 23, 13},
        {178, 34, 34},
        {255, 99, 71},
        {99, 38, 18},
        {255, 97, 0},
        {21, 161, 201},
        {56, 94, 15},
        {50, 205, 50},
        {160, 32, 240},
        {218, 60, 90}};
// 这些是好看的颜色
const RGB_Color_TypeDef color1 = {254, 67, 101};
// const RGB_Color_TypeDef color2 = {76,0,10};
// const RGB_Color_TypeDef color3 = {249,15,173};
// const RGB_Color_TypeDef color4 = {128,0,32};
// const RGB_Color_TypeDef color5 = {158,46,36};
// const RGB_Color_TypeDef color6 = {184,206,142};
// const RGB_Color_TypeDef color7 = {227,23,13};
// const RGB_Color_TypeDef color8 = {178,34,34};
// const RGB_Color_TypeDef color9 = {255,99,71};
// const RGB_Color_TypeDef color10 ={99,38,18};
// const RGB_Color_TypeDef color11= {255,97,0};
// const RGB_Color_TypeDef color12= {21,161,201};
// const RGB_Color_TypeDef color13= {56,94,15};
// const RGB_Color_TypeDef color14= {50,205,50};
// const RGB_Color_TypeDef color15= {160,32,240};
// const RGB_Color_TypeDef color16= {218,60,90};

/*二维数组存放最终PWM输出数组，每一行24个数据代表一个LED，最后一行24个0用于复位*/
u32 Pixel_Buf[Pixel_NUM + 1][24];

/*
功能：最后一行装在24个0，输出24个周期占空比为0的PWM波，作为最后reset延时，这里总时长为24*1.25=37.5us > 24us(要求大于24us)
//如果出现无法复位的情况，只需要在增加数组Pixel_Buf[Pixel_NUM+1][24]的行数，并改写Reset_Load即可，这里不做演示了，
*/
static void Reset_Load(void)
{
    u8 i;
    for (i = 0; i < 24; i++)
    {
        Pixel_Buf[Pixel_NUM][i] = 0;
    }
}

/*
功能：发送数组Pixel_Buf[Pixel_NUM+1][24]内的数据，发送的数据被存储到定时器1通道1的CCR寄存器，用于控制PWM占空比
参数：(&htim1)定时器1，(TIM_CHANNEL_1)通道1，((u32 *)Pixel_Buf)待发送数组，
            (Pixel_NUM+1)*24)发送个数，数组行列相乘
*/
static void RGB_SendArray(void)
{
    HAL_TIM_PWM_Start_DMA(&RGB_PWM_GET_HTIM, RGB_PWM_CHANNEL, (u32 *)Pixel_Buf, (Pixel_NUM + 1) * 24);
}

/*
功能：设定单个RGB LED的颜色，把结构体中RGB的24BIT转换为0码和1码
参数：LedId为LED序号，Color：定义的颜色结构体
*/
// 刷新WS2812B灯板显示函数
static void RGB_Flush(void)
{
    Reset_Load();    // 复位
    RGB_SendArray(); // 发送数据
}

void RGB_SetOne_Color(u8 LedId, RGB_Color_TypeDef Color)
{
    u8 i;
    if (LedId > Pixel_NUM)
        return; // avoid overflow 防止写入ID大于LED总数
    // 这里是对 Pixel_Buf[LedId][i]写入一个周期内高电平的持续时间（或者说时PWM的占空比寄存器CCR1），
    for (i = 0; i < 8; i++)
        Pixel_Buf[LedId][i] = (((Color.G / 5) & (1 << (7 - i))) ? (CODE_1) : CODE_0); // 数组某一行0~7转化存放G
    for (i = 8; i < 16; i++)
        Pixel_Buf[LedId][i] = (((Color.R / 5) & (1 << (15 - i))) ? (CODE_1) : CODE_0); // 数组某一行8~15转化存放R
    for (i = 16; i < 24; i++)
        Pixel_Buf[LedId][i] = (((Color.B / 5) & (1 << (23 - i))) ? (CODE_1) : CODE_0); // 数组某一行16~23转化存放B
}

// 调用RGB_SetOne_Color函数，完成对多个LED的颜色设置。
void RGB_SetMore_Color(u8 head, u8 heal, RGB_Color_TypeDef color)
{
    u8 i = 0;
    for (i = head; i <= heal; i++)
    {
        RGB_SetOne_Color(i, color);
    }
}

#define RGB_BREATHE_rate 0.01f
RGB_Color_TypeDef color_temp;
u8 up_flag = 1;
float color_index = 0;
void rgb_breathe(RGB_Color_TypeDef Color)
{
    color_temp.R = Color.R * color_index;
    color_temp.G = Color.G * color_index;
    color_temp.B = Color.B * color_index;
    if (up_flag == 1)
    {
        if (color_index <= 1.0f)
            color_index += RGB_BREATHE_rate;
        else
        {
            up_flag = 0;
        }
    }
    else
    {
        if (color_index >= 0.0f)
            color_index -= RGB_BREATHE_rate;
        else
        {
            up_flag = 1;
        }
    }
    RGB_SetMore_Color(0, Pixel_NUM, color_temp);
    RGB_Flush(); // 刷新WS2812B的显示
}
#define RGB_ALTERNATE_steps 100
static u16 _tic_steps = 0;
void rgb_alternate(RGB_Color_TypeDef Color1, RGB_Color_TypeDef Color2)
{
    if (_tic_steps == RGB_ALTERNATE_steps)
    {
        RGB_SetMore_Color(0, Pixel_NUM, Color1);
        RGB_Flush(); // 刷新WS2812B的显示
    }
    if (_tic_steps == (RGB_ALTERNATE_steps * 2))
    {
        _tic_steps = 0;
        RGB_SetMore_Color(0, Pixel_NUM, Color2);
        RGB_Flush(); // 刷新WS2812B的显示
    }
    _tic_steps++;
}
void rgb_3_alternate(RGB_Color_TypeDef Color1, RGB_Color_TypeDef Color2, RGB_Color_TypeDef Color3)
{
    if (_tic_steps == RGB_ALTERNATE_steps)
    {
        RGB_SetMore_Color(0, Pixel_NUM, Color1);
        RGB_Flush(); // 刷新WS2812B的显示
    }
    if (_tic_steps == (RGB_ALTERNATE_steps * 2))
    {
        RGB_SetMore_Color(0, Pixel_NUM, Color2);
        RGB_Flush(); // 刷新WS2812B的显示
    }
    if (_tic_steps == (RGB_ALTERNATE_steps * 3))
    {
        _tic_steps = 0;
        RGB_SetMore_Color(0, Pixel_NUM, Color3);
        RGB_Flush(); // 刷新WS2812B的显示
    }
    _tic_steps++;
}
void LED_ENCODER_EN(void)
{
    HAL_GPIO_WritePin(LED_ENCODER_GPIOx, LED_ENCODER_GPIOx_PIN, GPIO_PIN_SET);
}
void LED_ENCODER_DIS(void)
{
    HAL_GPIO_WritePin(LED_ENCODER_GPIOx, LED_ENCODER_GPIOx_PIN, GPIO_PIN_RESET);
}
void LED_CANrx_EN(void)
{
    HAL_GPIO_WritePin(LED_CANrx_GPIOx, LED_CANrx_GPIOx_PIN, GPIO_PIN_SET);
}
void LED_CANrx_DIS(void)
{
    HAL_GPIO_WritePin(LED_CANrx_GPIOx, LED_CANrx_GPIOx_PIN, GPIO_PIN_RESET);
}