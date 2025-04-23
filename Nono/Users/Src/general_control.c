#include "general_control.h"
#include "string.h"
#include "tim.h"

#define First_Init() ESP32_Init()
#define Second_Init() RingBuffer_Init()
#define Third_Init() clock_init()


void ALL_Init()
{

  LCD_Init();

//  LCD_ShowString(40, 100, (uint8_t *)"ESP32 Initing ...... ", WHITE, BLACK, 12, 1);
//  if (First_Init())
//    LCD_ShowString(40, 100, (uint8_t *)"ESP32 Init success!", WHITE, BLACK, 12, 1);

  LCD_ShowString(40, 110, (uint8_t *)"RingBuffer Initing......", WHITE, BLACK, 12, 1);
  if (Second_Init())
    LCD_ShowString(40, 110, (uint8_t *)"RingBuffer Init success!", WHITE, BLACK, 12, 1);

  LCD_ShowString(40, 120, (uint8_t *)"Clock Initing......", WHITE, BLACK, 12, 1);
  if (Third_Init())
    LCD_ShowString(40, 120, (uint8_t *)"Clock Init success!", WHITE, BLACK, 12, 1);

  HAL_Delay(1000);
  LCD_Initshow();
}       