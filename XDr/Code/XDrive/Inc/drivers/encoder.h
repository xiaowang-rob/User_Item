#ifndef ENCODER_H
#define ENCODER_H

#include "device.h"
#include "main.h"
#define ENCODER_SPI_CS_H() HAL_GPIO_WritePin(ENcoderCS_CPIOx, ENcoderCS_CPIOx_PIN, 1)
#define ENCODER_SPI_CS_L() HAL_GPIO_WritePin(ENcoderCS_CPIOx, ENcoderCS_CPIOx_PIN, 0)

typedef struct
{
    uint8_t state;   // 0 OK 1 OFFLINE 2 COMMUNICATION_FALUT 3 MAG_WEAK
    float angle_abs; // 弧度值
    float angle_last;
    float angle_inc; // 角度增量值rad
    float angle_deg; // 转换为角度值（0~360°）
    float omega;
    float angle_offset; // 角度偏移值
} ENCODER_t;

#if ENcoder == 1 // MT6816
// MT6816 寄存器地址定义
#define MT6816_REG_ANGLE_HIGH 0x03 // 角度高位寄存器
#define MT6816_REG_ANGLE_LOW 0x04  // 角度低位寄存器
#define MT6816_REG_STATUS 0x05     // 状态寄存器

// 状态位定义
#define MT6816_NO_MAG_WARNING (1 << 1) // 弱磁报警位
#define MT6816_PARITY_CHECK (1 << 0)   // 奇偶校验位

#endif

// 函数声明
bool ENCODER_Init(void);
float GET_ENCODER_ANGLE_ABS(void);
float GET_ENCODER_ANGLE_INC(void);
float GET_ENCODER_OMEGA(void);
void SET_ENCODER_ANGLE_OFFSET(float offset);
float GET_ENCODER_ANGLE_OFFSET(void);
uint8_t GET_ENCODER_STATUS();

#endif // ENCODER_H