#ifndef __BASE_PARAMETERS_H
#define __BASE_PARAMETERS_H
#include "main.h"
#include "spi.h"

/*
编码器选择
MT6816---1
*/
#define ENcoder 1
#define ENcoderCS_CPIOx GPIOA
#define ENcoderCS_CPIOx_PIN GPIO_PIN_15
#define ENcoder_SPI_Get_HSPI (hspi3)
/*Flash spi*/
#define FLASH_CS_CPIOx GPIOA
#define FLASH_CS_CPIOx_PIN GPIO_PIN_8
#define FLASH_SPI_Get_HSPI (hspi2)
/*RGB LED*/
#define RGB_PWM_GET_HTIM (htim4)
#define RGB_PWM_CHANNEL TIM_CHANNEL_2
#define Pixel_NUM 2 // RGB数量宏定义

#define LED_ENCODER_GPIOx GPIOD
#define LED_ENCODER_GPIOx_PIN GPIO_PIN_2
#define LED_CANrx_GPIOx GPIOB
#define LED_CANrx_GPIOx_PIN GPIO_PIN_3

#endif // __BASE_PARAMETERS_H