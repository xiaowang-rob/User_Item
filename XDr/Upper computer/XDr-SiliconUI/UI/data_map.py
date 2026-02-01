#参数映射表-对应下位机参数索引
class Pidx:
    # bool 类型参数
    SENSOR_MODE          = 0
    LOOP_MODE            = 1
    SW_CANQUEUE          = 2
    SW_WEAKMAG           = 3
    SW_FAN               = 4
    SW_VAGUE_PID         = 5
    SW_PVT               = 6

    MOTOR_WIRE_SEQUENCE  = 7
    MOTOR_POLEPAIRS      = 8
    FREQ_CURRENT_LOOP    = 9
    FREQ_SPEED_LOOP      = 10
    FREQ_POSITION_LOOP   = 11
    #u32 类型参数
    CAN_ID               = 12
    # float 类型参数
    F_PWM                = 13
    F_CURRENT_LOOP       = 14
    F_SPEED_LOOP         = 15
    F_POSITION_LOOP      = 16
    THETA_OFFSET         = 17
    MOTOR_RS             = 18
    MOTOR_LS             = 19
    MOTOR_PSIF           = 20
    MOTOR_KE             = 21
    MOTOR_J              = 22
    MOTOR_B              = 23
    KP_CURRENT           = 24
    KI_CURRENT           = 25
    KP_WEAKMAG           = 26
    KI_WEAKMAG           = 27
    KP_SPEED             = 28
    KI_SPEED             = 29
    KP_POSITION          = 30
    KI_POSITION          = 31
    KD_POSITION          = 32
    LIMIT_CURRENT        = 33
    LIMIT_SPEED          = 34
    LIMIT_POSITION_MIN   = 35
    LIMIT_POSITION_MAX   = 36
    TOLERANCE_TIME       = 37
    TOLERANCE_VOLTAGE    = 38
    TOLERANCE_CURRENT    = 39
    TOLERANCE_SPEED      = 40
    TOLERANCE_POSITION   = 41
    STARTUP_ACC          = 42
    ALIGN_CURRENT        = 43
    ALIGN_TIME           = 44
    OPEN_LOOP_CURRENT    = 45
    OPEN_LOOP_SPEED      = 46
    CHANGE_LOOP_SPEED    = 47


#日志映射表-对应下位机日志索引
class Lidx:
    num = 0          # 序号
    time = 1         # 发生时间
    fault = 2        # 错误
    warning = 3      # 警告
    sensor_mode = 4     # 感应模式
    loop_mode = 5    # 环模式（如：位置环、速度环、电流环）
    usb_status = 6   # USB状态
    can_status = 7   # CAN状态
    flash_status = 8 # 闪存状态
    encode_status = 9 # 编码状态
    voltage = 10     # 电压
    temperature = 11 # 温度
    iu = 12          # 相电流 U
    iv = 13          # 相电流 V
    iw = 14          # 相电流 W
    id = 15          # d轴电流
    id_ref = 16      # d轴电流参考值
    iq = 17          # q轴电流
    iq_ref = 18      # q轴电流参考值
    speed = 19       # 速度
    target_speed = 20 # 目标速度
    position = 21    # 位置
    target_position = 22 # 目标位置

#数据映射表-对应下位机数据索引
class Didx:
    SYSTEM_state = 0
    FOC_state = 1
    FAULT = 2
    WARNING = 3
    TEMPERATURE = 4 
    VBUS = 5
    VOLTAGE_U = 6
    VOLTAGE_V = 7
    VOLTAGE_W = 8
    VOLTAGE_q = 9
    VOLTAGE_d = 10
    CURRENT_U = 11
    CURRENT_V = 12
    CURRENT_W = 13
    CURRENT_q = 14
    CURRENT_d = 15
    CURRENT_q_ref = 16
    CURRENT_d_ref = 17
    SPEED = 18
    SPEED_con = 19
    SPEED_ref = 20
    THETA_elec = 21
    THETA_mech = 22
    POSITION = 23
    POSITION_con = 24
    POSITION_ref = 25

#控制映射表-对应下位机控制索引
class Cidx:
    UC_CONNECT    = 0xF0
    UC_DISCONNECT = 0xFE
    START_TUNNING = 0xF1
    BRAKE         = 0xF2
    FOC_NRST      = 0xF3
    ENABLE        = 0xF4
    DISABLE       = 0xF5
    PROTECT_RESET = 0xF6
    LOG_GET       = 0xF7
    LOG_ERASE     = 0xF8
    PARAM_ERASE   = 0x01
    PARAM_WRITE   = 0x02
    PARAM_READ    = 0x03
    PARAM_SAVE    = 0x04
    CMD_REFVALUE_SET = 0x21
    CMD_MODE_SET     = 0x22
    CMD_STREAM_GET   = 0x23
    CMD_STREAM_SET   = 0x25
    
#模式、状态映射表-对应下位机模式、状态索引
class Midx:
    sensor_mode=["编码器","smo","无"]
    loop_mode=["电压环","电流环","速度环","绝对位置环","相对位置环"]
    can_mode=["实时处理","队列处理"]
    weakmag_mode=["禁用","启动"]
    vague_PID_mode=["禁用","启动"]
    pvt_mode=["禁用","启动"]
    fan_mode=["禁用","启动"]

    sys_state=["INIT","RUN","ERROR"]
    foc_state=["IDLE","TUNE","RESET","ENABLE","DISABLE","RUNNING","SHUTDOWN","FAULT","WARNING"]
    fault_state = [
    "无故障",
    "闪存离线",
    "整定超时",
    "极对数不匹配",
    "电机参数异常",
    "过压",
    "低电压",
    "过流",
    "CAN初始化失败",
    "CAN通信失败"
]
    warning_state = [
    "无警告",
    "过温",
    "超速",
    "位置超限",
    "编码器离线",
    "编码器通信错误"
]
    drive_state=["离线","在线","运行错误"]
    
    data_select=["NONE"] + [
        "温度",
        "Vbus",
        "VOL_U",
        "VOL_V",
        "VOL_W",
        "VOL_q",
        "VOL_d",
        "CUR_U",
        "CUR_V",
        "CUR_W",
        "CUR_q",
        "CUR_d",
        "CUR_q_ref",
        "CUR_d_ref",
        "SPE",
        "SPE_con",
        "SPE_ref",
        "THE_elec",
        "THE_mech",
        "POS",
        "POS_con",
        "POS_ref"
    ]

class data_map:
    def __init__(self,main_window):
        self.main_window=main_window
        self.param_map={


        }
