#include "clock.h"
#include "tft.h"
#include "rtc.h"
#include "TFT_init.h"
#include "bt_wifi_voice.h"
#include "sensor.h"
RTC_TimeTypeDef RTC_Time; // 时间数据结构体变量?
RTC_DateTypeDef RTC_Date;
uint16_t CLOCK_NUM_COLOR = WHITE;
uint16_t CLOCK_PUN_COLOR = BLUE;
uint8_t TIME_X = 120;
uint8_t TIME_Y = 160;
uint8_t time_flash_flag = 0;
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
bool clock_run_time(uint8_t size)
{
    uint16_t refresh = RTC_Time.Minutes;
    HAL_RTC_GetDate(&hrtc, &RTC_Date, RTC_FORMAT_BIN);
    HAL_RTC_GetTime(&hrtc, &RTC_Time, RTC_FORMAT_BIN);
    if (RTC_Time.Minutes == refresh && time_flash_flag == 1)
    {
        return 1;
    }
    time_flash_flag = 1;
    LCD_Fill(TIME_X - size / 2 * 5 / 2, TIME_Y - size / 2, TIME_X + size / 2 * 5 / 2, TIME_Y + size / 2, BLACK);
    LCD_ShowIntNum(TIME_X - size / 2 * 5 / 2, TIME_Y - size / 2, RTC_Time.Hours, 2, CLOCK_NUM_COLOR, WHITE, size);
    LCD_ShowChar(TIME_X - size / 4, TIME_Y - size / 2, (uint8_t)':', CLOCK_PUN_COLOR, WHITE, size, 1);
    LCD_ShowIntNum(TIME_X + size / 4, TIME_Y - size / 2, RTC_Time.Minutes / 10, 1, CLOCK_NUM_COLOR, WHITE, size);
    LCD_ShowIntNum(TIME_X + size / 2 * 3 / 2, TIME_Y - size / 2, RTC_Time.Minutes % 10, 1, CLOCK_NUM_COLOR, WHITE, size);
    Sensor_show_power_temp();
    Sersor_Read();

    return 0;
}
/******************************************************************************
      函数说明：日期显示?
      入口数据：Time 储存时间日期的结构体
      返回值：  无
******************************************************************************/
bool clock_run_date(uint8_t size)
{
    HAL_RTC_GetDate(&hrtc, &RTC_Date, RTC_FORMAT_BIN);
    HAL_RTC_GetTime(&hrtc, &RTC_Time, RTC_FORMAT_BIN);
    LCD_ShowIntNum(72, 24, 20, 2, BLACK, WHITE, 32);
    LCD_ShowIntNum(104, 24, RTC_Date.Year, 2, BLACK, WHITE, size);
    LCD_ShowChinese32x32(136, 24, (uint8_t *)"年", BLUE, WHITE, size, 1);
    LCD_ShowIntNum(56, 152, RTC_Date.Month, 2, BLACK, WHITE, size);
    LCD_ShowChinese32x32(88, 152, (uint8_t *)"月", BLUE, WHITE, size, 1);
    LCD_ShowIntNum(120, 152, RTC_Date.Date, 2, BLACK, WHITE, size);
    LCD_ShowChinese32x32(152, 152, (uint8_t *)"日", BLUE, WHITE, size, 1);
}
/******************************************************************************
      函数说明：秒的显示?
      入口数据：Time 储存时间日期的结构体
      返回值：   无
******************************************************************************/
bool clock_run_second(uint8_t size)
{
    uint16_t refresh = RTC_Time.Seconds;
    HAL_RTC_GetDate(&hrtc, &RTC_Date, RTC_FORMAT_BIN);
    HAL_RTC_GetTime(&hrtc, &RTC_Time, RTC_FORMAT_BIN);
    if (RTC_Time.Seconds != refresh)
    {
        LCD_Initshow();
    }
    LCD_ShowIntNum(88, 88, RTC_Time.Seconds, 2, CLOCK_NUM_COLOR, WHITE, size);
}
