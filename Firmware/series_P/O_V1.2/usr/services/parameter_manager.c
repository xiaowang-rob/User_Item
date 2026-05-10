#include "parameter_manager.h"
#include "string.h"
#include "foc_main.h"
#include "math_fast.h"
#include "protection_manager.h"
#include "can_port.h"
#include "usr_config.h"

tParameter g_Param;

void fParamSet(eParameter para, u8 *value)
{
    switch (para)
    {
    // u8类型参数
    case ENCODER_CHIP:
        g_Param.encoder_chip = *(u8 *)value;
        break;
    case SENSOR_MODE:
        g_Param.sensor_mode = *(u8 *)value;
        break;
    case RUN_MODE:
        g_Param.run_mode = *(u8 *)value;
        break;
    case CAN_MODE:
        g_Param.sw_canqueue = *(u8 *)value;
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

    case MOTOR_POLEPAIRS:
        g_Param.motor_polepairs = *(u8 *)value;
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
    case MOTOR_PSIF:
        g_Param.motor_psif = *(float *)value;
        break;
    case MOTOR_KE:
        g_Param.motor_ke = *(float *)value;
        break;
    case MOTOR_J:
        g_Param.motor_j = *(float *)value;
        break;
    case MOTOR_B:
        g_Param.motor_b = *(float *)value;
        break;
    case KP_SPEED:
        g_Param.kp_speed = *(float *)value;
        break;
    case KI_SPEED:
        g_Param.ki_speed = *(float *)value;
        break;
    case KP_POSITION:
        g_Param.kp_position = *(float *)value;
        break;
    case KI_POSITION:
        g_Param.ki_position = *(float *)value;
        break;
    case KD_POSITION:
        g_Param.kd_position = *(float *)value;
        break;
    case LIMIT_CURRENT:
        g_Param.limit_current = *(float *)value;
        break;
    case LIMIT_SPEED:
        g_Param.limit_omega = *(float *)value;
        break;
    case LIMIT_POSITION_MIN:
        g_Param.limit_position_min = *(float *)value;
        break;
    case LIMIT_POSITION_MAX:
        g_Param.limit_position_max = *(float *)value;
        break;
    case TOLERANCE_TIME:
        g_Param.tolerance_time = *(float *)value;
        break;
    case TOLERANCE_LIMIT:
        g_Param.tolerance_limit = *(float *)value;
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
    default: // 最后会发送一个 成功 反馈
        fFOC_ParamUpdate(&g_Param);
        fCAN_SetConfig(g_Param.can_id, g_Param.sw_canqueue);
        fProManagerInit(&g_Param);
        break;
    }
}

void fParamGet(eParameter para, u8 *value, u8 *len)
{
    switch (para)
    {
    // u8类型参数
    case ENCODER_CHIP:
        *(u8 *)value = g_Param.encoder_chip;
        *len = sizeof(u8);
        break;
    case SENSOR_MODE:
        *(u8 *)value = g_Param.sensor_mode;
        *len = sizeof(u8);
    case RUN_MODE:
        *(u8 *)value = g_Param.run_mode;
        *len = sizeof(u8);
        break;
    case CAN_MODE:
        *(u8 *)value = g_Param.sw_canqueue;
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

    case MOTOR_POLEPAIRS:
        *(u8 *)value = g_Param.motor_polepairs;
        *len = sizeof(u8);
        break;

    // u32类型参数
    case CAN_ID:
        *(u32 *)value = g_Param.can_id;
        *len = sizeof(u32);
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
    case MOTOR_PSIF:
        *(float *)value = g_Param.motor_psif;
        *len = sizeof(float);
        break;
    case MOTOR_KE:
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
    case KP_SPEED:
        *(float *)value = g_Param.kp_speed;
        *len = sizeof(float);
        break;
    case KI_SPEED:
        *(float *)value = g_Param.ki_speed;
        *len = sizeof(float);
        break;
    case KP_POSITION:
        *(float *)value = g_Param.kp_position;
        *len = sizeof(float);
        break;
    case KI_POSITION:
        *(float *)value = g_Param.ki_position;
        *len = sizeof(float);
        break;
    case KD_POSITION:
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
    case LIMIT_POSITION_MIN:
        *(float *)value = g_Param.limit_position_min;
        *len = sizeof(float);
        break;
    case LIMIT_POSITION_MAX:
        *(float *)value = g_Param.limit_position_max;
        *len = sizeof(float);
        break;
    case TOLERANCE_TIME:
        *(float *)value = g_Param.tolerance_time;
        *len = sizeof(float);
        break;
    case TOLERANCE_LIMIT:
        *(float *)value = g_Param.tolerance_limit;
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
    if (g_Param.none_flag != 0x0f)
    { // flash中没有参数，初始化参数
        g_Param.none_flag = 0x0f;
        // 初始化u8类型参数
        // 初始化u8类型参数
        g_Param.sw_canqueue = 0;  // CAN队列开关
        g_Param.sw_vague_pid = 0; // 模糊PID
        g_Param.sw_pvt = 0;       // PVT模式
        g_Param.traj_type = 0;    // 轨迹类型
        g_Param.sensor_mode = 0;  // 运行模式
        g_Param.run_mode = 0;     // 环模式

        g_Param.motor_polepairs = 14; // 电机转子对数
        g_Param.theta_elec_offset = false;

        // 初始化u32类型参数
        g_Param.can_id = 1; // CAN ID

        // 初始化float类型参数

        g_Param.theta_offset = 0.0f; // 角度补偿
        g_Param.motor_kv = 100;
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

        g_Param.limit_current = 30.0f;           // 电流限幅 50A
        g_Param.limit_omega = 500.0f;            // 速度限幅 3000 RPM (假设转换后)
        g_Param.limit_position_min = -100000.0f; // 最小位置限制 -10000度 (弧度)
        g_Param.limit_position_max = 100000.0f;  // 最大位置限制 10000度 (弧度)
        g_Param.tolerance_time = 0.1f;           // 容忍时间 0.1ms
        g_Param.tolerance_limit = 1.1f;          // 容忍度

        g_Param.traj_max_rate = 0.0f; // 最大变化率
        g_Param.traj_max_acc = 0.0f;  // 最大加速度
        g_Param.traj_max_jerk = 0.0f; // 最大加加速度
        g_Param.tolerance = 0.01f;    // 容差

        g_Param.cur_fiter_alpha = 0.5f; // 电流滤波系数
        g_Param.adc_U_zero_offset = 4095.0f / 2.0f;
        g_Param.adc_V_zero_offset = 4095.0f / 2.0f;
        g_Param.adc_W_zero_offset = 4095.0f / 2.0f;
    }
    return true;
}

void fParamErase()
{
    fEraseOneSector(PARAMETER_LOAD_ADDr);
}
