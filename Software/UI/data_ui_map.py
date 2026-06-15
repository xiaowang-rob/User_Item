from protocol import Pidx, Sidx, Lidx


# ====================================
# UI 控件与下位机索引的映射关系
# ====================================
class Data_UI_Map:
    def __init__(self, main_window):
        self.mw = main_window
        data_page = self.mw.data_page
        top_area = self.mw.top_area

        # 参数编辑控件映射（用于写入/读取参数）
        self.param_map = {
            # u8 类型
            Pidx.ENCODER_CHIP: data_page.encoder_input,
            Pidx.SENSOR_MODE: data_page.sensormode_input,
            Pidx.RUN_MODE: data_page.runmode_input,
            Pidx.CAN_MODE: data_page.can_mode_input,
            Pidx.VAGUE_PID_MODE: data_page.vaguePID_input,
            Pidx.PVT_MODE: data_page.PVT_mode_input,
            Pidx.TRAJ_TYPE: data_page.TRAJ_mode_input,
            Pidx.MOTOR_POLEPAIRS: data_page.motor_polepairs_input,
            # u32 类型
            Pidx.CAN_ID: data_page.CAN_ID_input,
            # float 类型
            Pidx.THETA_OFFSET: data_page.offsetangle_input,
            Pidx.MOTOR_KV: data_page.motor_KV_input,
            Pidx.MOTOR_RS: data_page.motor_resistance_input,
            Pidx.MOTOR_Ld: data_page.motor_Ld_input,
            Pidx.MOTOR_Lq: data_page.motor_Lq_input,
            Pidx.MOTOR_PSIF: data_page.motor_psif_input,
            Pidx.MOTOR_KE: data_page.motor_Ke_input,
            Pidx.MOTOR_J: data_page.motor_J_input,
            Pidx.MOTOR_B: data_page.motor_B_input,
            Pidx.KP_SPEED: data_page.speed_loop_P_input,
            Pidx.KI_SPEED: data_page.speed_loop_I_input,
            Pidx.KP_POSITION: data_page.position_loop_P_input,
            Pidx.KI_POSITION: data_page.position_loop_I_input,
            Pidx.KD_POSITION: data_page.position_loop_D_input,
            Pidx.MIT_KP: data_page.mit_kp_input,
            Pidx.MIT_KD: data_page.mit_kd_input,
            Pidx.MIT_TFF: data_page.mit_tau_ff_input,
            Pidx.MIT_TMAX: data_page.mit_tau_max_input,
            Pidx.TUNE_CURRENT: data_page.tune_current,
            Pidx.LIMIT_CURRENT: data_page.limit_current,
            Pidx.LIMIT_SPEED: data_page.limit_speed,
            Pidx.LIMIT_POSITION_MIN: data_page.min_position,
            Pidx.LIMIT_POSITION_MAX: data_page.max_position,
            Pidx.TOLERANCE_TIME: data_page.tolerance_time,
            Pidx.TOLERANCE_LIMIT: data_page.tolerance_limit,
            Pidx.TRAJ_MAX_RATE: data_page.traj_max_rate,
            Pidx.TRAJ_MAX_ACC: data_page.traj_max_acc,
            Pidx.TRAJ_MAX_JERK: data_page.traj_max_jerk,
            Pidx.TRAJ_TOLERANCE: data_page.traj_tolerance,
        }

        # 参数显示控件映射（顶部状态栏）
        self.param_show_map = {
            Pidx.SENSOR_MODE: top_area.sensormode_show,
            Pidx.RUN_MODE: top_area.runmode_show,
        }

        # 实时状态显示映射
        self.status_map = {
            Sidx.TUNE_STATE: top_area.state_show,
            Sidx.FOC_STATE: top_area.state_show,
            Sidx.FAULT: top_area.fault_warnning_show,
            Sidx.WARNING: top_area.fault_warnning_show,
            Sidx.TEMPERATURE: top_area.temp_show,
            Sidx.VBUS: top_area.Vbus_show,
        }

        # 日志页面字段映射
        self.log_map = {
            Lidx.num: data_page.num,
            Lidx.time: data_page.time,
            Lidx.fault: data_page.fault,
            Lidx.warning: data_page.warning,
            Lidx.sensor_mode: data_page.sensor_mode,
            Lidx.run_mode: data_page.run_mode,
            Lidx.can_status: data_page.can_status,
            Lidx.encode_status: data_page.encode_status,
            Lidx.voltage: data_page.voltage,
            Lidx.temperature: data_page.temperature,
            Lidx.iu: data_page.iu,
            Lidx.iv: data_page.iv,
            Lidx.iw: data_page.iw,
            Lidx.id: data_page.id,
            Lidx.id_ref: data_page.id_ref,
            Lidx.iq: data_page.iq,
            Lidx.iq_ref: data_page.iq_ref,
            Lidx.speed: data_page.speed,
            Lidx.target_speed: data_page.target_speed,
            Lidx.position: data_page.position,
            Lidx.target_position: data_page.target_position,
        }
