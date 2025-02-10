#include "clock.h"
#include "tft.h"
#include "rtc.h"
#include "TFT_init.h"
/******************************************************************************
      函数说明：时钟数据初始化
      入口数据：time 储存时间日期的结构体
      返回值：  无
******************************************************************************/
void clock_init(clock_data *time)
{
    RTC_TimeTypeDef time_standard;
    RTC_DateTypeDef date_standard;
    HAL_RTC_GetDate(&hrtc, &date_standard, RTC_FORMAT_BIN);
    HAL_RTC_GetTime(&hrtc, &time_standard, RTC_FORMAT_BIN);

    //    time->year = date_standard.Year;
    //    time->month = date_standard.Month;
    //    time->day = date_standard.Date;
    //    time->week = date_standard.WeekDay;
    //    time->hour = time_standard.Hours;
    //    time->minute = time_standard.Minutes;
    //    time->second = time_standard.Seconds;
}
/******************************************************************************
      函数说明：定时用WiFi获取UTF时间对rtc时钟结构体校准
      入口数据：time 储存时间日期的结构体
      返回值：  无
******************************************************************************/
void clock_calibration(clock_data *time)
{
}
/******************************************************************************
      函数说明：显示时分
      入口数据：time 储存时间日期的结构体
      返回值：  无
******************************************************************************/
void clock_run_time(clock_data *time)
{
    uint16_t i = time->minute;
    RTC_TimeTypeDef time_standard;
    HAL_RTC_GetTime(&hrtc, &time_standard, RTC_FORMAT_BIN);
    time->hour = time_standard.Hours;
    time->minute = time_standard.Minutes;
    if (time->second != i)
    {
        LCD_Initshow();
    }
    LCD_ShowIntNum(40, 88, time_standard.Hours, 2, WHITE, BLACK, 64);
    LCD_ShowChar(104, 88, (uint8_t)':', BLUE, BLACK, 64, 1);
    LCD_ShowIntNum(136, 88, time_standard.Minutes, 2, WHITE, BLACK, 64);
}
/******************************************************************************
      函数说明：日期显示
      入口数据：time 储存时间日期的结构体
      返回值：  无
******************************************************************************/
void clock_run_date(clock_data *time)
{
    RTC_DateTypeDef date_standard;
    HAL_RTC_GetDate(&hrtc, &date_standard, RTC_FORMAT_BIN);
    time->year = date_standard.Year;
    time->month = date_standard.Month;
    time->day = date_standard.Date;
    time->week = date_standard.WeekDay;
    LCD_ShowIntNum(24, 24, 20, 2, WHITE, BLACK, 64);
    LCD_ShowIntNum(88, 24, date_standard.Year, 2, WHITE, BLACK, 64);
    LCD_ShowChinese64x64(152, 24, (uint8_t *)"年", BLUE, BLACK, 64, 1);
    LCD_ShowIntNum(56, 88, time->month, 2, WHITE, BLACK, 64);
    LCD_ShowChinese64x64(120, 88, (uint8_t *)"月", BLUE, BLACK, 64, 1);
    LCD_ShowIntNum(56, 152, time->day, 2, WHITE, BLACK, 64);
    LCD_ShowChinese64x64(120, 152, (uint8_t *)"日", BLUE, BLACK, 64, 1);
}
/******************************************************************************
      函数说明：秒的显示
      入口数据：time 储存时间日期的结构体
      返回值：  无
******************************************************************************/
void clock_run_second(clock_data *time)
{
    uint16_t i = time->second;
    RTC_TimeTypeDef time_standard;
    RTC_DateTypeDef date_standard;
    HAL_RTC_GetDate(&hrtc, &date_standard, RTC_FORMAT_BIN);
    HAL_RTC_GetTime(&hrtc, &time_standard, RTC_FORMAT_BIN);
    time->second = time_standard.Seconds;
    if (time->second != i)
    {
        LCD_Initshow();
    }
    LCD_ShowIntNum(88, 88, time->second, 2, WHITE, BLACK, 64);
}