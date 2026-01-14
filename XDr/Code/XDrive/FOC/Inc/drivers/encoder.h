#ifndef ENCODER_H
#define ENCODER_H

#include "main.h"
#include "device.h"

typedef enum
{
    ENCODER_STATE_START_READ,  // 开始读取高位
    ENCODER_STATE_WAIT_HIGH,   // 等待高位完成
    ENCODER_STATE_WAIT_LOW,    // 等待低位完成
    ENCODER_STATE_PROCESS_DATA // 处理数据
} ENCODER_STATE_DMA;
typedef struct
{
    ENCODER_STATE_DMA state;
    float angle_abs; // 弧度值
    float angle_last;
    float angle_inc; // 角度增量值rad
    float angle_inc_last;
    float omega;
    float angle_offset; // 角度偏移值
    u32 last_time;      // 上次读取时间
    u32 time_T;         // 周期
    int num_turns;      // 转数
} ENCODER_t;

// 全局变量
static u16 reg03_cmd = 0x83ff;
static u16 reg03_data = 0; // 高位寄存器数据
static u16 reg04_cmd = 0x84ff;
static u16 reg04_data = 0; // 低位寄存器数据
static uint32_t transfer_start_time = 0;
static const uint32_t TRANSFER_TIMEOUT_MS = 2; // 2ms超时

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

void ENCODER_MainLoopTask();
float GET_ENCODER_ANGLE_ABS();
float GET_ENCODER_ANGLE_INC();
float GET_ENCODER_OMEGA();
void SET_ENCODER_ANGLE_OFFSET(float offset);
float GET_ENCODER_ANGLE_OFFSET();

#endif // ENCODER_H