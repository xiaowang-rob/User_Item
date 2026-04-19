#ifndef __DEVICE_CONFIG_H
#define __DEVICE_CONFIG_H
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
#define FLASH_CS_CPIOx GPIOB
#define FLASH_CS_CPIOx_PIN GPIO_PIN_12
#define FLASH_SPI_Get_HSPI (hspi2)
/*RGB LED*/
#define RGB_PWM_GET_HTIM (htim4)
#define RGB_PWM_CHANNEL1 TIM_CHANNEL_2
#define Pixel_NUM 2 // RGB数量宏定义
#define CODE_1 (75)
#define CODE_0 (35)

#define LED_ENCODER_GPIOx GPIOD
#define LED_ENCODER_GPIOx_PIN GPIO_PIN_2
#define LED_CANrx_GPIOx GPIOB
#define LED_CANrx_GPIOx_PIN GPIO_PIN_3

/*FAN*/
#define FAN_GPIOx GPIOC
#define FAN_GPIOx_PIN GPIO_PIN_13

/*12V POWER*/
#define POWER12V_GPIOx GPIOC
#define POWER12V_GPIOx_PIN GPIO_PIN_13

/*USB cs pin*/
#define USB_CS_GPIOx GPIOA
#define USB_CS_GPIOx_PIN GPIO_PIN_8

// 外设状态
typedef enum
{
    OFFLINE,
    ONLINE,
    RUN_ERROR,
    RUNNING,
} eDeviceStatus;

typedef struct
{
    eDeviceStatus can_state;
    eDeviceStatus encoder_state;
    eDeviceStatus flash_state;
    eDeviceStatus usb_state;
} tDeviceStatus;

extern tDeviceStatus g_device_status;

#endif