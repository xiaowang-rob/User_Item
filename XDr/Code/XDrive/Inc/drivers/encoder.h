#ifndef ENCODER_H
#define ENCODER_H

#include "base_parameters.h"
#define ENCODER_SPI_CS_H() HAL_GPIO_WritePin(ENcoderCS_CPIOx, ENcoderCS_CPIOx_PIN, 1)
#define ENCODER_SPI_CS_L() HAL_GPIO_WritePin(ENcoderCS_CPIOx, ENcoderCS_CPIOx_PIN, 0)

typedef struct
{
    bool online_flag;
    bool no_mag_flag;         // 磁场无效标志位
    bool communication_error; // 通信错误标志位
    float angle_rad;          // 弧度值
    float angle_last;
    float angle_inc;    // 角度增量值rad
    float angle_deg;    // 转换为角度值（0~360°）
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
float GET_ENCODER_ANGLE_RAD(void);
float GET_ENCODER_ANGLE_INC(void);
void SET_ENCODER_ANGLE_OFFSET(float offset);
float GET_ENCODER_ANGLE_OFFSET(void);
bool GET_ENCODER_STATUS();
bool GET_ENCODER_COMMUNICATION_ERROR(void);
bool GET_ENCODER_NO_MAG_FLAG(void);

#endif // ENCODER_H