#include "main.h"

/*定义灯亮灭*/
#define red_set() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
#define red_reset() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
#define blue_set() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
#define blue_reset() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
#define green_set() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
#define green_reset() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
#define orange_set() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
#define orange_reset() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);



/*正常-长短灯*/
void LED_nomal();
/*错误1*/
void LED_error1();
/*LED运行*/
void LED_run();
/*LED模式选择*/
void LED_mode(int mode);