#ifndef __BASE_PARAMETERS_H
#define __BASE_PARAMETERS_H
#include "main.h"
#include "spi.h"
#define rate_CurrentSample 42.6749f // 电流采样 电流与电压比值

/*
编码器选择
MT6816---1
*/
#define ENcoder 1
#define ENcoderCS_CPIOx GPIO_A
#define ENcoderCS_CPIOx_PIN GPIO_PIN_15
#define ENcoder_SPI_Get_HSPI (hspi3)

#define FLASH_CS_CPIOx GPIO_A
#define FLASH_CS_CPIOx_PIN GPIO_PIN_8
#define FLASH_SPI_Get_HSPI (hspi2)

#endif // __BASE_PARAMETERS_H