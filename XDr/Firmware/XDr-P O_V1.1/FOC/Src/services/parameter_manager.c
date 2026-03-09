#include "parameter_manager.h"
#include "string.h"
#include "flashDr.h"
#include "foc_statemachine.h"
#include "math_fast.h"
#include "protection_manager.h"
#include "can_port.h"
#include "drive_parameters.h"

tParameter g_Param;

void fParamSet(eParameter para, u8 *value)
{
    float temp = 0;
    switch (para)
    {
    // u8类型参数
    case SENSOR_MODE:
        g_Param.sensor_mode = *(u8 *)value;
        break;
    case LOOP_MODE:
        g_Param.run_mode = *(u8 *)value;
        break;
    case CAN_MODE:
        g_Param.sw_canqueue = *(u8 *)value;
        break;
    case WEAKMAG_MODE:
        g_Param.sw_weakmag = *(u8 *)value;
        break;
    case VAGUE_PID_MODE:
        g_Param.sw_vague_pid = *(u8 *)value;
        break;
    case PVT_MODE:
        g_Param.sw_pvt = *(u8 *)value;
        break;
    case TRAJ_TYPE:
        g_Param.traj_type = *(u8 *)value;
        break;

    case MOTOR_WIRE_SEQUENCE:
        g_Param.motor_wire_sequence = *(u8 *)value;
        break;
    case MOTOR_POLEPAIRS:
        g_Param.motor_polepairs = *(u8 *)value;
        break;
    case FREQ_CURRENT_LOOP:
        g_Param.freq_current_loop = *(u8 *)value;
        g_Param.f_current_loop = (float)fpwm / g_Param.freq_current_loop;
        break;
    case FREQ_SPEED_LOOP:
        g_Param.freq_speed_loop = *(u8 *)value;
        g_Param.f_speed_loop = g_Param.f_current_loop / g_Param.freq_speed_loop;
        break;
    case FREQ_POSITION_LOOP:
        g_Param.freq_position_loop = *(u8 *)value;
        g_Param.f_position_loop = g_Param.f_speed_loop / g_Param.freq_position_loop;
        break;
    // u32类型参数
    case CAN_ID:
        g_Param.can_id = *(u32 *)value;
        break;

        // float类型参数
    case THETA_OFFSET:
        g_Param.theta_offset = *(float *)value;
        break;
    case MOTOR_KV:
        g_Param.motor_kv = *(float *)value;
        break;
    case MOTOR_RS:
        g_Param.motor_rs = *(float *)value;
        break;
    case MOTOR_Ld:
        g_Param.motor_ld = *(float *)value;
        break;
    case MOTOR_Lq:
        g_Param.motor_lq = *(float *)value;
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
        g_Param.limit_omega = *(float *)value;
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
    case TRAJ_MAX_RATE:
        g_Param.traj_max_rate = *(float *)value;
        break;
    case TRAJ_MAX_ACC:
        g_Param.traj_max_acc = *(float *)value;
        break;
    case TRAJ_MAX_JERK:
        g_Param.traj_max_jerk = *(float *)value;
        break;
    case TRAJ_TOLERANCE:
        g_Param.tolerance = *(float *)value;
        break;
    default:
        break;
    }
    fParamWriteFOC();
}

void fParamGet(eParameter para, u8 *value, u8 *len)
{
    float temp = 0;
    switch (para)
    {
    // u8类型参数
    case SENSOR_MODE:
        *(u8 *)value = g_Param.sensor_mode;
        *len = sizeof(u8);
        break;
    case LOOP_MODE:
        *(u8 *)value = g_Param.run_mode;
        *len = sizeof(u8);
        break;
    case CAN_MODE:
        *(u8 *)value = g_Param.sw_canqueue;
        *len = sizeof(u8);
        break;
    case WEAKMAG_MODE:
        *(u8 *)value = g_Param.sw_weakmag;
        *len = sizeof(u8);
        break;
    case VAGUE_PID_MODE:
        *(u8 *)value = g_Param.sw_vague_pid;
        *len = sizeof(u8);
        break;
    case PVT_MODE:
        *(u8 *)value = g_Param.sw_pvt;
        *len = sizeof(u8);
        break;
    case TRAJ_TYPE:
        *(u8 *)value = g_Param.traj_type;
        *len = sizeof(u8);
        break;

    case MOTOR_WIRE_SEQUENCE:
        *(u8 *)value = g_Param.motor_wire_sequence;
        *len = sizeof(u8);
        break;

    case MOTOR_POLEPAIRS:
        *(u8 *)value = g_Param.motor_polepairs;
        *len = sizeof(u8);
        break;

    case FREQ_CURRENT_LOOP:
        *(u8 *)value = g_Param.freq_current_loop;
        *len = sizeof(u8);
        break;
    case FREQ_SPEED_LOOP:
        *(u8 *)value = g_Param.freq_speed_loop;
        *len = sizeof(u8);
        break;
    case FREQ_POSITION_LOOP:
        *(u8 *)value = g_Param.freq_position_loop;
        *len = sizeof(u8);
        break;
    // u32类型参数
    case CAN_ID:
        *(u32 *)value = g_Param.can_id;
        *len = sizeof(u32);
        break;

    // float类型参数
    case f_PWM:
        *(float *)value = fpwm;
        *len = sizeof(float);
        break;
    case f_CURRENT_LOOP:
        *(float *)value = g_Param.f_current_loop;
        *len = sizeof(float);
        break;
    case f_SPEED_LOOP:
        *(float *)value = g_Param.f_speed_loop;
        *len = sizeof(float);
        break;
    case f_POSITION_LOOP:
        *(float *)value = g_Param.f_position_loop;
        *len = sizeof(float);
        break;
    case THETA_OFFSET:
        *(float *)value = g_Param.theta_offset;
        *len = sizeof(float);
        break;
    case MOTOR_KV:
        *(float *)value = g_Param.motor_kv;
        *len = sizeof(float);
        break;
    case MOTOR_RS:
        *(float *)value = g_Param.motor_rs;
        *len = sizeof(float);
        break;
    case MOTOR_Ld:
        *(float *)value = g_Param.motor_ld;
        *len = sizeof(float);
        break;
    case MOTOR_Lq:
        *(float *)value = g_Param.motor_lq;
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
        *(float *)value = g_Param.limit_omega;
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

    case TRAJ_MAX_RATE:
        *(float *)value = g_Param.traj_max_rate;
        *len = sizeof(float);
        break;
    case TRAJ_MAX_ACC:
        *(float *)value = g_Param.traj_max_acc;
        *len = sizeof(float);
        break;
    case TRAJ_MAX_JERK:
        *(float *)value = g_Param.traj_max_jerk;
        *len = sizeof(float);
        break;
    case TRAJ_TOLERANCE:
        *(float *)value = g_Param.tolerance;
        *len = sizeof(float);
        break;

    default:
        *len = 0;
        break;
    }
}
bool _ParamReadFlash()
{
    return fFLASH_ReadData((u8 *)&g_Param, PARAMETER_LOAD_ADDr, sizeof(g_Param));
}
bool fParamSave()
{
    fFLASH_EraseSector(PARAMETER_LOAD_ADDr, sizeof(g_Param));
    return fFLASH_WriteWord((u8 *)&g_Param, PARAMETER_LOAD_ADDr, sizeof(g_Param));
}
bool fParamInit()
{
    if (_ParamReadFlash() == false)
        return false;
    if (g_Param.none_flag != 0x01)
    { // flash中没有参数，初始化参数
        g_Param.none_flag = 0x01;
        // 初始化u8类型参数
        // 初始化u8类型参数
        g_Param.sw_canqueue = 0;  // CAN队列开关
        g_Param.sw_weakmag = 0;   // 弱磁开关
        g_Param.sw_vague_pid = 0; // 模糊PID
        g_Param.sw_pvt = 0;       // PVT模式
        g_Param.traj_type = 0;    // 轨迹类型
        g_Param.sensor_mode = 0;  // 运行模式
        g_Param.run_mode = 0;     // 环模式

        g_Param.motor_wire_sequence = 0; // 电机线圈顺序
        g_Param.motor_polepairs = 14;    // 电机转子对数

        g_Param.freq_current_loop = 1;
        g_Param.freq_speed_loop = 4;
        g_Param.freq_position_loop = 5;

        // 初始化u32类型参数
        g_Param.can_id = 1; // CAN ID

        // 初始化float类型参数
        g_Param.f_pwm = fpwm;
        g_Param.f_current_loop = (float)fpwm / g_Param.freq_current_loop;
        g_Param.f_speed_loop = g_Param.f_current_loop / g_Param.freq_speed_loop;
        g_Param.f_position_loop = g_Param.f_speed_loop / g_Param.freq_position_loop;

        g_Param.theta_offset = 0.453290999f; // 角度补偿
        g_Param.motor_kv = 0.00;
        g_Param.motor_rs = 0.0218206495f; // 电阻Rs 50mΩ3.062550.0218206495
        g_Param.motor_lq = 0.00003f;      // 电感Ls 30μH
        g_Param.motor_ld = 0.00003f;
        g_Param.motor_psif = 0.01f; // 磁链 0.01Wb
        g_Param.motor_ke = 0.01f;
        g_Param.motor_j = 0.001f;  // 转动惯量 0.001 kg·m²
        g_Param.motor_b = 0.0005f; // 摩擦系数 0.0005 N·m·s/rad

        g_Param.kp_current = 0.2f;    // 电流环比例系数
        g_Param.ki_current = 40.0f;   // 电流环积分系数
        g_Param.kp_weakmag = 0.05f;   // 弱磁环比例系数
        g_Param.ki_weakmag = 40.f;    // 弱磁环积分系数
        g_Param.kp_speed = 3.0f;      // 速度环比例系数
        g_Param.ki_speed = 20.f;      // 速度环积分系数
        g_Param.kp_position = 0.5f;   // 位置环比例系数
        g_Param.ki_position = 0.05f;  // 位置环积分系数
        g_Param.kd_position = 0.005f; // 位置环微分系数

        g_Param.limit_current = 30.0f;                      // 电流限幅 50A
        g_Param.limit_omega = fRpmToRad(500.0f);            // 速度限幅 3000 RPM (假设转换后)
        g_Param.limit_position_min = fDegToRad(-100000.0f); // 最小位置限制 -10000度 (弧度)
        g_Param.limit_position_max = fDegToRad(100000.0f);  // 最大位置限制 10000度 (弧度)
        g_Param.tolerance_time = 0.1f;                      // 容忍时间 0.1秒
        g_Param.tolerance_voltage = 1.2f;                   // 电压容忍度 1.2
        g_Param.tolerance_current = 1.1f;                   // 电流容忍度 1.1
        g_Param.tolerance_speed = 1.1f;                     // 速度容忍度 1.1
        g_Param.tolerance_position = 1.1f;                  // 位置容忍度 1.1

        g_Param.traj_max_rate = 100.0f; // 最大变化率
        g_Param.traj_max_acc = 50.0f;   // 最大加速度
        g_Param.traj_max_jerk = 200.f;  // 最大加加速度
        g_Param.tolerance = 0.01f;      // 容差
    }
    return true;
}

void fParamErase()
{
    fEraseOneSector(PARAMETER_LOAD_ADDr);
}
// 写入FOC
void fParamWriteFOC()
{
    // 带参数写入的全部初始化
    fProManagerInit();
    fCAN_SetConfig(g_Param.can_id, g_Param.sw_canqueue);
    fFOC_Init();
}
