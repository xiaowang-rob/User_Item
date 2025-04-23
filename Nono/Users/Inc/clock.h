
#include "main.h"
#include "stdbool.h"
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

bool clock_init();                                 // 获取WiFi时间对RTC_time结构体初始化
void clock_calibration(); // 固定时间对RTC time时间进行校准
bool clock_run_time(uint8_t size);                             // 显示时分
bool clock_run_date(uint8_t size);                             // 显示年月日周
bool clock_run_second(uint8_t size);                           // 显示秒
