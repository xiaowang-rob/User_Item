#include "clock.h"
#include "tft.h"
#include "rtc.h"
#include "TFT_init.h"
#include "bt_wifi_voice.h"
RTC_TimeTypeDef RTC_Time; // 时间数据结构体变量?
RTC_DateTypeDef RTC_Date;

/******************************************************************************
      函数说明：时钟数据初始化
      入口数据：Time 储存时间日期的结构体
      返回值： 无
******************************************************************************/
bool clock_init()
{
    clock_calibration();
    return true;
}
/******************************************************************************
      函数说明：定时用WiFi获取UTF时间对rtc时钟结构体校验?
      入口数据：Time 储存时间日期的结构体
      返回值：  无
******************************************************************************/
void clock_calibration()
{
    ESP32_SntpSetRtc();	
}
/******************************************************************************
      函数说明：显示时间
      入口数据：Time 储存时间日期的结构体
      返回值：  无
******************************************************************************/
void clock_run_time()
{
    uint16_t refresh = RTC_Time.Minutes;
    HAL_RTC_GetDate(&hrtc, &RTC_Date, RTC_FORMAT_BIN);
    HAL_RTC_GetTime(&hrtc, &RTC_Time, RTC_FORMAT_BIN);
    if (RTC_Time.Minutes != refresh)
    {
        LCD_Initshow();
    }
    LCD_ShowIntNum(40, 88, RTC_Time.Hours, 2, BLACK, WHITE, 64);
    LCD_ShowChar(104, 88, (uint8_t)':', BLUE, WHITE, 64, 1);
		LCD_ShowIntNum(136, 88, RTC_Time.Minutes/10, 1, BLACK, WHITE, 64);
    LCD_ShowIntNum(168, 88, RTC_Time.Minutes%10, 1, BLACK, WHITE, 64);
}
/******************************************************************************
      函数说明：日期显示?
      入口数据：Time 储存时间日期的结构体
      返回值：  无
******************************************************************************/
void clock_run_date()
{
    HAL_RTC_GetDate(&hrtc, &RTC_Date, RTC_FORMAT_BIN);
    HAL_RTC_GetTime(&hrtc, &RTC_Time, RTC_FORMAT_BIN);
    LCD_ShowIntNum(72, 24, 20, 2, BLACK, WHITE, 32);
    LCD_ShowIntNum(104, 24, RTC_Date.Year, 2, BLACK, WHITE, 32);
    LCD_ShowChinese32x32(136, 24, (uint8_t *)"年", BLUE, WHITE, 32, 1);
    LCD_ShowIntNum(56, 152, RTC_Date.Month, 2, BLACK, WHITE, 32);
    LCD_ShowChinese32x32(88, 152, (uint8_t *)"月", BLUE, WHITE, 32, 1);
    LCD_ShowIntNum(120, 152, RTC_Date.Date, 2, BLACK, WHITE, 32);
    LCD_ShowChinese32x32(152, 152, (uint8_t *)"日", BLUE, WHITE, 32, 1);
}
/******************************************************************************
      函数说明：秒的显示?
      入口数据：Time 储存时间日期的结构体
      返回值：   无
******************************************************************************/
void clock_run_second()
{
    uint16_t refresh = RTC_Time.Seconds;
    HAL_RTC_GetDate(&hrtc, &RTC_Date, RTC_FORMAT_BIN);
    HAL_RTC_GetTime(&hrtc, &RTC_Time, RTC_FORMAT_BIN);
    if (RTC_Time.Seconds != refresh)
    {
        LCD_Initshow();
    }
    LCD_ShowIntNum(88, 88, RTC_Time.Seconds, 2, BLACK, WHITE, 64);
}