#include "expression.h"
#include "TFT.h"
#include "cmsis_os.h"
#include "math.h"

#define EYE1_X_MidPoint 80
#define EYE2_X_MidPoint 160
#define EYE1_Y_MidPoint 100
#define EYE2_Y_MidPoint 100

uint8_t EXPRESSION_STYLE = 0;
/*
0--卡姿兰大圆眼睛
        a=10  b=20
        (x-x0)^2/a^2+(y-y0)^2/b^2=1
        x1=((1-(y-y0)^2/b^2)*a^2)^0.5+x0
        x2=-((1-(y-y0)^2/b^2)*a^2)^0.5+x0
*/
uint8_t a = 20;
uint8_t b = 46;
/*
1--眯眯眼 可爱

*/
uint8_t width = 46;
uint8_t thickness = 10;
void EYE_MODE(uint16_t color)
{
    switch (EXPRESSION_STYLE)
    {
    case 0:
    {
        EXPRESSION_STYLE = 0;
        break;
    }
    case 1:
    {
        EXPRESSION_STYLE = 1;
        break;
    }
    default:
        break;
    }
}
void EYE_OPEN(uint16_t color, uint8_t speed)
{
    uint8_t spe = 0;
    if (speed > 10)
        spe = 10;
    else
        spe = 10 - speed;

    switch (EXPRESSION_STYLE)
    {
    case 0:
    {
        for (int y = 0; y <= b; y++)
        {
            float x1str = -sqrt((1 - pow(y, 2) / pow(b, 2)) * pow(a, 2)) + EYE1_X_MidPoint;
            float x2str = -sqrt((1 - pow(y, 2) / pow(b, 2)) * pow(a, 2)) + EYE2_X_MidPoint;
            uint8_t width = (EYE1_X_MidPoint - x1str) * 2;
            for (int x = 0; x <= width; x++)
            {
                LCD_DrawPoint(x1str + x, EYE1_Y_MidPoint + y, color);
                LCD_DrawPoint(x2str + x, EYE2_Y_MidPoint + y, color);
                LCD_DrawPoint(x1str + x, EYE1_Y_MidPoint - y, color);
                LCD_DrawPoint(x2str + x, EYE2_Y_MidPoint - y, color);
            }
            osDelay(spe);
        }
        break;
    }
    case 1:
    {
			LCD_Fill(0,40,240,121,BLACK);
			LCD_Fill(70,110,150,210,BLACK);
        for (int i = 0; i < thickness; i++)
        {
            // 眼睛
            LCD_DrawLine(EYE1_X_MidPoint - width, EYE1_Y_MidPoint - width / 2 - i, EYE1_X_MidPoint + width / 4, EYE1_Y_MidPoint - i / 2 + 2, color);
            LCD_DrawLine(EYE2_X_MidPoint + width, EYE2_Y_MidPoint - width / 2 - i, EYE2_X_MidPoint - width / 4, EYE2_Y_MidPoint - i / 2 + 2, color);
						LCD_DrawLine(EYE1_X_MidPoint - width, EYE1_Y_MidPoint + width / 5 + i, EYE1_X_MidPoint + width / 4, EYE1_Y_MidPoint + i / 2 - 2, color);
            LCD_DrawLine(EYE2_X_MidPoint + width, EYE2_Y_MidPoint + width / 5 + i, EYE2_X_MidPoint - width / 4, EYE1_Y_MidPoint + i / 2 - 2, color);


            // 腮红
            LCD_DrawLine(EYE1_X_MidPoint - width + i / 5, EYE1_Y_MidPoint + width / 2 + 4, EYE1_X_MidPoint - width + i / 5, EYE1_Y_MidPoint + width - 4, RED);
            LCD_DrawLine(EYE1_X_MidPoint - width + 10 + i / 5, EYE1_Y_MidPoint + width / 2, EYE1_X_MidPoint - width + 10 + i / 5, EYE1_Y_MidPoint + width, RED);
            LCD_DrawLine(EYE1_X_MidPoint - width + 20 + i / 5, EYE1_Y_MidPoint + width / 2, EYE1_X_MidPoint - width + 20 + i / 5, EYE1_Y_MidPoint + width, RED);
            LCD_DrawLine(EYE1_X_MidPoint - width + 30 + i / 5, EYE1_Y_MidPoint + width / 2, EYE1_X_MidPoint - width + 30 + i / 5, EYE1_Y_MidPoint + width - 4, RED);

            LCD_DrawLine(EYE2_X_MidPoint + width - i / 5, EYE2_Y_MidPoint + width / 2 + 4, EYE2_X_MidPoint + width - i / 5, EYE2_Y_MidPoint + width - 4, RED);
            LCD_DrawLine(EYE2_X_MidPoint + width - 10 - i / 5, EYE2_Y_MidPoint + width / 2, EYE2_X_MidPoint + width - 10 - i / 5, EYE2_Y_MidPoint + width, RED);
            LCD_DrawLine(EYE2_X_MidPoint + width - 20 - i / 5, EYE2_Y_MidPoint + width / 2, EYE2_X_MidPoint + width - 20 - i / 5, EYE2_Y_MidPoint + width, RED);
            LCD_DrawLine(EYE2_X_MidPoint + width - 30 - i / 5, EYE2_Y_MidPoint + width / 2, EYE2_X_MidPoint + width - 30 - i / 5, EYE2_Y_MidPoint + width - 4, RED);
            // 嘴
	
					Draw_Circle(120,160,width /3+i/3,color);
            
        }

        break;
    }
    default:
        break;
    }
}
void EYE_CLOSE(uint16_t color, uint8_t speed)
{
    uint8_t spe = 0;
    if (speed > 10)
        spe = 10;
    else
        spe = 10 - speed;

    switch (EXPRESSION_STYLE)
    {
    case 0:
    {
        for (int y = b; y >= 4; y--)
        {
            uint8_t x1str = -a + EYE1_X_MidPoint;
            uint8_t x2str = -a + EYE2_X_MidPoint;
            for (int x = 0; x <= 2 * a; x++)
            {
                LCD_DrawPoint(x1str + x, EYE1_Y_MidPoint + y, BLACK);
                LCD_DrawPoint(x2str + x, EYE2_Y_MidPoint + y, BLACK);
                LCD_DrawPoint(x1str + x, EYE1_Y_MidPoint - y, BLACK);
                LCD_DrawPoint(x2str + x, EYE2_Y_MidPoint - y, BLACK);

            }
            osDelay(spe);
        }
        break;
    }
    case 1:
    {
			LCD_Fill(0,40,240,121,BLACK);
			LCD_Fill(70,110,150,210,BLACK);
        for (int i = 0; i < thickness ; i++)
        {
					
            // 眼睛
            LCD_DrawLine(EYE1_X_MidPoint - width, EYE1_Y_MidPoint - width / 6 - i / 3, EYE1_X_MidPoint + width / 4, EYE1_Y_MidPoint - i / 4 - 2, color);
            LCD_DrawLine(EYE2_X_MidPoint + width, EYE2_Y_MidPoint - width / 6 - i / 3, EYE2_X_MidPoint - width / 4, EYE2_Y_MidPoint - i / 4 - 2, color);
            
            LCD_DrawLine(EYE1_X_MidPoint - width, EYE1_Y_MidPoint + width / 8 + i / 3, EYE1_X_MidPoint + width / 4, EYE1_Y_MidPoint + i / 4 + 2, color);
            LCD_DrawLine(EYE2_X_MidPoint + width, EYE2_Y_MidPoint + width / 8 + i / 3, EYE2_X_MidPoint - width / 4, EYE1_Y_MidPoint + i / 4 + 2, color);
            // 腮红
            LCD_DrawLine(EYE1_X_MidPoint - width + i / 5, EYE1_Y_MidPoint + width / 2 + 4, EYE1_X_MidPoint - width + i / 5, EYE1_Y_MidPoint + width - 4, RED);
            LCD_DrawLine(EYE1_X_MidPoint - width + 10 + i / 5, EYE1_Y_MidPoint + width / 2, EYE1_X_MidPoint - width + 10 + i / 5, EYE1_Y_MidPoint + width, RED);
            LCD_DrawLine(EYE1_X_MidPoint - width + 20 + i / 5, EYE1_Y_MidPoint + width / 2, EYE1_X_MidPoint - width + 20 + i / 5, EYE1_Y_MidPoint + width, RED);
            LCD_DrawLine(EYE1_X_MidPoint - width + 30 + i / 5, EYE1_Y_MidPoint + width / 2, EYE1_X_MidPoint - width + 30 + i / 5, EYE1_Y_MidPoint + width - 4, RED);

            LCD_DrawLine(EYE2_X_MidPoint + width - i / 5, EYE2_Y_MidPoint + width / 2 + 4, EYE2_X_MidPoint + width - i / 5, EYE2_Y_MidPoint + width - 4, RED);
            LCD_DrawLine(EYE2_X_MidPoint + width - 10 - i / 5, EYE2_Y_MidPoint + width / 2, EYE2_X_MidPoint + width - 10 - i / 5, EYE2_Y_MidPoint + width, RED);
            LCD_DrawLine(EYE2_X_MidPoint + width - 20 - i / 5, EYE2_Y_MidPoint + width / 2, EYE2_X_MidPoint + width - 20 - i / 5, EYE2_Y_MidPoint + width, RED);
            LCD_DrawLine(EYE2_X_MidPoint + width - 30 - i / 5, EYE2_Y_MidPoint + width / 2, EYE2_X_MidPoint + width - 30 - i / 5, EYE2_Y_MidPoint + width - 4, RED);
            // 嘴
					
            LCD_DrawLine(120 - width / 3, 160 + i / 2, 120 + width / 3, 160 + i / 2, color);
        }
        break;
    }
    default:
        break;
    }
}
void EYE_BLINK(uint16_t color, uint8_t speed)
{
    EYE_OPEN(color, speed);
    osDelay(2000);
    EYE_CLOSE(color, speed);
    osDelay(500);
}