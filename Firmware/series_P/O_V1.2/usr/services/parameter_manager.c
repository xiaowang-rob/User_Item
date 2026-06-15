#include "parameter_manager.h"
#include <stddef.h>
#include "string.h"
#include "foc_main.h"
#include "math_fast.h"
#include "protection_manager.h"
#include "can_port.h"
#include "usr_config.h"

tParameter g_Param;

// param 描述符表
#define PARAM_ENTRY(id, field) [id] = { .offset = offsetof(tParameter, field), .size = sizeof(((tParameter*)0)->field) }

const tParamEntry g_param_table[] = {
    PARAM_ENTRY(ENCODER_CHIP,       encoder_chip),
    PARAM_ENTRY(SENSOR_MODE,        sensor_mode),
    PARAM_ENTRY(RUN_MODE,           run_mode),
    PARAM_ENTRY(CAN_MODE,           sw_canqueue),
    PARAM_ENTRY(VAGUE_PID_MODE,     sw_vague_pid),
    PARAM_ENTRY(PVT_MODE,           sw_pvt),
    PARAM_ENTRY(TRAJ_TYPE,          traj_type),
    PARAM_ENTRY(MOTOR_POLEPAIRS,    motor_polepairs),
    PARAM_ENTRY(CAN_ID,             can_id),
    PARAM_ENTRY(THETA_OFFSET,       theta_offset),
    PARAM_ENTRY(MOTOR_KV,           motor_kv),
    PARAM_ENTRY(MOTOR_RS,           motor_rs),
    PARAM_ENTRY(MOTOR_Ld,           motor_ld),
    PARAM_ENTRY(MOTOR_Lq,           motor_lq),
    PARAM_ENTRY(MOTOR_PSIF,         motor_psif),
    PARAM_ENTRY(MOTOR_KE,           motor_ke),
    PARAM_ENTRY(MOTOR_J,            motor_j),
    PARAM_ENTRY(MOTOR_B,            motor_b),
    PARAM_ENTRY(KP_SPEED,           kp_speed),
    PARAM_ENTRY(KI_SPEED,           ki_speed),
    PARAM_ENTRY(KP_POSITION,        kp_position),
    PARAM_ENTRY(KI_POSITION,        ki_position),
    PARAM_ENTRY(KD_POSITION,        kd_position),
    PARAM_ENTRY(MIT_KP,             kp_MIT),
    PARAM_ENTRY(MIT_KD,             kd_MIT),
    PARAM_ENTRY(MIT_TFF,            tff_MIT),
    PARAM_ENTRY(MIT_TMAX,           tmax_MIT),
    PARAM_ENTRY(TUNE_CURRENT,       tune_current),
    PARAM_ENTRY(LIMIT_CURRENT,      limit_current),
    PARAM_ENTRY(LIMIT_SPEED,        limit_omega),
    PARAM_ENTRY(LIMIT_POSITION_MIN, limit_position_min),
    PARAM_ENTRY(LIMIT_POSITION_MAX, limit_position_max),
    PARAM_ENTRY(TOLERANCE_TIME,     tolerance_time),
    PARAM_ENTRY(TOLERANCE_LIMIT,    tolerance_limit),
    PARAM_ENTRY(TRAJ_MAX_RATE,      traj_max_rate),
    PARAM_ENTRY(TRAJ_MAX_ACC,       traj_max_acc),
    PARAM_ENTRY(TRAJ_MAX_JERK,      traj_max_jerk),
    PARAM_ENTRY(TRAJ_TOLERANCE,     tolerance),
};

void fParamSet(eParameter para, u8 *value)
{
    if (para >= PARAM_NUM) {
        return;
    }
    u8 *dst = (u8 *)&g_Param + g_param_table[para].offset;
    memcpy(dst, value, g_param_table[para].size);
}

void fParamGet(eParameter para, u8 *value, u8 *len)
{
    if (para >= PARAM_NUM) {
        *len = 0;
        return;
    }
    u8 *src = (u8 *)&g_Param + g_param_table[para].offset;
    memcpy(value, src, g_param_table[para].size);
    *len = g_param_table[para].size;
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
    }
    return true;
}

void fParamErase()
{
    fEraseOneSector(PARAMETER_LOAD_ADDr);
}
