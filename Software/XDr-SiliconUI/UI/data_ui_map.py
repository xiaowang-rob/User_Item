from shared_constants import Pidx, Sidx, Lidx


# ====================================
# UI 控件与下位机索引的映射关系
# ====================================
class Data_UI_Map:
    def __init__(self, main_window):
        self.mw = main_window
        parameter_page = self.mw.parameter_page
        top_area = self.mw.top_area
        log_page = self.mw.log_page

        # 参数编辑控件映射（用于写入/读取参数）
        self.param_map = {
            # u8 类型
            Pidx.SENSOR_MODE: parameter_page.sensormode_input,
            Pidx.RUN_MODE: parameter_page.runmode_input,
            Pidx.CAN_MODE: parameter_page.can_mode_input,
            Pidx.VAGUE_PID_MODE: parameter_page.vaguePID_input,
            Pidx.PVT_MODE: parameter_page.PVT_mode_input,
            Pidx.TRAJ_TYPE: parameter_page.TRAJ_mode_input,
            Pidx.MOTOR_POLEPAIRS: parameter_page.motor_polepairs_input,
            # u32 类型
            Pidx.CAN_ID: parameter_page.CAN_ID_input,
            # float 类型
            Pidx.THETA_OFFSET: parameter_page.offsetangle_input,
            Pidx.MOTOR_KV: parameter_page.motor_KV_input,
            Pidx.MOTOR_RS: parameter_page.motor_resistance_input,
            Pidx.MOTOR_Ld: parameter_page.motor_Ld_input,
            Pidx.MOTOR_Lq: parameter_page.motor_Lq_input,
            Pidx.MOTOR_PSIF: parameter_page.motor_psif_input,
            Pidx.MOTOR_KE: parameter_page.motor_Ke_input,
            Pidx.MOTOR_J: parameter_page.motor_J_input,
            Pidx.MOTOR_B: parameter_page.motor_B_input,
            Pidx.KP_SPEED: parameter_page.speed_loop_P_input,
            Pidx.KI_SPEED: parameter_page.speed_loop_I_input,
            Pidx.KP_POSITION: parameter_page.position_loop_P_input,
            Pidx.KI_POSITION: parameter_page.position_loop_I_input,
            Pidx.KD_POSITION: parameter_page.position_loop_D_input,
            Pidx.LIMIT_CURRENT: parameter_page.limit_current,
            Pidx.LIMIT_SPEED: parameter_page.limit_speed,
            Pidx.LIMIT_POSITION_MIN: parameter_page.min_position,
            Pidx.LIMIT_POSITION_MAX: parameter_page.max_position,
            Pidx.TOLERANCE_TIME: parameter_page.tolerance_time,
            Pidx.TOLERANCE_LIMIT: parameter_page.tolerance_limit,
            Pidx.TRAJ_MAX_RATE: parameter_page.traj_max_rate,
            Pidx.TRAJ_MAX_ACC: parameter_page.traj_max_acc,
            Pidx.TRAJ_MAX_JERK: parameter_page.traj_max_jerk,
            Pidx.TRAJ_TOLERANCE: parameter_page.traj_tolerance,
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
            Lidx.num: log_page.num,
            Lidx.time: log_page.time,
            Lidx.fault: log_page.fault,
            Lidx.warning: log_page.warning,
            Lidx.sensor_mode: log_page.sensor_mode,
            Lidx.run_mode: log_page.run_mode,
            Lidx.can_status: log_page.can_status,
            Lidx.encode_status: log_page.encode_status,
            Lidx.voltage: log_page.voltage,
            Lidx.temperature: log_page.temperature,
            Lidx.iu: log_page.iu,
            Lidx.iv: log_page.iv,
            Lidx.iw: log_page.iw,
            Lidx.id: log_page.id,
            Lidx.id_ref: log_page.id_ref,
            Lidx.iq: log_page.iq,
            Lidx.iq_ref: log_page.iq_ref,
            Lidx.speed: log_page.speed,
            Lidx.target_speed: log_page.target_speed,
            Lidx.position: log_page.position,
            Lidx.target_position: log_page.target_position,
        }
