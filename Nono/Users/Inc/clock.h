
#include "main.h"

typedef struct TimeData
{

    uint16_t year;
    uint16_t month;
    uint16_t day;
	uint16_t week;
	
    uint16_t hour;
    uint16_t minute;
    uint16_t second;
    
} clock_data;

void clock_init(clock_data *time);                 // 获取WiFi时间对RTC_time结构体初始化
void clock_calibration(clock_data *Time_Standard); // 固定时间对RTC time时间进行校准
void clock_run_time(clock_data *time);             // 显示时分
void clock_run_date(clock_data *time);             // 显示年月日周
void clock_run_second(clock_data *time);           // 显示秒
