# ==============================
# 参数映射表 - 对应下位机参数索引
# ==============================
class Pidx:
    SENSOR_MODE          = 0
    RUN_MODE             = 1
    CAN_MODE             = 2
    VAGUE_PID_MODE       = 3
    PVT_MODE             = 4
    TRAJ_TYPE            = 5
    MOTOR_POLEPAIRS      = 6
    CAN_ID               = 7
    THETA_OFFSET         = 8
    MOTOR_KV             = 9
    MOTOR_RS             = 10
    MOTOR_Ld             = 11
    MOTOR_Lq             = 12
    MOTOR_PSIF           = 13
    MOTOR_KE             = 14
    MOTOR_J              = 15
    MOTOR_B              = 16
    KP_SPEED             = 17
    KI_SPEED             = 18
    KP_POSITION          = 19
    KI_POSITION          = 20
    KD_POSITION          = 21
    LIMIT_CURRENT        = 22
    LIMIT_SPEED          = 23
    LIMIT_POSITION_MIN   = 24
    LIMIT_POSITION_MAX   = 25
    TOLERANCE_TIME       = 26
    TOLERANCE_LIMIT      = 27
    TRAJ_MAX_RATE        = 28
    TRAJ_MAX_ACC         = 29
    TRAJ_MAX_JERK        = 30
    TRAJ_TOLERANCE       = 31

    NUM_OF_PARAM         = 32

# ============================
# 日志映射表 - 对应下位机日志索引
# ============================
class Lidx:
    num = 0 # 序号
    time = 1 # 发生时间
    fault = 2 # 错误
    warning = 3 # 警告
    sensor_mode = 4 # 感应模式
    run_mode = 5 # 运行模式
    can_status = 6 # CAN状态 
    encode_status = 7 # 编码状态 
    voltage = 8 # 电压 
    temperature = 9 # 温度 
    iu = 10 # 相电流 U 
    iv = 11 # 相电流 V 
    iw = 12 # 相电流 W 
    id = 13 # d轴电流 
    id_ref = 14 # d轴电流参考值 
    iq = 15 # q轴电流 
    iq_ref = 16 # q轴电流参考值 
    speed = 17 # 速度 
    target_speed = 18 # 目标速度 
    position = 19 # 位置 
    target_position = 20 # 目标位置 

    log_num = 21 # 日志数量


# ============================
# 数据映射表 - 对应下位机实时数据索引
# ============================
class Didx:
    SYSTEM_state       = 0
    FOC_state          = 1
    FAULT              = 2
    WARNING            = 3
    TEMPERATURE        = 4
    VBUS               = 5
    VOLTAGE_U          = 6
    VOLTAGE_V          = 7
    VOLTAGE_W          = 8
    VOLTAGE_q          = 9
    VOLTAGE_d          = 10
    CURRENT_alpha      = 11
    CURRENT_beta       = 12
    CURRENT_q          = 13  
    CURRENT_d          = 14  
    CURRENT_q_ref      = 15  
    CURRENT_d_ref      = 16  
    SPEED              = 17  
    SPEED_ref          = 18  
    THETA_elec         = 19  
    THETA_mech         = 20  
    POSITION           = 21  
    POSITION_ref       = 22  


# ============================
# 控制命令映射表 - 对应下位机控制指令
# ============================
class Cidx:
    UC_CONNECT         = 0xF0
    UC_DISCONNECT      = 0xFE
    START_TUNNING      = 0xF1
    BRAKE              = 0xF2
    FOC_NRST           = 0xF3
    ENABLE             = 0xF4
    DISABLE            = 0xF5
    LOG_GET            = 0xF7
    LOG_ERASE          = 0xF8
    PARAM_ERASE        = 0x01
    PARAM_WRITE        = 0x02
    PARAM_READ         = 0x03
    PARAM_SAVE         = 0x04
    CMD_REFVALUE_SET   = 0x21
    CMD_MODE_SET       = 0x22
    CMD_STREAM_GET     = 0x23
    CMD_STREAM_SET     = 0x25
    CMD_SET_ZERO_POS   = 0x26
    CMD_SET_LIMIT_POS  = 0x27

    CMD_SYSTEM_RESET   = 0x30   #系统复位
    CMD_BL_CONNECT     = 0x3f   #连接BL设备
    CMD_IAP_ENTER      = 0x31   #进入IAP模式
    CMD_IAP_ERASE      = 0x32   #擦除flash
    CMD_IAP_WRITE      = 0x33   #写入flash
    CMD_IAP_VERIFY     = 0x34   #校验flash
    CMD_IAP_EXIT       = 0x35   #退出IAP模式

# ============================
# 反馈映射表 - 对应下位机对上位机消息的反馈
# ============================
class Fidx:
    SUCCESS            = 0xf0
    ERROR              = 0xfe

# ============================
# 状态映射表 - 对应下位机状态索引
# ============================
class Sidx:
    SYSTEM_state       = 0
    FOC_state          = 1
    FAULT              = 2
    WARNING            = 3
    TEMPERATURE        = 4
    VBUS               = 5


# ====================================
# 模式与状态字符串映射表
# ====================================
class Midx:
    sensor_mode        = ["编码反馈", "无感观测","混合模式"]
    run_mode           = [ "电流模式", "速度模式", "位置模式"]
    target_value       = ["拖动电流/A","速度/rpm","位置/°"]
    can_mode           = ["实时处理", "队列处理","实时反馈","队列反馈"]
    vague_PID_mode     = ["禁用", "启动"]
    pvt_mode           = ["禁用", "PV","PT"]
    traj_type          = ["禁用","梯形", "S形"]

    sys_state          = ["INIT", "RUN", "ERROR"]
    foc_state          = ["IDLE", "TUNE", "RESET", "ENABLE", "DISABLE", "RUNNING", "SHUTDOWN", "FAULT", "WARNING"]
    fault_state        = [
        "无故障",
        "闪存离线",
        "整定电流异常",
        "极对数不匹配",
        "电机堵转",
        "电阻校准失败",
        "编码器校准失败",
        "电气参数校准失败",
        "机械参数校准失败",
        "过压",
        "低电压",
        "过流",
        "CAN初始化失败",
        "CAN通信异常"
    ]
    warning_state      = [
        "无警告",
        "过温",
        "超速",
        "位置超限",
        "编码器离线",
        "编码器通信错误"
    ]
    drive_state        = ["离线", "在线", "运行错误"]
    data_select        = [
        "NONE",
        "温度", "Vbus", "VOL_U", "VOL_V", "VOL_W", "VOL_q", "VOL_d",
        "I_α", "I_β", "I_q", "I_d", "I_q_ref", "I_d_ref",
        "SPE", "SPE_ref", "THE_elec", "THE_mech",
        "POS", "POS_ref"
    ]


# ====================================
# UI 控件与下位机索引的映射关系
# ====================================
class Data_UI_Map:
    def __init__(self, main_window):
        self.mw = main_window
        parameter_page = self.mw.parameter_page
        top_area       = self.mw.top_area
        log_page       = self.mw.log_page

        # 参数编辑控件映射（用于写入/读取参数）
        self.param_map = {
            # u8 类型
            Pidx.SENSOR_MODE:          parameter_page.sensormode_input,
            Pidx.RUN_MODE:            parameter_page.runmode_input,
            Pidx.CAN_MODE:             parameter_page.can_mode_input,

            Pidx.VAGUE_PID_MODE:       parameter_page.vaguePID_input,
            Pidx.PVT_MODE:             parameter_page.PVT_mode_input,
            Pidx.TRAJ_TYPE:             parameter_page.TRAJ_mode_input,

            Pidx.MOTOR_POLEPAIRS:      parameter_page.motor_polepairs_input,

            # u32 类型
            Pidx.CAN_ID:               parameter_page.CAN_ID_input,

            # float 类型
            Pidx.THETA_OFFSET:         parameter_page.offsetangle_input,
            Pidx.MOTOR_KV:             parameter_page.motor_KV_input,
            Pidx.MOTOR_RS:             parameter_page.motor_resistance_input,
            Pidx.MOTOR_Ld:             parameter_page.motor_Ld_input,
            Pidx.MOTOR_Lq:             parameter_page.motor_Lq_input,
            Pidx.MOTOR_PSIF:           parameter_page.motor_psif_input,
            Pidx.MOTOR_KE:             parameter_page.motor_Ke_input,
            Pidx.MOTOR_J:              parameter_page.motor_J_input,
            Pidx.MOTOR_B:              parameter_page.motor_B_input,
            Pidx.KP_SPEED:             parameter_page.speed_loop_P_input,
            Pidx.KI_SPEED:             parameter_page.speed_loop_I_input,
            Pidx.KP_POSITION:          parameter_page.position_loop_P_input,
            Pidx.KI_POSITION:          parameter_page.position_loop_I_input,
            Pidx.KD_POSITION:          parameter_page.position_loop_D_input,
            Pidx.LIMIT_CURRENT:        parameter_page.limit_current,
            Pidx.LIMIT_SPEED:          parameter_page.limit_speed,
            Pidx.LIMIT_POSITION_MIN:   parameter_page.min_position,
            Pidx.LIMIT_POSITION_MAX:   parameter_page.max_position,
            Pidx.TOLERANCE_TIME:       parameter_page.tolerance_time,
            Pidx.TOLERANCE_LIMIT:      parameter_page.tolerance_limit,
            Pidx.TRAJ_MAX_RATE:        parameter_page.traj_max_rate,
            Pidx.TRAJ_MAX_ACC:         parameter_page.traj_max_acc,
            Pidx.TRAJ_MAX_JERK:        parameter_page.traj_max_jerk,
            Pidx.TRAJ_TOLERANCE:       parameter_page.traj_tolerance,
        }

        # 参数显示控件映射（顶部状态栏）
        self.param_show_map = {
            Pidx.SENSOR_MODE:          top_area.sensormode_show,
            Pidx.RUN_MODE:            top_area.runmode_show,
        }

        # 实时状态显示映射
        self.status_map = {
            # Sidx.SYSTEM_state:         top_area.systemstate_show,
            Sidx.FOC_state:            top_area.focstate_show,
            Sidx.FAULT:                top_area.fault_warnning_show,
            Sidx.WARNING:              top_area.fault_warnning_show,
            Sidx.TEMPERATURE:          top_area.temp_show,
            Sidx.VBUS:                 top_area.Vbus_show,
        }

        # 日志页面字段映射
        self.log_map = {
            Lidx.num:                  log_page.num,
            Lidx.time:                 log_page.time,
            Lidx.fault:                log_page.fault,
            Lidx.warning:              log_page.warning,
            Lidx.sensor_mode:          log_page.sensor_mode,
            Lidx.run_mode:             log_page.run_mode,
            Lidx.can_status:           log_page.can_status,
            Lidx.encode_status:        log_page.encode_status,
            Lidx.voltage:              log_page.voltage,
            Lidx.temperature:          log_page.temperature,
            Lidx.iu:                   log_page.iu,
            Lidx.iv:                   log_page.iv,
            Lidx.iw:                   log_page.iw,
            Lidx.id:                   log_page.id,
            Lidx.id_ref:               log_page.id_ref,
            Lidx.iq:                   log_page.iq,
            Lidx.iq_ref:               log_page.iq_ref,
            Lidx.speed:                log_page.speed,
            Lidx.target_speed:         log_page.target_speed,
            Lidx.position:             log_page.position,
            Lidx.target_position:      log_page.target_position,
        }

