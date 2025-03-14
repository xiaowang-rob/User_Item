#include "general_control.h"
#include "string.h"
#define First_Init() ESP32_Init()
#define Second_Init() RingBuffer_Init()
#define Third_Init() clock_init()


void ALL_Init()
{

    LCD_Init();
	
    LCD_ShowString(40, 100,(uint8_t *)"ESP32 Init ...", WHITE, BLACK, 12, 1);
    if (First_Init() == 1)
        LCD_ShowString(40, 100,  (uint8_t *)"ESP32 Init success!", WHITE, BLACK, 12, 1);
    else
        LCD_ShowString(40, 100, (uint8_t *)"ESP32 Init error X", WHITE, BLACK, 12, 1);
	
    LCD_ShowString(40, 110, (uint8_t *)"RingBuffer Init ...", WHITE, BLACK, 12, 1);
    if (Second_Init() == 1)
        LCD_ShowString(40, 110, (uint8_t *)"RingBuffer Init success!", WHITE, BLACK, 12, 1);
    else
        LCD_ShowString(40, 110,(uint8_t *) "RingBuffer Init error X", WHITE, BLACK, 12, 1);
		
		    LCD_ShowString(40, 120, (uint8_t *)"Clock Init ...", WHITE, BLACK, 12, 1);
    if (clock_init() == 1)
        LCD_ShowString(40, 120, (uint8_t *)"Clock Init success!", WHITE, BLACK, 12, 1);
    else
        LCD_ShowString(40, 120, (uint8_t *)"Clock Init error X", WHITE, BLACK, 12, 1);
		
    HAL_Delay(1000);
    LCD_Initshow();
}       