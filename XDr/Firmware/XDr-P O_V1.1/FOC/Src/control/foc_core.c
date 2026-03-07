#include "foc_core.h"
#include "adc_dr.h"
#include "svpwm.h"
#include "math_fast.h"
#include "encoder.h"
#include "string.h"
#include "smo.h"
#include "loop_control.h"
#include "drive_parameters.h"

tFOC_Mode foc_mode = {0};
tFOC_val foc_val = {0};
tMotor Motor = {0};

tFOC_Core foc_core = {.foc_mode = &foc_mode, .foc_val = &foc_val, .motor = &Motor};

// 启动器初始化
void _trajectory_init(tParameter param)
{
    tTraj_Config traj_cfg;
    traj_cfg.max_rate = param.traj_max_rate;
    traj_cfg.max_acc = param.traj_max_acc;
    traj_cfg.max_jerk = param.traj_max_jerk;
    traj_cfg.tolerance = param.tolerance;
    traj_cfg.type = param.traj_type;
    fTraj_Init(traj_cfg);
}

// 模式初始化
void _mode_init(tParameter param)
{
    fFOC_SetSensorMode(param.sensor_mode);
    foc_mode.runmode = param.run_mode;
    foc_mode.pvt_mode = param.sw_pvt;
    foc_mode.weak_mag = param.sw_weakmag;
}

// 电机参数初始化
void _motor_init(tParameter param)
{
    Motor.Wire_sequence = param.motor_wire_sequence == 0 ? 1 : -1;
    Motor.offset_angle = param.theta_offset;
    Motor.pole_pairs = param.motor_polepairs;
    Motor.Rs = param.motor_rs;
    Motor.Ls = param.motor_ls;
    Motor.Psi_f = param.motor_psif;
    Motor.Ke = param.motor_ke;
    Motor.J = param.motor_j;
    Motor.B = param.motor_b;
    fSMO_Init(Motor);
    fParamTuneReset();
}

// FOC核心初始化
void fFOC_CoreInit(void)
{
    fAdcGetVoltage(&Motor.Udc);
    _motor_init(g_Param);
    fLoopControlInit(g_Param, Motor.Udc / MATH_SQRT3);
    fSvpwmInit(Motor.Udc);
    _trajectory_init(g_Param);
    fFOC_CoreReset();
    _mode_init(g_Param);
}

// 重置FOC中间变量
void _FocValReset(void)
{
    memset(&foc_val, 0, sizeof(tFOC_val));
}

// FOC 复位
void fFOC_CoreReset(void)
{
    _FocValReset();
    fLoopReset();
    fSMO_Reset();
    if (foc_mode.runmode == POSITION_MODE)
        fTraj_Reset(foc_val.pos_fb);
    else if (foc_mode.runmode == SPEED_MODE)
        fTraj_Reset(foc_val.omega_fb);
}

// 电流重构：根据扇区将线电流转换为相电流
void Current_reconstruction(void)
{
    float ui, vi, wi;
    fAdcGetCurrent(&foc_val.Iu, &foc_val.Iv, &foc_val.Iw);
    switch (fSvpwmGetSector())
    {
    case 1:
    case 6:
        ui = foc_val.Iv + foc_val.Iw;
        vi = -foc_val.Iv;
        wi = -foc_val.Iw;
        break;
    case 2:
    case 3:
        ui = -foc_val.Iu;
        vi = foc_val.Iu + foc_val.Iw;
        wi = -foc_val.Iw;
        break;
    case 4:
    case 5:
        ui = -foc_val.Iu;
        vi = -foc_val.Iv;
        wi = foc_val.Iu + foc_val.Iv;
        break;
    default:
        ui = foc_val.Iu;
        vi = foc_val.Iv;
        wi = foc_val.Iw;
        break;
    }
    fClarkTransform(ui, vi, wi, &foc_val.Ialpha, &foc_val.Ibeta);
}

// todo:待完善 无感FOC（SMO）：获取电压/电流/角度/速度
void foc_senless_get_vito(void)
{
}

// 电压开环控制
void voltage_control(void)
{
    fInvParkTransform(foc_val.ud, foc_val.uq, foc_val.theta_elec, &foc_val.Ualpha, &foc_val.Ubeta);
}

// 电流环运行
void current_loop_run(void)
{
    if (!loop_con.fd.current_update)
        return;
    foc_val.uq = fCurrentLoopUpdate(foc_val.iq_ref, foc_val.iq_fb);
    foc_val.ud = fMagLoopUpdate(foc_val.id_ref, foc_val.id_fb);
}

// 速度环运行
void speed_loop_run(void)
{
    if (!loop_con.fd.speed_update)
        return;
    if (foc_mode.runmode == SPEED_MODE)
    {
        tTraj_Out traj_out = fTraj_Update(loop_con.fd.Tspd);
        foc_val.omega_ref = traj_out.value;
    }
    foc_val.iq_ref = fSpeedLoopUpdate(foc_val.omega_ref, foc_val.omega_fb);
}

// 弱磁控制
void weak_mag_loop_run(void)
{
    if (!loop_con.fd.speed_update)
        return;
    if (foc_val.omega_fb > 30.0f)
        foc_val.id_ref = fWeakMagLoopUpdate(foc_val.ud, foc_val.uq);
    else
        foc_val.id_ref = 0;
}

// 位置环
void position_loop_run(void)
{
    if (!loop_con.fd.position_update)
        return;
    if (foc_mode.runmode == POSITION_MODE)
    {
        tTraj_Out traj_out = fTraj_Update(loop_con.fd.Tpos);
        foc_val.pos_ref = traj_out.value;
    }
    foc_val.omega_ref = fPositionRelLoopUpdate(foc_val.pos_ref, foc_val.pos_fb);
}
// 功能使能矩阵

void fFOC_ValueUpdate(void)
{
    fFrequencyDivisionUpdate();
    fAdcGetVoltage(&Motor.Udc);
    Current_reconstruction();

    if (foc_mode.Encoder_enable)
    {
        fEncoderMainLoopTask(); // 编码器主循环
        foc_val.theta_mech = fGetEncoderAngle_ABS();
        foc_val.theta_elec = foc_val.theta_mech * Motor.pole_pairs - Motor.offset_angle;
        foc_val.theta_elec = Motor.Wire_sequence * fNormalizeAngle_0_2pi(foc_val.theta_elec);
        fParkTransform(foc_val.Ialpha, foc_val.Ibeta, foc_val.theta_elec, &foc_val.id_fb, &foc_val.iq_fb);
        foc_val.pos_fb = fGetEncoderAngle_INC();
        foc_val.omega_fb = fGetEncoderOmega();
    }
    if (foc_mode.SMO_enable)
    {
        // SMO 主循环
        fSMO_MainLoop(foc_val.Ualpha, foc_val.Ubeta, foc_val.Ialpha, foc_val.Ibeta);
        // if (startup_machine.change_flag)
        // {
        //     foc_val.theta_elec = smo_get_theta();
        // }
        // else
        // {
        //     if (foc_val.omega_ref > 0.1f)
        //         foc_val.iq_ref = startup_machine.openloop_iq;
        //     foc_val.theta_elec += startup_machine.openloop_omega * Tcon;
        //     foc_val.theta_elec = fNormalizeAngle02pi(foc_val.theta_elec);
        // }

        foc_val.omega_fb = fSMO_GetOmega();
    }
}
// 使能后执行：按模式运行对应控制环
void fFOC_MainLoopTask(void)
{
    if (foc_mode.OPEN_LOOP_enable)
    {
        foc_val.theta_openloop += foc_val.omega_openloop * Tcon;
        foc_val.theta_openloop = fNormalizeAngle_0_2pi(foc_val.theta_openloop);
        foc_val.theta_elec = foc_val.theta_openloop;
    }
    switch (foc_mode.sensor_mode)
    {
    case ENCODER_CONTROL: // 编码器闭环控制
        switch (foc_mode.runmode)
        {
        case SPEED_MODE:
            speed_loop_run();
            if (foc_mode.weak_mag)
                weak_mag_loop_run();
        case CURRENT_MODE:
            current_loop_run();
            voltage_control();
            break;
        case POSITION_MODE:
            position_loop_run();
            speed_loop_run();
            current_loop_run();
            voltage_control();
            break;
        default:
            break;
        }
        break;
    case SENSORLESS_CONTROL: // 无感控制
        switch (foc_mode.runmode)
        {
        case CURRENT_MODE:
            current_loop_run();
            voltage_control();
            break;
        case SPEED_MODE:
            speed_loop_run();
            current_loop_run();
            voltage_control();
            break;
        default:
            break;
        }
        break;
    case MERGE_CONTROL:
        switch (foc_mode.runmode)
        {
        case SPEED_MODE:
            speed_loop_run();
            if (foc_mode.weak_mag)
                weak_mag_loop_run();
        case CURRENT_MODE:
            current_loop_run();
            voltage_control();
            break;

        case POSITION_MODE:
            position_loop_run();
            speed_loop_run();
            current_loop_run();
            voltage_control();
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    fSvpwmRun(foc_val.Ualpha, foc_val.Ubeta);
    fSamplePointCalibration();
}

// 设置各环指令值
void fFOC_SetTargetValue(float *value)
{
    switch (foc_mode.runmode)
    {
    case CURRENT_MODE:
        foc_val.iq_ref = value[0];
        foc_val.id_ref = value[1];
        break;
    case SPEED_MODE:
        fTraj_SetTarget(value[0]);
        break;
    case POSITION_MODE:
        fTraj_SetTarget(value[0]);
        if (foc_mode.pvt_mode)
            fTraj_SetRate(value[1]);
        break;
    default: // IDLE 未使能 不能设置目标值
        break;
    }
}

// 参数自动校准
bool fAutoCalibrationUpdate(void)
{
    if (PARAM_TUNE_COMPLETE == fParamTuneUpdate(foc_val))
    {
        fFOC_CoreInit();
        return true;
    }
    return false;
}
// 设置 αβ 电压
void fFOC_SetUalphaBeta(float Ualpha, float Ubeta)
{
    foc_val.Ualpha = Ualpha;
    foc_val.Ubeta = Ubeta;
}

// 设置编码器零点偏移
void fSetThetaOffset(float thetaoffset)
{
    Motor.offset_angle = thetaoffset;
}

// 设置电机线序
void fSetWireSequence(int wire_sequence)
{
    Motor.Wire_sequence = wire_sequence;
}
// 电角度开环控制
void fOpenLoopEnable(bool enable)
{
    foc_mode.OPEN_LOOP_enable = enable;
}

//  设置开环初始电角度
void fSetOpendLoopTheta(float theta_elec)
{
    foc_val.omega_openloop = 0.0f;
    foc_val.theta_openloop = theta_elec;
}

// 设置开环旋转电角速度
void fSetOpendLoopOmega(float omega_elec)
{
    foc_val.omega_openloop = omega_elec;
}
// 切换传感模式
void fFOC_SetSensorMode(eSensorMode mode)
{
    fFOC_CoreReset();
    foc_mode.sensor_mode = mode;
    foc_mode.OPEN_LOOP_enable = false;
    switch (foc_mode.sensor_mode)
    {
    case ENCODER_CONTROL:
        foc_mode.Encoder_enable = true;
        foc_mode.SMO_enable = false;
        break;
    case SENSORLESS_CONTROL:
        foc_mode.SMO_enable = true;
        foc_mode.Encoder_enable = false;
        break;
    default: // 融合模式
        foc_mode.Encoder_enable = true;
        foc_mode.SMO_enable = true;
        break;
    }
}
// 切换控制模式
void fFOC_SetRunMode(eRunMode mode)
{
    _FocValReset();
    foc_mode.runmode = mode;
}

// 强制刹车
bool fFOC_Shutdown(void)
{
    if (fabsf(foc_val.omega_fb) < 0.1f)
        return true;
    // todo:完善不同模式下的停机方式
    if (foc_mode.runmode != SPEED_MODE)
        fFOC_SetRunMode(SPEED_MODE);
    float omega_shutdown = -foc_val.omega_fb * 0.5f;
    fFOC_SetTargetValue(&omega_shutdown);
    return false;
}