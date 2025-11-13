#include "stream_transmission.h"
#include "adaptive_control.h"
#include "foc_core.h"
#include "math_fast.h"
#include "encoder.h"
#include "auto_calibration.h"
#include "loop_control.h"
#include "system_parameters.h"
#include "protection_manager.h"
#include "flashDr.h"
#include "canDr.h"
#include "svpwm.h"
#include "system_statemachine.h"
#include "string.h"
u8 _sta[4];
void stream_data_get(Data_stream_e stream, float *data)
{
    switch (stream)
    {
    case STATUS:
        _sta[0] = SystemState_get();
        _sta[1] = g_foccore.state;
        _sta[2] = g_protection_manager.fault;
        _sta[3] = g_protection_manager.warning;
        memcpy(data, _sta, 4);
        break;
    case TEMPERATURE:
        *data = g_adaptive_con.tempareture;
        break;
    case VBUS:
        *data = g_adaptive_con.Udc;
        break;
    case VOLTAGE_U:
        *data = g_svpwm.ticu * g_adaptive_con.Udc / ticpwm;
        break;
    case VOLTAGE_V:
        *data = g_monitor.Ibeta * g_adaptive_con.Udc / ticpwm;
        break;
    case VOLTAGE_W:
        *data = g_monitor.Ualpha * g_adaptive_con.Udc / ticpwm;
        break;
    case VOLTAGE_q:
        *data = g_monitor.uq;
        break;
    case VOLTAGE_d:
        *data = g_monitor.ud;
        break;
    case CURRENT_U:
        *data = g_monitor.Iu;
        break;
    case CURRENT_V:
        *data = g_monitor.Iv;
        break;
    case CURRENT_W:
        *data = g_monitor.Iw;
        break;
    case CURRENT_q:
        *data = g_monitor.iq_fb;
        break;
    case CURRENT_d:
        *data = g_monitor.id_fb;
        break;
    case CURRENT_q_ref:
        *data = g_foccore.iq_ref;
        break;
    case CURRENT_d_ref:
        *data = g_foccore.id_ref;
        break;
    case SPEED:
        *data = rad_to_rpm(g_monitor.omega_fb);
        break;
    case SPEED_con:
        *data = rad_to_rpm(g_foccore.omega_con);
        break;
    case SPEED_ref:
        *data = rad_to_rpm(g_foccore.omega_ref);
        break;
    case THETA_elec:
        *data = rad_to_deg(g_monitor.theta_elec);
        break;
    case THETA_mech:
        *data = rad_to_deg(g_monitor.theta_mech);
        break;
    case THETA_mech_con:
        *data = rad_to_deg(g_foccore.pos_con);
        break;
    case THETA_mech_ref:
        *data = rad_to_deg(g_foccore.pos_ref);
        break;
    default:
        break;
    }
}
Mode_t g_foc_mode;
Parameter_t g_foc_parameters;

void parameter_apply()
{
    // todo：弱磁控制
    CANDr_Init(g_foc_parameters.can_id, g_foc_mode.canqueue);
    protection_manager_init(g_foc_parameters.limit_current, g_foc_parameters.limit_speed, g_foc_parameters.limit_position_min, g_foc_parameters.limit_position_max,
                            g_foc_parameters.tolerance_time, g_foc_parameters.tolerance_voltage, g_foc_parameters.tolerance_current, g_foc_parameters.tolerance_speed,
                            g_foc_parameters.tolerance_position);
    FOC_CHANGE_STATE(FOC_INIT);
}

void mode_apply()
{
    FOC_CHANGE_STATE(FOC_INIT);
}

void mode_set(Mode_e mode, u8 *data)
{
    switch (mode)
    {
    case SW_CANQUEUE:
        g_foc_mode.canqueue = *data;
        break;
    case SW_WEAKMAG:
        g_foc_mode.weakmag = *data;
        break;
    case SW_FAN:
        g_foc_mode.fan = *data;
        break;
    case SW_TLC:
        g_foc_mode.tls = *data;
        break;
    case SW_CLS:
        g_foc_mode.cls = *data;
        break;
    case SW_VAGUE_PID:
        g_foc_mode.vague_pid = *data;
        break;
    case SW_PVT:
        g_foc_mode.pvt = *data;
        break;
    case FOC_RUN_MODE:
        g_foc_mode.foc_run_mode = *data;
        break;
    case FOC_LOOP_MODE:
        g_foc_mode.foc_loop_mode = *data;
        break;
    case FOC_AUTOTUNE_MODE:
        g_foc_mode.foc_autotune_mode = *data;
        break;
    default:
        break;
    }
}
void mode_ask(Mode_e mode, u8 *data)
{
    if (data == NULL)
    {
        // 空指针检查，防止程序崩溃
        return;
    }
    switch (mode)
    {
    case SW_CANQUEUE:
        *data = g_foc_mode.canqueue;
        break;
    case SW_WEAKMAG:
        *data = g_foc_mode.weakmag;
        break;
    case SW_FAN:
        *data = g_foc_mode.fan;
        break;
    case SW_TLC:
        *data = g_foc_mode.tls;
        break;
    case SW_CLS:
        *data = g_foc_mode.cls;
        break;
    case SW_VAGUE_PID:
        *data = g_foc_mode.vague_pid;
        break;
    case SW_PVT:
        *data = g_foc_mode.pvt;
        break;
    case FOC_RUN_MODE:
        *data = g_foc_mode.foc_run_mode;
        break;
    case FOC_LOOP_MODE:
        *data = g_foc_mode.foc_loop_mode;
        break;
    case FOC_AUTOTUNE_MODE:
        *data = g_foc_mode.foc_autotune_mode;
        break;
    default:
        break;
    }
}

// 写入个别参数
void parameter_set(Parameter_e parameter, u32 *data)
{
    switch (parameter)
    {
    case CAN_ID:
        g_foc_parameters.can_id = *data;
        break;
    case THETA_OFFSET:
        g_foc_parameters.theta_offset = *data;
        break;
    case MOTOR_POLEPAIRS:
        g_foc_parameters.motor_polepairs = *data;
        break;
    case MOTOR_RS:
        g_foc_parameters.motor_rs = *data;
        break;
    case MOTOR_LS:
        g_foc_parameters.motor_ls = *data;
        break;
    case MOTOR_Psif:
        g_foc_parameters.motor_psif = *data;
        break;
    case MOTOR_J:
        g_foc_parameters.motor_j = *data;
        break;
    case MOTOR_B:
        g_foc_parameters.motor_b = *data;
        break;
    case MOTOR_TC:
        g_foc_parameters.motor_tc = *data;
        break;
    case f_CURRENT_LOOP:
        g_foc_parameters.f_current_loop = *data;
        break;
    case f_SPEED_LOOP:
        g_foc_parameters.f_speed_loop = *data;
        break;
    case f_POSITION_LOOP:
        g_foc_parameters.f_position_loop = *data;
        break;
    case Kp_CURRENT:
        g_foc_parameters.kp_current = *data;
        break;
    case Ki_CURRENT:
        g_foc_parameters.ki_current = *data;
        break;
    case Kp_WEAKMAG:
        g_foc_parameters.kp_weakmag = *data;
        break;
    case Ki_WEAKMAG:
        g_foc_parameters.ki_weakmag = *data;
        break;
    case Kp_SPEED:
        g_foc_parameters.kp_speed = *data;
        break;
    case Ki_SPEED:
        g_foc_parameters.ki_speed = *data;
        break;
    case Kp_POSITION:
        g_foc_parameters.kp_position = *data;
        break;
    case Ki_POSITION:
        g_foc_parameters.ki_position = *data;
        break;
    case Kd_POSITION:
        g_foc_parameters.kd_position = *data;
        break;
    case LIMIT_CURRENT:
        g_foc_parameters.limit_current = *data;
        break;
    case LIMIT_SPEED:
        g_foc_parameters.limit_speed = rpm_to_rad(*data);
        break;
    case LIMIT_POSITION_min:
        g_foc_parameters.limit_position_min = deg_to_rad(*data);
        break;
    case LIMIT_POSITION_max:
        g_foc_parameters.limit_position_max = deg_to_rad(*data);
        break;
    case TOLERANCE_TIME:
        g_foc_parameters.tolerance_time = *data;
        break;
    case TOLERANCE_VOLTAGE:
        g_foc_parameters.tolerance_voltage = *data;
        break;
    case TOLERANCE_CURRENT:
        g_foc_parameters.tolerance_current = *data;
        break;
    case TOLERANCE_SPEED:
        g_foc_parameters.tolerance_speed = *data;
        break;
    case TOLERANCE_POSITION:
        g_foc_parameters.tolerance_position = *data;
        break;
    case STARTUP_POS_GRAD:
        g_foc_parameters.startup_pos_grad = deg_to_rad(*data);
        break;
    case STARTUP_SPE_GRAD:
        g_foc_parameters.startup_spe_grad = rpm_to_rad(*data);
        break;
    case ALIGN_CURRENT:
        g_foc_parameters.align_current = *data;
        break;
    case ALIGN_TIME:
        g_foc_parameters.align_time = *data;
        break;
    case OPEN_LOOP_CURRENT:
        g_foc_parameters.open_loop_current = *data;
        break;
    case OPEN_LOOP_SPEED:
        g_foc_parameters.open_loop_speed = rpm_to_rad(*data);
        break;
    case CHANGE_LOOP_SPEED:
        g_foc_parameters.change_loop_speed = rpm_to_rad(*data);
        break;
    default:
        break;
    }
    parameter_apply();
}
// 一键写入所有参数

void parameter_ask(Parameter_e parameter, u32 *data)
{
    if (data == NULL)
    {
        // 空指针检查，防止程序崩溃
        return;
    }

    switch (parameter)
    {
    case CAN_ID:
        *data = g_foc_parameters.can_id;
        break;
    case THETA_OFFSET:
        *data = g_foc_parameters.theta_offset;
        break;
    case MOTOR_POLEPAIRS:
        *data = g_foc_parameters.motor_polepairs;
        break;
    case MOTOR_RS:
        *data = g_foc_parameters.motor_rs;
        break;
    case MOTOR_LS:
        *data = g_foc_parameters.motor_ls;
        break;
    case MOTOR_Psif:
        *data = g_foc_parameters.motor_psif;
        break;
    case MOTOR_J:
        *data = g_foc_parameters.motor_j;
        break;
    case MOTOR_B:
        *data = g_foc_parameters.motor_b;
        break;
    case MOTOR_TC:
        *data = g_foc_parameters.motor_tc;
        break;
    case f_CURRENT_LOOP:
        *data = g_foc_parameters.f_current_loop;
        break;
    case f_SPEED_LOOP:
        *data = g_foc_parameters.f_speed_loop;
        break;
    case f_POSITION_LOOP:
        *data = g_foc_parameters.f_position_loop;
        break;
    case Kp_CURRENT:
        *data = g_foc_parameters.kp_current;
        break;
    case Ki_CURRENT:
        *data = g_foc_parameters.ki_current;
        break;
    case Kp_WEAKMAG:
        *data = g_foc_parameters.kp_weakmag;
        break;
    case Ki_WEAKMAG:
        *data = g_foc_parameters.ki_weakmag;
        break;
    case Kp_SPEED:
        *data = g_foc_parameters.kp_speed;
        break;
    case Ki_SPEED:
        *data = g_foc_parameters.ki_speed;
        break;
    case Kp_POSITION:
        *data = g_foc_parameters.kp_position;
        break;
    case Ki_POSITION:
        *data = g_foc_parameters.ki_position;
        break;
    case Kd_POSITION:
        *data = g_foc_parameters.kd_position;
        break;
    case LIMIT_CURRENT:
        *data = g_foc_parameters.limit_current;
        break;
    case LIMIT_SPEED:
        *data = rad_to_rpm(g_foc_parameters.limit_speed);
        break;
    case LIMIT_POSITION_min:
        *data = rad_to_deg(g_foc_parameters.limit_position_min);
        break;
    case LIMIT_POSITION_max:
        *data = rad_to_deg(g_foc_parameters.limit_position_max);
        break;
    case TOLERANCE_TIME:
        *data = g_foc_parameters.tolerance_time;
        break;
    case TOLERANCE_VOLTAGE:
        *data = g_foc_parameters.tolerance_voltage;
        break;
    case TOLERANCE_CURRENT:
        *data = g_foc_parameters.tolerance_current;
        break;
    case TOLERANCE_SPEED:
        *data = g_foc_parameters.tolerance_speed;
        break;
    case TOLERANCE_POSITION:
        *data = g_foc_parameters.tolerance_position;
        break;
    case STARTUP_POS_GRAD:
        *data = rad_to_deg(g_foc_parameters.startup_pos_grad);
        break;
    case STARTUP_SPE_GRAD:
        *data = rad_to_rpm(g_foc_parameters.startup_spe_grad);
        break;
    case ALIGN_CURRENT:
        *data = g_foc_parameters.align_current;
        break;
    case ALIGN_TIME:
        *data = g_foc_parameters.align_time;
        break;
    case OPEN_LOOP_CURRENT:
        *data = g_foc_parameters.open_loop_current;
        break;
    case OPEN_LOOP_SPEED:
        *data = rad_to_rpm(g_foc_parameters.open_loop_speed);
        break;
    case CHANGE_LOOP_SPEED:
        *data = rad_to_rpm(g_foc_parameters.change_loop_speed);
        break;
    default:
        break;
    }
}

// 保存参数
bool parameter_save()
{
    FLASH_erase_sector(PARAMETER_LOAD_ADDr, sizeof(Parameter_t));
    return FLASH_Write_Word((u8 *)&g_foc_parameters, PARAMETER_LOAD_ADDr, sizeof(Parameter_t));
}
// 保存模式
bool mode_save()
{
    FLASH_erase_sector(MODE_LOAD_ADDr, sizeof(Mode_t));
    return FLASH_Write_Word((u8 *)&g_foc_mode, MODE_LOAD_ADDr, sizeof(Mode_t));
}
bool parameter_load()
{
    return FLASH_Read_data((u8 *)&g_foc_parameters, PARAMETER_LOAD_ADDr, sizeof(Parameter_t));
}
bool mode_load()
{
    return FLASH_Read_data((u8 *)&g_foc_mode, MODE_LOAD_ADDr, sizeof(Mode_t));
}
// 擦除参数
void parameter_erase()
{
    FLASH_erase_sector(PARAMETER_LOAD_ADDr, sizeof(Parameter_t));
}
// 擦除模式
void mode_erase()
{
    FLASH_erase_sector(MODE_LOAD_ADDr, sizeof(Mode_t));
}
bool parameter_mode_init()
{
    if (!parameter_load())
        return false;
    if (!mode_load())
        return false;
    if (g_foc_parameters.None_flag == 0) // 模式存储是否为空
        return true;
    else
    {
        g_foc_mode.None_flag = 0;
        g_foc_mode.canqueue = 0;
        g_foc_mode.weakmag = 0;
        g_foc_mode.fan = 0;
        g_foc_mode.tls = 0;
        g_foc_mode.cls = 0;
        g_foc_mode.vague_pid = 0;
        g_foc_mode.pvt = 0;
        g_foc_mode.foc_run_mode = 2;
        g_foc_mode.foc_loop_mode = 1;
        g_foc_mode.foc_autotune_mode = 1;
    }

    if (g_foc_parameters.None_flag == 0) // 模式存储是否为空
        return true;
    else
    {
        g_foc_parameters.None_flag = 0;
        g_foc_parameters.theta_offset = 0.0f;
        g_foc_parameters.motor_polepairs = 7;
        g_foc_parameters.motor_rs = 0.05f;                           // 相电阻 50mΩ
        g_foc_parameters.motor_ls = 0.0002f;                         // 相电感 200μH
        g_foc_parameters.motor_psif = 0.01f;                         // 永磁体磁链 0.01Wb
        g_foc_parameters.motor_j = 0.0001f;                          // 转动惯量 0.0001 kg·m²
        g_foc_parameters.motor_b = 0.0005f;                          // 阻尼系数 0.0005 N·m·s/rad
        g_foc_parameters.motor_tc = 0.1f;                            // 转矩常数 0.1 N·m/A
        g_foc_parameters.f_current_loop = fpwm;                      // 电流环频率 20kHz
        g_foc_parameters.f_speed_loop = 2000;                        // 速度环频率 2kHz
        g_foc_parameters.f_position_loop = 1000;                     // 位置环频率 500Hz
        g_foc_parameters.kp_current = 0.5f;                          // 电流环比例系数
        g_foc_parameters.ki_current = 100.0f;                        // 电流环积分系数
        g_foc_parameters.kp_weakmag = 0.5f;                          // 弱磁环比例系数
        g_foc_parameters.ki_weakmag = 10.0f;                         // 弱磁环积分系数
        g_foc_parameters.kp_speed = 0.1f;                            // 速度环比例系数
        g_foc_parameters.ki_speed = 5.0f;                            // 速度环积分系数
        g_foc_parameters.kp_position = 50.0f;                        // 位置环比例系数
        g_foc_parameters.ki_position = 1.0f;                         // 位置环积分系数
        g_foc_parameters.kd_position = 0.1f;                         // 位置环微分系数
        g_foc_parameters.limit_current = 50.0f;                      // 电流限幅 50A
        g_foc_parameters.limit_speed = rpm_to_rad(3000.0f);          // 速度限幅 3000 RPM
        g_foc_parameters.limit_position_min = deg_to_rad(-10000.0f); // 最小位置限制
        g_foc_parameters.limit_position_max = deg_to_rad(10000.0f);  // 最大位置限制
        g_foc_parameters.tolerance_time = 0.1f;                      // 容忍时间 0.1秒
        g_foc_parameters.tolerance_voltage = 1.2f;                   // 电压容忍度 1.2 max_voltage*1.2
        g_foc_parameters.tolerance_current = 1.1f;                   // 电流容忍度 50*1.1=55A
        g_foc_parameters.tolerance_speed = 1.1f;                     // 速度容忍度
        g_foc_parameters.tolerance_position = 1.1f;                  // 位置容忍度
        g_foc_parameters.startup_pos_grad = deg_to_rad(10.0f);       // 启动位置梯度 10度/秒
        g_foc_parameters.startup_spe_grad = rpm_to_rad(100.0f);      // 启动速度梯度 100 RPM/秒
        g_foc_parameters.align_current = 5.0f;                       // 对齐电流 5A
        g_foc_parameters.align_time = 0.5f;                          // 对齐时间
        g_foc_parameters.open_loop_current = 5.0f;                   // 开环电流
        g_foc_parameters.open_loop_speed = rpm_to_rad(150.0f);       // 开环速度 100 RPM
        g_foc_parameters.change_loop_speed = rpm_to_rad(100.0f);     // 切环速度 50 RPM}
    }
    mode_apply();
    parameter_apply();
    return true;
}

void STATUS_get(u8 *foc_status, u8 *fault)
{
    *foc_status = g_foccore.state;
    *fault = GET_Protect_fault();
}