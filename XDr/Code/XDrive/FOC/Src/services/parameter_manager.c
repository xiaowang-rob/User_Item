#include "parameter_manager.h"
#include "string.h"
#include "flashDr.h"
#include "foc_core.h"
#include "math_fast.h"
Parameter_t Param;

void Param_set(Parameter_e para, u8 *value)
{
    if (para < U8_COUNT)
        memcpy(&Param.u8_data[para], value, sizeof(u8));
    else if (para < U32_COUNT)
        memcpy(&Param.u32_data[para - U8_COUNT - 1], value, sizeof(u32));
    else if (para < FLOAT_COUNT)
        memcpy(&Param.float_data[para - U32_COUNT - 1], value, sizeof(float));
}

void Param_get(Parameter_e para, u8 *value, u8 *len)
{
    if (para < U8_COUNT)
    {
        memcpy(value, &Param.u8_data[para], sizeof(u8));
        *len = sizeof(u8);
    }
    else if (para < U32_COUNT)
    {
        memcpy(value, &Param.u32_data[para - U8_COUNT - 1], sizeof(u32));
        *len = sizeof(u32);
    }
    else if (para < FLOAT_COUNT)
    {
        memcpy(value, &Param.float_data[para - U32_COUNT - 1], sizeof(float));
        *len = sizeof(float);
    }
}
bool Param_read_flash()
{
    return FLASH_Read_data((u8 *)&Param, PARAMETER_LOAD_ADDr, sizeof(Param));
}
bool Param_write_flash()
{
    FLASH_erase_sector(PARAMETER_LOAD_ADDr, sizeof(Param));
    return FLASH_Write_Word((u8 *)&Param, PARAMETER_LOAD_ADDr, sizeof(Param));
}
bool Param_init()
{
    if (Param_read_flash() == false)
        return false;
    if (Param.none_flag != 0x000000001)
    { // flash中没有参数，初始化参数
        Param.none_flag = 0x000000001;
        // 初始化u8类型参数
        Param.u8_data[SW_CANQUEUE] = 0;     // CAN队列开关
        Param.u8_data[SW_WEAKMAG] = 0;      // 弱磁开关
        Param.u8_data[SW_FAN] = 0;          // 风扇
        Param.u8_data[SW_VAGUE_PID] = 0;    // 模糊PID
        Param.u8_data[SW_PVT] = 0;          // PVT模式
        Param.u8_data[FOC_MODE] = 0;        // 运行模式
        Param.u8_data[LOOP_MODE] = 0;       // 环路模式
        Param.u8_data[AUTOTUNE_MODE] = 0;   // 自动调参模式
        Param.u8_data[MOTOR_POLEPAIRS] = 7; // 电机转子对数

        // 初始化u32类型参数
        Param.u32_data[CAN_ID - U8_COUNT - 1] = 1;             // CAN ID
        Param.u32_data[f_CURRENT_LOOP - U8_COUNT - 1] = 20000; // 电流环频率 20kHz
        Param.u32_data[f_SPEED_LOOP - U8_COUNT - 1] = 2000;    // 速度环频率 2kHz
        Param.u32_data[f_POSITION_LOOP - U8_COUNT - 1] = 1000; // 位置环频率 1kHz

        // 初始化float类型参数
        Param.float_data[THETA_OFFSET - U32_COUNT - 1] = 0.0f; // 角度补偿
        Param.float_data[MOTOR_RS - U32_COUNT - 1] = 0.05f;    // 电阻Rs 50mΩ
        Param.float_data[MOTOR_LS - U32_COUNT - 1] = 0.0002f;  // 电感Ls 200μH
        Param.float_data[MOTOR_Psif - U32_COUNT - 1] = 0.01f;  // 磁链 0.01Wb
        Param.float_data[MOTOR_J - U32_COUNT - 1] = 0.0001f;   // 转动惯量 0.0001 kg·m²
        Param.float_data[MOTOR_B - U32_COUNT - 1] = 0.0005f;   // 摩擦系数 0.0005 N·m·s/rad

        Param.float_data[Kp_CURRENT - U32_COUNT - 1] = 0.5f;   // 电流环比例系数
        Param.float_data[Ki_CURRENT - U32_COUNT - 1] = 100.0f; // 电流环积分系数
        Param.float_data[Kp_WEAKMAG - U32_COUNT - 1] = 0.5f;   // 弱磁环比例系数
        Param.float_data[Ki_WEAKMAG - U32_COUNT - 1] = 10.0f;  // 弱磁环积分系数
        Param.float_data[Kp_SPEED - U32_COUNT - 1] = 0.1f;     // 速度环比例系数
        Param.float_data[Ki_SPEED - U32_COUNT - 1] = 5.0f;     // 速度环积分系数
        Param.float_data[Kp_POSITION - U32_COUNT - 1] = 50.0f; // 位置环比例系数
        Param.float_data[Ki_POSITION - U32_COUNT - 1] = 1.0f;  // 位置环积分系数
        Param.float_data[Kd_POSITION - U32_COUNT - 1] = 0.1f;  // 位置环微分系数

        Param.float_data[LIMIT_CURRENT - U32_COUNT - 1] = 50.0f;                    // 电流限幅 50A
        Param.float_data[LIMIT_SPEED - U32_COUNT - 1] = rpm_to_rad(500.0f);         // 速度限幅 3000 RPM (假设转换后)
        Param.float_data[LIMIT_POSITION_min - U32_COUNT - 1] = deg_to_rad(-720.0f); // 最小位置限制 -10000度 (弧度)
        Param.float_data[LIMIT_POSITION_max - U32_COUNT - 1] = deg_to_rad(720.0f);  // 最大位置限制 10000度 (弧度)
        Param.float_data[TOLERANCE_TIME - U32_COUNT - 1] = 0.1f;                    // 容忍时间 0.1秒
        Param.float_data[TOLERANCE_VOLTAGE - U32_COUNT - 1] = 1.2f;                 // 电压容忍度 1.2
        Param.float_data[TOLERANCE_CURRENT - U32_COUNT - 1] = 1.1f;                 // 电流容忍度 1.1
        Param.float_data[TOLERANCE_SPEED - U32_COUNT - 1] = 1.1f;                   // 速度容忍度 1.1
        Param.float_data[TOLERANCE_POSITION - U32_COUNT - 1] = 1.1f;                // 位置容忍度 1.1

        Param.float_data[STARTUP_POS_GRAD - U32_COUNT - 1] = deg_to_rad(10.0f);   // 启动位置梯度 10度/秒 (弧度)
        Param.float_data[STARTUP_SPE_GRAD - U32_COUNT - 1] = rpm_to_rad(1000.0f); // 启动速度梯度 1000 RPM/秒 (弧度/秒²)
        Param.float_data[ALIGN_CURRENT - U32_COUNT - 1] = 5.0f;                   // 对齐电流 5A
        Param.float_data[ALIGN_TIME - U32_COUNT - 1] = 0.5f;                      // 对齐时间 0.5秒
        Param.float_data[OPEN_LOOP_CURRENT - U32_COUNT - 1] = 5.0f;               // 开环电流 5A
        Param.float_data[OPEN_LOOP_SPEED - U32_COUNT - 1] = rpm_to_rad(150.0f);   // 开环速度 150 RPM (弧度/秒)
        Param.float_data[CHANGE_LOOP_SPEED - U32_COUNT - 1] = rpm_to_rad(100.0f); // 切环速度 100 RPM (弧度/秒)
    }
}

bool Param_save()
{
    return Param_write_flash();
}

void Param_erase()
{
    Erase_one_Sector(PARAMETER_LOAD_ADDr);
}
// 写入FOC
bool Param_write_foc()
{
    // TODO:FOC重新初始化
}
