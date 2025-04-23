#include "sensor.h"
#include "adc.h"
#include "tim.h"
#include "TFT.h"
uint16_t DMA_ADC[4];
/*
0 左光敏 1 右光敏 2 电池电量 3 MCU温度
*/
uint8_t power = 0;
uint8_t temp = 0;
uint8_t Sensor_Mode = 0;
uint16_t size = 16;

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        switch (Sensor_Mode)
        {
        case 0:
        {
            power = DMA_ADC[2] * 3300 / 4096;
            temp = (DMA_ADC[3] - 760) / 2.5 + 25;
            break;
        }
        case 1:
        {
            break;
        }
        default:
            break;
        }
    }
}

void Sersor_Read()
{
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)DMA_ADC, 4); // 开启ADC1的DMA传输
}

void Sensor_show_power_temp()
{
    LCD_ShowIntNum(10, 120 - size, power, 3, WHITE, BLACK, size);
    LCD_ShowIntNum(10, 120 + size / 2, temp, 3, WHITE, BLACK, size);
    LCD_ShowChar(10, 120 + size * 2, 'C', WHITE, BLACK, size, 1);
}
