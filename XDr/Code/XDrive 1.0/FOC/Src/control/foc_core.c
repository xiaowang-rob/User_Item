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
tStartupMechine startup_machine = {0};
tMotor Motor = {0};

tFOC_Core foc_core = {.foc_mode = &foc_mode, .foc_val = &foc_val, .startup_machine = &startup_machine, .motor = &Motor};

// 启动器初始化
void startup_machine_init(tParameter param)
{
    memset(&startup_machine, 0, sizeof(tStartupMechine));
    startup_machine.omega_acc = param.startup_acc * loop_con.fd.Tspd;
    startup_machine.align_id = param.align_current;
    startup_machine.align_steps = (u32)(param.align_time / Tcon);
    startup_machine.openloop_iq = param.open_loop_current;
    startup_machine.openloop_omega = param.open_loop_omega;
}

// 模式初始化
void mode_init(tParameter param)
{
    fFOC_SetSensorMode(param.sensor_mode);
    foc_mode.loop_mode = param.loop_mode;
    foc_mode.pvt_mode = param.sw_pvt;
    foc_mode.weak_mag = param.sw_weakmag;
}

// 电机参数初始化
void motor_init(tParameter param)
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
    motor_init(g_Param);
    fLoopControlInit(g_Param, Motor.Udc / MATH_SQRT3);
    fSvpwmInit(Motor.Udc);
    startup_machine_init(g_Param);
    fFOC_CoreReset();
    mode_init(g_Param);
}

// todo:待完善 重置启动状态机
void startup_machine_reset(void)
{
    startup_machine.current_steps = 0;
}

// 重置FOC中间变量
void foc_val_reset(void)
{
    memset(&foc_val, 0, sizeof(tFOC_val));
}

// FOC 复位
void fFOC_CoreReset(void)
{
    foc_val_reset();
    fLoopReset();
    fSMO_Reset();
    startup_machine_reset();
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

// todo:待完善 SMO模式下：初始角度对齐
bool theta_align_curloop(void)
{
    if (startup_machine.current_steps < startup_machine.align_steps)
    {
        startup_machine.current_steps++;
        foc_val.id_ref = startup_machine.align_id;
        return false;
    }
    startup_machine.current_steps = 0;
    foc_val.ud = 0;
    foc_val.theta_elec = 0;
    startup_machine.align_flag = true;
    return true;
}

// todo:待完善 无感FOC：开环转闭环判断
static float omega_pre = 0.0f;
void oloop_to_cloop(void)
{
    foc_val.iq_ref = startup_machine.openloop_iq;
    foc_val.theta_elec += startup_machine.openloop_omega * loop_con.fd.Tcur;
    foc_val.theta_elec = fNormalizeAngle02pi(foc_val.theta_elec);
    foc_val.omega_fb = fSMO_GetOmega();
    if (foc_val.omega_fb > startup_machine.openloop_omega * 0.7f && fabsf(foc_val.omega_fb - omega_pre) < 1.0f)
    {
        startup_machine.change_flag = true;
    }
    omega_pre = foc_val.omega_fb;
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
    float delta = foc_val.omega_ref - foc_val.omega_con;
    float omega_step = loop_con.fd.Tspd * startup_machine.omega_acc;
    if (delta > omega_step)
        foc_val.omega_con += omega_step;
    else if (delta < -omega_step)
        foc_val.omega_con -= omega_step;
    else
        foc_val.omega_con = foc_val.omega_ref;
    foc_val.iq_ref = fSpeedLoopUpdate(foc_val.omega_con, foc_val.omega_fb);
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

// 绝对位置环
void position_abs_loop_run(void)
{
    if (!loop_con.fd.position_update)
        return;
    foc_val.pos_ref = fmodf(foc_val.pos_ref, MATH_2PI);
    // todo:最小角度转动
    foc_val.omega_ref = fPositionAbsLoopUpdate(foc_val.pos_ref, foc_val.pos_fb);
}

// 相对位置环
void position_rel_loop_run(void)
{
    if (!loop_con.fd.position_update)
        return;
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
        foc_val.theta_elec = Motor.Wire_sequence * fNormalizeAngle02pi(foc_val.theta_elec);
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
        omega_pre = foc_val.omega_fb;
    }
}
// 使能后执行：按模式运行对应控制环
void fFOC_MainLoopTask(void)
{
    if (foc_mode.OPEN_LOOP_enable)
    {
        foc_val.theta_openloop += foc_val.omega_openloop * Tcon;
        foc_val.theta_openloop = fNormalizeAngle02pi(foc_val.theta_openloop);
        foc_val.theta_elec = foc_val.theta_openloop;
    }
    switch (foc_mode.sensor_mode)
    {
    case ENCODER_CONTROL: // 编码器闭环控制
        switch (foc_mode.loop_mode)
        {
        case SPEED_LOOP:
            speed_loop_run();
            if (foc_mode.weak_mag)
                weak_mag_loop_run();
        case CURRENT_LOOP:
            current_loop_run();
        case VOLTAGE_LOOP:
            voltage_control();
            break;
        case POSITION_ABS_LOOP:
            position_abs_loop_run();
            speed_loop_run();
            current_loop_run();
            voltage_control();
            break;
        case POSITION_REL_LOOP:
            position_rel_loop_run();
            speed_loop_run();
            current_loop_run();
            voltage_control();
            break;
        default:
            break;
        }
        break;
    case SMO_CONTROL: // SMO无感控制
        switch (foc_mode.loop_mode)
        {
        case VOLTAGE_LOOP:
            voltage_control();
            break;
        case CURRENT_LOOP:
            current_loop_run();
            voltage_control();
            break;
        case SPEED_LOOP:
            speed_loop_run();
            current_loop_run();
            voltage_control();
            break;
        default:
            break;
        }
        break;
    case ENCODER_SMO_CONTROL:
        switch (foc_mode.loop_mode)
        {
        case SPEED_LOOP:
            speed_loop_run();
            if (foc_mode.weak_mag)
                weak_mag_loop_run();
        case CURRENT_LOOP:
            current_loop_run();
        case VOLTAGE_LOOP:
            voltage_control();
            break;
        case POSITION_ABS_LOOP:
            position_abs_loop_run();
            speed_loop_run();
            current_loop_run();
            voltage_control();
            break;
        case POSITION_REL_LOOP:
            position_rel_loop_run();
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

// 设置速度指令（立即生效）
void fFOC_SetOmegaIM(float value)
{
    foc_val.omega_ref = value;
    foc_val.omega_con = value; // 立即生效，跳过斜坡
}

// 设置各环指令值
void fFOC_SetTargetValue(float *value)
{
    switch (foc_mode.loop_mode)
    {
    case VOLTAGE_LOOP:
        foc_val.uq = value[0];
        foc_val.ud = value[1];
        break;
    case CURRENT_LOOP:
        foc_val.iq_ref = value[0];
        foc_val.id_ref = value[1];
        break; // 修正：缺少break
    case SPEED_LOOP:
        foc_val.omega_ref = value[0];
        break;
    case POSITION_ABS_LOOP:
    case POSITION_REL_LOOP:
        foc_val.pos_ref = value[0];
        if (foc_mode.pvt_mode)
            fPVT_SetOmega(value[1]);
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
    case SMO_CONTROL:
        foc_mode.SMO_enable = true;
        foc_mode.Encoder_enable = false;
        break;
    default: // 融合模式
        foc_mode.Encoder_enable = true;
        foc_mode.SMO_enable = true;
        break;
    }
}
// 切换控制环模式
void fFOC_SetLoopMode(eLoopMode mode)
{
    foc_val_reset();
    foc_mode.loop_mode = mode;
}

// 强制停机
bool fFOC_Shutdown(void)
{
    if (fabsf(foc_val.omega_fb) < 0.1f)
        return true;
    // todo:完善不同模式下的停机方式
    if (foc_mode.loop_mode != SPEED_LOOP)
        fFOC_SetLoopMode(SPEED_LOOP);
    float omega_shutdown = -foc_val.omega_fb * 0.5f;
    fFOC_SetTargetValue(&omega_shutdown);
    return false;
}