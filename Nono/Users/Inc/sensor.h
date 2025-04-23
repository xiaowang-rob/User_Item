/*
ADC1  12bits 4095 对应 3.3V
IN0 --- PA0 --- 左边温度传感器

IN1 --- PA1 --- 右边温度传感器

IN15 --- PC5 --- 电池电量

IN16 --- 内部温度传感器
*/
#include "main.h"
#include "stdbool.h"

void Sersor_Read();
void Sensor_show_power_temp();
