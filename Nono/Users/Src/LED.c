#include "LED.h"
#include "cmsis_os.h"

uint8_t led_mode = 0;
/*正常-长短灯*/
void LED_nomal()
{
    red_set();
    osDelay(100);
    blue_set();
    osDelay(100);
    green_set();
    osDelay(100);
    orange_set();
    osDelay(500);
    orange_reset();
    osDelay(100);
    green_reset();
    osDelay(100);
    blue_reset();
    osDelay(100);
    red_reset();
    osDelay(500);
}
/*错误1*/
void LED_error1()
{
    red_set();
    osDelay(100);
    red_reset();
    osDelay(100);
}
/*led运行*/
void LED_run()
{
    switch (led_mode)
    {
    case 0:
        LED_nomal();
        break;
    case 1:
        LED_error1();
        break;
    default:
        break;
    }
}
/*led模式改变*/
void LED_mode(int mode)
{
    led_mode = mode;
}