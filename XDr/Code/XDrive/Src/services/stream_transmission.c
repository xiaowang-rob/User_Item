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
/*
在数据写入读出的时候进行单位转换，以便于在PC端进行数据可视化
*/
#define rad_to_rpm(rad) rad * 9.549296748f;
#define rpm_to_rad(rpm) rpm / 9.549296748f;
#define deg_to_rad(deg) deg * 0.017453293f;
#define rad_to_deg(rad) rad * 57.29577951f;

void stream_data_get(Data_stream_e stream, float *data)
{
    switch (stream)
    {
    case VBUS:
        *data = g_adaptive_con.Udc;
        break;
    case TEMP:
        *data = g_adaptive_con.tempareture;
        break;
    case THETA_elec:
        *data = rad_to_deg(g_monitor.theta_elec);
        break;
    case THETA_mech:
        *data = rad_to_deg(g_monitor.theta_mech);
        break;
    case CURRENT_q:
        *data = g_monitor.iq_fb;
        break;
    case CURRENT_d:
        *data = g_monitor.id_fb;
        break;
    case CURRENT_alpha:
        *data = g_monitor.Ialpha;
        break;
    case CURRENT_beta:
        *data = g_monitor.Ibeta;
        break;
    case VOLTAGE_alpha:
        *data = g_monitor.Ualpha;
        break;
    case VOLTAGE_beta:
        *data = g_monitor.Ibeta;
        break;
    case SPEED_rpm:
        *data = rad_to_rpm(g_monitor.omega_fb);
        break;
    case POSITION_motor:
        *data = rad_to_deg(g_monitor.pos_fb);
        break;
    case POSITION_target:
        *data = rad_to_deg(g_monitor.pos_fb / g_foccore.Reduction_ratio);
        break;
    case REF_iq:
        *data = g_foccore.iq_ref;
        break;
    case REF_id:
        *data = g_foccore.id_ref;
        break;
    case REF_speed:
        *data = rad_to_rpm(g_foccore.omega_ref);
        break;
    case REF_position_motor:
        *data = rad_to_deg(g_foccore.pos_ref * g_foccore.Reduction_ratio);
        break;
    case REF_position_target:
        *data = rad_to_deg(g_foccore.pos_ref);
        break;
    default:
        break;
    }
}

Parameter_t g_foc_parameters;

void parameter_set(Parameter_e parameter, u32 *data)
{
    switch (parameter)
    {
    case CAN_ID:
        g_foc_parameters.can_id = *data;
        break;
    case CAN_QUEUE:
        g_foc_parameters.can_queue = *data;
        break;
    case WEAK_MAG:
        g_foc_parameters.weak_mag = *data;
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
    case REDUCTION_RATIO:
        g_foc_parameters.reduction_ratio = *data;
        break;
    case FOC_RUN_MODE:
        g_foc_parameters.foc_run_mode = *data;
        break;
    case FOC_LOOP_MODE:
        g_foc_parameters.foc_loop_mode = *data;
        break;
    case FOC_AUTOTUNE_MODE:
        g_foc_parameters.foc_autotune_mode = *data;
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
        // 可以添加错误处理，如打印错误信息或断言
        break;
    }
}
void all_parameters_set(u32 *data)
{
    for (int i = 0; i < CHANGE_LOOP_SPEED + 1; i++)
    {
        parameter_set(i, data + i);
    }
}
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
    case REDUCTION_RATIO:
        *data = g_foc_parameters.reduction_ratio;
        break;
    case FOC_RUN_MODE:
        *data = g_foc_parameters.foc_run_mode;
        break;
    case FOC_LOOP_MODE:
        *data = g_foc_parameters.foc_loop_mode;
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
        // 可以设置默认值或记录错误
        *data = 0.0f; // 默认返回0
        break;
    }
}
void all_parameters_ask(u32 *data, u8 *len)
{
    for (int i = 0; i < CHANGE_LOOP_SPEED + 1; i++)
    {
        parameter_ask(i, data + i);
    }
    *len = CHANGE_LOOP_SPEED + 1;
}
void parameter_apply()
{
    // todo:can队列
    // todo：弱磁控制
    CANDr_Init(g_foc_parameters.can_id);
    protection_manager_init(g_foc_parameters.limit_current, g_foc_parameters.limit_speed, g_foc_parameters.limit_position_min, g_foc_parameters.limit_position_max,
                            g_foc_parameters.tolerance_time, g_foc_parameters.tolerance_voltage, g_foc_parameters.tolerance_current, g_foc_parameters.tolerance_speed,
                            g_foc_parameters.tolerance_position);
    FOC_CHANGE_STATE(FOC_INIT);
}
bool parameter_save()
{
    FLASH_erase_sector(PARAMETER_LOAD_ADDr, sizeof(Parameter_t));
    return FLASH_Write_Word((u8 *)&g_foc_parameters, PARAMETER_LOAD_ADDr, sizeof(Parameter_t));
}
bool parameter_load()
{
    return FLASH_Read_data((u8 *)&g_foc_parameters, PARAMETER_LOAD_ADDr, sizeof(Parameter_t));
}
void parameter_erase()
{
    FLASH_erase_sector(PARAMETER_LOAD_ADDr, sizeof(Parameter_t));
}

bool parameter_init()
{
    if (!parameter_load())
        return false;
    if (g_foc_parameters.None_flag == 0)
        return true;
    else
    {
        g_foc_parameters.None_flag = 0; // 参数存储是否为空
        g_foc_parameters.theta_offset = 0.0f;
        g_foc_parameters.motor_polepairs = 7;
        g_foc_parameters.motor_rs = 0.05f;                           // 相电阻 50mΩ
        g_foc_parameters.motor_ls = 0.0002f;                         // 相电感 200μH
        g_foc_parameters.motor_psif = 0.01f;                         // 永磁体磁链 0.01Wb
        g_foc_parameters.motor_j = 0.0001f;                          // 转动惯量 0.0001 kg·m²
        g_foc_parameters.motor_b = 0.0005f;                          // 阻尼系数 0.0005 N·m·s/rad
        g_foc_parameters.motor_tc = 0.1f;                            // 转矩常数 0.1 N·m/A
        g_foc_parameters.reduction_ratio = 1.0f;                     // 减速比 1:1（直驱）
        g_foc_parameters.foc_run_mode = 2;                           // 运行模式：0-整定 1-有感 2-无感
        g_foc_parameters.foc_loop_mode = 1;                          // 环路模式：0-电流环 1-速度环 2-绝对位置环 3-相对位置环
        g_foc_parameters.foc_autotune_mode = 1;                      // 自整定模式：0-有感整定 1-无感整定 2-手动整定
        g_foc_parameters.f_current_loop = fpwm;                      // 电流环频率 20kHz
        g_foc_parameters.f_speed_loop = 2000;                        // 速度环频率 2kHz
        g_foc_parameters.f_position_loop = 1000;                     // 位置环频率 500Hz
        g_foc_parameters.kp_current = 0.5f;                          // 电流环比例系数
        g_foc_parameters.ki_current = 100.0f;                        // 电流环积分系数
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
    parameter_apply();
    return true;
}
void CONTROL_value_update(float *data)
{
    switch (g_foccore.loop_mode)
    {
    case CURRENT_LOOP_CONTROL:
        g_foccore.iq_ref = data[0];
        break;
    case SPEED_LOOP_CONTROL:
        g_foccore.omega_ref = rpm_to_rad(data[0]);
        break;
    case POSITION_ABS_CONTROL:
    case POSITION_REL_CONTROL:
        g_foccore.pos_ref = deg_to_rad(data[0]);
        break;
    default:
        break;
    }
}
void CONTROL_mode_updata(u8 mode)
{
    g_foccore.loop_mode = mode;
}
void STATUS_get(u8 *foc_status, u8 *fault)
{
    *foc_status = g_foccore.state;
    *fault = GET_Protect_fault();
}