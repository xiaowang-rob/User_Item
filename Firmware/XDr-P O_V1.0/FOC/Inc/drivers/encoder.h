#ifndef __ENCODER_H
#define __ENCODER_H

#include "main.h"
#include "device.h"

typedef enum
{
    ENCODER_STATE_START_READ,  // 开始读取高位
    ENCODER_STATE_WAIT_HIGH,   // 等待高位完成
    ENCODER_STATE_WAIT_LOW,    // 等待低位完成
    ENCODER_STATE_PROCESS_DATA // 处理数据
} eEncoderState_DMA;
typedef struct
{
    eEncoderState_DMA state;
    float angle_abs; // 弧度值
    float angle_last;
    float angle_inc; // 角度增量值rad
    float angle_inc_last;
    float omega;
    u32 last_time; // 上次读取时间
    u32 time_T;    // 周期
    int num_turns; // 转数
} tEncoder;

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

void fEncoderMainLoopTask();
float fGetEncoderAngle_ABS();
float fGetEncoderAngle_INC();
float fGetEncoderOmega();

#endif // __ENCODER_H