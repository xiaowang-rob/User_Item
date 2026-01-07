#include "parameter_manager.h"
#include "string.h"
#include "flashDr.h"
#include "foc_core.h"
#include "math_fast.h"
Parameter_t g_Param;

void Param_set(Parameter_e para, u8 *value)
{
    switch (para)
    {
    // u8类型参数
    case SW_CANQUEUE:
        g_Param.sw_canqueue = *(u8 *)value;
        break;
    case SW_WEAKMAG:
        g_Param.sw_weakmag = *(u8 *)value;
        break;
    case SW_FAN:
        g_Param.sw_fan = *(u8 *)value;
        break;
    case SW_VAGUE_PID:
        g_Param.sw_vague_pid = *(u8 *)value;
        break;
    case SW_PVT:
        g_Param.sw_pvt = *(u8 *)value;
        break;
    case FOC_MODE:
        g_Param.foc_mode = *(u8 *)value;
        break;
    case LOOP_MODE:
        g_Param.loop_mode = *(u8 *)value;
        break;
    case AUTOTUNE_MODE:
        g_Param.autotune_mode = *(u8 *)value;
        break;
    case MOTOR_POLEPAIRS:
        g_Param.motor_polepairs = *(u8 *)value;
        break;

    // u32类型参数
    case CAN_ID:
        g_Param.can_id = *(uint32_t *)value;
        break;
    case f_CURRENT_LOOP:
        g_Param.f_current_loop = *(uint32_t *)value;
        break;
    case f_SPEED_LOOP:
        g_Param.f_speed_loop = *(uint32_t *)value;
        break;
    case f_POSITION_LOOP:
        g_Param.f_position_loop = *(uint32_t *)value;
        break;

    // float类型参数
    case THETA_OFFSET:
        g_Param.theta_offset = *(float *)value;
        break;
    case MOTOR_RS:
        g_Param.motor_rs = *(float *)value;
        break;
    case MOTOR_LS:
        g_Param.motor_ls = *(float *)value;
        break;
    case MOTOR_Psif:
        g_Param.motor_psif = *(float *)value;
        break;
    case MOTOR_Ke:
        g_Param.motor_ke = *(float *)value;
        break;
    case MOTOR_J:
        g_Param.motor_j = *(float *)value;
        break;
    case MOTOR_B:
        g_Param.motor_b = *(float *)value;
        break;
    case Kp_CURRENT:
        g_Param.kp_current = *(float *)value;
        break;
    case Ki_CURRENT:
        g_Param.ki_current = *(float *)value;
        break;
    case Kp_WEAKMAG:
        g_Param.kp_weakmag = *(float *)value;
        break;
    case Ki_WEAKMAG:
        g_Param.ki_weakmag = *(float *)value;
        break;
    case Kp_SPEED:
        g_Param.kp_speed = *(float *)value;
        break;
    case Ki_SPEED:
        g_Param.ki_speed = *(float *)value;
        break;
    case Kp_POSITION:
        g_Param.kp_position = *(float *)value;
        break;
    case Ki_POSITION:
        g_Param.ki_position = *(float *)value;
        break;
    case Kd_POSITION:
        g_Param.kd_position = *(float *)value;
        break;
    case LIMIT_CURRENT:
        g_Param.limit_current = *(float *)value;
        break;
    case LIMIT_SPEED:
        g_Param.limit_speed = *(float *)value;
        break;
    case LIMIT_POSITION_min:
        g_Param.limit_position_min = *(float *)value;
        break;
    case LIMIT_POSITION_max:
        g_Param.limit_position_max = *(float *)value;
        break;
    case TOLERANCE_TIME:
        g_Param.tolerance_time = *(float *)value;
        break;
    case TOLERANCE_VOLTAGE:
        g_Param.tolerance_voltage = *(float *)value;
        break;
    case TOLERANCE_CURRENT:
        g_Param.tolerance_current = *(float *)value;
        break;
    case TOLERANCE_SPEED:
        g_Param.tolerance_speed = *(float *)value;
        break;
    case TOLERANCE_POSITION:
        g_Param.tolerance_position = *(float *)value;
        break;
    case STARTUP_POS_GRAD:
        g_Param.startup_pos_grad = *(float *)value;
        break;
    case STARTUP_SPE_GRAD:
        g_Param.startup_spe_grad = *(float *)value;
        break;
    case ALIGN_CURRENT:
        g_Param.align_current = *(float *)value;
        break;
    case ALIGN_TIME:
        g_Param.align_time = *(float *)value;
        break;
    case OPEN_LOOP_CURRENT:
        g_Param.open_loop_current = *(float *)value;
        break;
    case OPEN_LOOP_SPEED:
        g_Param.open_loop_speed = *(float *)value;
        break;
    case CHANGE_LOOP_SPEED:
        g_Param.change_loop_speed = *(float *)value;
        break;

    default:
        break;
    }
}

void Param_get(Parameter_e para, u8 *value, u8 *len)
{
    switch (para)
    {
    // u8类型参数
    case SW_CANQUEUE:
        *(u8 *)value = g_Param.sw_canqueue;
        *len = sizeof(u8);
        break;
    case SW_WEAKMAG:
        *(u8 *)value = g_Param.sw_weakmag;
        *len = sizeof(u8);
        break;
    case SW_FAN:
        *(u8 *)value = g_Param.sw_fan;
        *len = sizeof(u8);
        break;
    case SW_VAGUE_PID:
        *(u8 *)value = g_Param.sw_vague_pid;
        *len = sizeof(u8);
        break;
    case SW_PVT:
        *(u8 *)value = g_Param.sw_pvt;
        *len = sizeof(u8);
        break;
    case FOC_MODE:
        *(u8 *)value = g_Param.foc_mode;
        *len = sizeof(u8);
        break;
    case LOOP_MODE:
        *(u8 *)value = g_Param.loop_mode;
        *len = sizeof(u8);
        break;
    case AUTOTUNE_MODE:
        *(u8 *)value = g_Param.autotune_mode;
        *len = sizeof(u8);
        break;
    case MOTOR_POLEPAIRS:
        *(u8 *)value = g_Param.motor_polepairs;
        *len = sizeof(u8);
        break;

    // u32类型参数
    case CAN_ID:
        *(uint32_t *)value = g_Param.can_id;
        *len = sizeof(uint32_t);
        break;
    case f_CURRENT_LOOP:
        *(uint32_t *)value = g_Param.f_current_loop;
        *len = sizeof(uint32_t);
        break;
    case f_SPEED_LOOP:
        *(uint32_t *)value = g_Param.f_speed_loop;
        *len = sizeof(uint32_t);
        break;
    case f_POSITION_LOOP:
        *(uint32_t *)value = g_Param.f_position_loop;
        *len = sizeof(uint32_t);
        break;

    // float类型参数
    case THETA_OFFSET:
        *(float *)value = g_Param.theta_offset;
        *len = sizeof(float);
        break;
    case MOTOR_RS:
        *(float *)value = g_Param.motor_rs;
        *len = sizeof(float);
        break;
    case MOTOR_LS:
        *(float *)value = g_Param.motor_ls;
        *len = sizeof(float);
        break;
    case MOTOR_Psif:
        *(float *)value = g_Param.motor_psif;
        *len = sizeof(float);
        break;
    case MOTOR_Ke:
        *(float *)value = g_Param.motor_ke;
        *len = sizeof(float);
        break;
    case MOTOR_J:
        *(float *)value = g_Param.motor_j;
        *len = sizeof(float);
        break;
    case MOTOR_B:
        *(float *)value = g_Param.motor_b;
        *len = sizeof(float);
        break;
    case Kp_CURRENT:
        *(float *)value = g_Param.kp_current;
        *len = sizeof(float);
        break;
    case Ki_CURRENT:
        *(float *)value = g_Param.ki_current;
        *len = sizeof(float);
        break;
    case Kp_WEAKMAG:
        *(float *)value = g_Param.kp_weakmag;
        *len = sizeof(float);
        break;
    case Ki_WEAKMAG:
        *(float *)value = g_Param.ki_weakmag;
        *len = sizeof(float);
        break;
    case Kp_SPEED:
        *(float *)value = g_Param.kp_speed;
        *len = sizeof(float);
        break;
    case Ki_SPEED:
        *(float *)value = g_Param.ki_speed;
        *len = sizeof(float);
        break;
    case Kp_POSITION:
        *(float *)value = g_Param.kp_position;
        *len = sizeof(float);
        break;
    case Ki_POSITION:
        *(float *)value = g_Param.ki_position;
        *len = sizeof(float);
        break;
    case Kd_POSITION:
        *(float *)value = g_Param.kd_position;
        *len = sizeof(float);
        break;
    case LIMIT_CURRENT:
        *(float *)value = g_Param.limit_current;
        *len = sizeof(float);
        break;
    case LIMIT_SPEED:
        *(float *)value = g_Param.limit_speed;
        *len = sizeof(float);
        break;
    case LIMIT_POSITION_min:
        *(float *)value = g_Param.limit_position_min;
        *len = sizeof(float);
        break;
    case LIMIT_POSITION_max:
        *(float *)value = g_Param.limit_position_max;
        *len = sizeof(float);
        break;
    case TOLERANCE_TIME:
        *(float *)value = g_Param.tolerance_time;
        *len = sizeof(float);
        break;
    case TOLERANCE_VOLTAGE:
        *(float *)value = g_Param.tolerance_voltage;
        *len = sizeof(float);
        break;
    case TOLERANCE_CURRENT:
        *(float *)value = g_Param.tolerance_current;
        *len = sizeof(float);
        break;
    case TOLERANCE_SPEED:
        *(float *)value = g_Param.tolerance_speed;
        *len = sizeof(float);
        break;
    case TOLERANCE_POSITION:
        *(float *)value = g_Param.tolerance_position;
        *len = sizeof(float);
        break;
    case STARTUP_POS_GRAD:
        *(float *)value = g_Param.startup_pos_grad;
        *len = sizeof(float);
        break;
    case STARTUP_SPE_GRAD:
        *(float *)value = g_Param.startup_spe_grad;
        *len = sizeof(float);
        break;
    case ALIGN_CURRENT:
        *(float *)value = g_Param.align_current;
        *len = sizeof(float);
        break;
    case ALIGN_TIME:
        *(float *)value = g_Param.align_time;
        *len = sizeof(float);
        break;
    case OPEN_LOOP_CURRENT:
        *(float *)value = g_Param.open_loop_current;
        *len = sizeof(float);
        break;
    case OPEN_LOOP_SPEED:
        *(float *)value = g_Param.open_loop_speed;
        *len = sizeof(float);
        break;
    case CHANGE_LOOP_SPEED:
        *(float *)value = g_Param.change_loop_speed;
        *len = sizeof(float);
        break;

    default:
        *len = 0;
        break;
    }
}
bool Param_read_flash()
{
    return FLASH_Read_data((u8 *)&g_Param, PARAMETER_LOAD_ADDr, sizeof(g_Param));
}
bool Param_write_flash()
{
    FLASH_erase_sector(PARAMETER_LOAD_ADDr, sizeof(g_Param));
    return FLASH_Write_Word((u8 *)&g_Param, PARAMETER_LOAD_ADDr, sizeof(g_Param));
}
bool Param_init()
{
    if (Param_read_flash() == false)
        return false;
    if (g_Param.none_flag != 0x01)
    { // flash中没有参数，初始化参数
        g_Param.none_flag = 0x01;
        // 初始化u8类型参数
        // 初始化u8类型参数
        g_Param.sw_canqueue = 0;     // CAN队列开关
        g_Param.sw_weakmag = 0;      // 弱磁开关
        g_Param.sw_fan = 0;          // 风扇
        g_Param.sw_vague_pid = 0;    // 模糊PID
        g_Param.sw_pvt = 0;          // PVT模式
        g_Param.foc_mode = 0;        // 运行模式
        g_Param.loop_mode = 0;       // 环模式
        g_Param.autotune_mode = 0;   // 自动调参模式
        g_Param.motor_polepairs = 7; // 电机转子对数

        // 初始化u32类型参数
        g_Param.can_id = 1;             // CAN ID
        g_Param.f_current_loop = 20000; // 电流环频率 20kHz
        g_Param.f_speed_loop = 2000;    // 速度环频率 2kHz
        g_Param.f_position_loop = 1000; // 位置环频率 1kHz

        // 初始化float类型参数
        g_Param.theta_offset = 0.0f; // 角度补偿
        g_Param.motor_rs = 0.05f;    // 电阻Rs 50mΩ
        g_Param.motor_ls = 0.0002f;  // 电感Ls 200μH
        g_Param.motor_psif = 0.01f;  // 磁链 0.01Wb
        g_Param.motor_ke = 0.01f;
        g_Param.motor_j = 0.0001f; // 转动惯量 0.0001 kg·m²
        g_Param.motor_b = 0.0005f; // 摩擦系数 0.0005 N·m·s/rad

        g_Param.kp_current = 0.5f;   // 电流环比例系数
        g_Param.ki_current = 100.0f; // 电流环积分系数
        g_Param.kp_weakmag = 0.5f;   // 弱磁环比例系数
        g_Param.ki_weakmag = 10.0f;  // 弱磁环积分系数
        g_Param.kp_speed = 0.1f;     // 速度环比例系数
        g_Param.ki_speed = 5.0f;     // 速度环积分系数
        g_Param.kp_position = 50.0f; // 位置环比例系数
        g_Param.ki_position = 1.0f;  // 位置环积分系数
        g_Param.kd_position = 0.1f;  // 位置环微分系数

        g_Param.limit_current = 50.0f;                    // 电流限幅 50A
        g_Param.limit_speed = rpm_to_rad(500.0f);         // 速度限幅 3000 RPM (假设转换后)
        g_Param.limit_position_min = deg_to_rad(-720.0f); // 最小位置限制 -10000度 (弧度)
        g_Param.limit_position_max = deg_to_rad(720.0f);  // 最大位置限制 10000度 (弧度)
        g_Param.tolerance_time = 0.1f;                    // 容忍时间 0.1秒
        g_Param.tolerance_voltage = 1.2f;                 // 电压容忍度 1.2
        g_Param.tolerance_current = 1.1f;                 // 电流容忍度 1.1
        g_Param.tolerance_speed = 1.1f;                   // 速度容忍度 1.1
        g_Param.tolerance_position = 1.1f;                // 位置容忍度 1.1

        g_Param.startup_pos_grad = deg_to_rad(10.0f);   // 启动位置梯度 10度/秒 (弧度)
        g_Param.startup_spe_grad = rpm_to_rad(1000.0f); // 启动速度梯度 1000 RPM/秒 (弧度/秒²)
        g_Param.align_current = 5.0f;                   // 对齐电流 5A
        g_Param.align_time = 0.5f;                      // 对齐时间 0.5秒
        g_Param.open_loop_current = 5.0f;               // 开环电流 5A
        g_Param.open_loop_speed = rpm_to_rad(150.0f);   // 开环速度 150 RPM (弧度/秒)
        g_Param.change_loop_speed = rpm_to_rad(100.0f); // 切环速度 100 RPM (弧度/秒) (弧度/秒)
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
