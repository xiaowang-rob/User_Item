# 此文件由 gen_protocol.py 自动生成，请勿手动修改，相关配置在 protocol.json 中

class Pidx:
    ENCODER_CHIP       = 0  # 编码器芯片
    SENSOR_MODE        = 1  # 感应模式
    RUN_MODE           = 2  # 运行模式
    CAN_MODE           = 3  # CAN模式
    VAGUE_PID_MODE     = 4  # 模糊PID
    PVT_MODE           = 5  # PVT模式
    TRAJ_TYPE          = 6  # 轨迹规划器类型
    MOTOR_POLEPAIRS    = 7  # 电机极对数
    CAN_ID             = 8  # CAN ID
    THETA_OFFSET       = 9  # 角度补偿
    MOTOR_KV           = 10  # KV
    MOTOR_RS           = 11  # 相电阻
    MOTOR_Ld           = 12  # Ld
    MOTOR_Lq           = 13  # Lq
    MOTOR_PSIF         = 14  # 磁链
    MOTOR_KE           = 15  # 反电动势常数
    MOTOR_J            = 16  # 转动惯量
    MOTOR_B            = 17  # 摩擦系数
    KP_SPEED           = 18  # 速度环比例
    KI_SPEED           = 19  # 速度环积分
    KP_POSITION        = 20  # 位置环比例
    KI_POSITION        = 21  # 位置环积分
    KD_POSITION        = 22  # 位置环微分
    TUNE_CURRENT       = 23  # 校准电流
    LIMIT_CURRENT      = 24  # 电流限幅
    LIMIT_SPEED        = 25  # 速度限幅
    LIMIT_POSITION_MIN = 26  # 位置限幅最小值
    LIMIT_POSITION_MAX = 27  # 位置限幅最大值
    TOLERANCE_TIME     = 28  # 容忍时间
    TOLERANCE_LIMIT    = 29  # 超限容忍度
    TRAJ_MAX_RATE      = 30  # 轨迹最大变化率
    TRAJ_MAX_ACC       = 31  # 轨迹最大加速度
    TRAJ_MAX_JERK      = 32  # 轨迹最大加加速度
    TRAJ_TOLERANCE     = 33  # 轨迹规划容差
    NUM_OF_PARAM       = 34  # 参数总数

class Lidx:
    num             = 0  # 序号
    time            = 1  # 发生时间
    fault           = 2  # 错误
    warning         = 3  # 警告
    sensor_mode     = 4  # 感应模式
    run_mode        = 5  # 运行模式
    can_status      = 6  # CAN状态
    encode_status   = 7  # 编码状态
    voltage         = 8  # 电压
    temperature     = 9  # 温度
    iu              = 10  # 相电流 U
    iv              = 11  # 相电流 V
    iw              = 12  # 相电流 W
    id              = 13  # d轴电流
    id_ref          = 14  # Id_ref
    iq              = 15  # q轴电流
    iq_ref          = 16  # Iq_ref
    speed           = 17  # 速度
    target_speed    = 18  # 目标速度
    position        = 19  # 位置
    target_position = 20  # 目标位置
    log_num         = 21  # 日志字段数量

class Didx:
    CURRENT_U     = 0  # U相电流
    CURRENT_V     = 1  # V相电流
    CURRENT_W     = 2  # W相电流
    VOLTAGE_Q     = 3  # q轴电压
    VOLTAGE_D     = 4  # d轴电压
    CURRENT_ALPHA = 5  # α轴电流
    CURRENT_BETA  = 6  # β轴电流
    CURRENT_Q     = 7  # q轴电流
    CURRENT_D     = 8  # d轴电流
    CURRENT_Q_REF = 9  # Iq_ref
    CURRENT_D_REF = 10  # Id_ref
    SPEED         = 11  # 速度
    SPEED_REF     = 12  # 目标速度
    THETA_ELEC    = 13  # 电角度
    THETA_MECH    = 14  # 机械角度
    POSITION      = 15  # 位置
    POSITION_REF  = 16  # 目标位置

class Cidx:
    UC_CONNECT           = 0xf0  # 上位机连接
    UC_DISCONNECT        = 0xfe  # 上位机断开
    START_TUNNING        = 0xf1  # 开始调参
    BRAKE                = 0xf2  # 刹车
    FOC_NRST             = 0xf3  # FOC复位
    CMD_ENABLE           = 0xf4  # 电机使能
    CMD_DISABLE          = 0xf5  # 电机失能
    LOG_GET              = 0xf7  # 获取日志
    LOG_ERASE            = 0xf8  # 日志擦除
    PARAM_ERASE          = 0x01  # 参数擦除
    PARAM_WRITE          = 0x02  # 参数写入
    PARAM_READ           = 0x03  # 参数读取
    PARAM_SAVE           = 0x04  # 参数保存
    CMD_REFVALUE_SET     = 0x21  # 目标值设置
    CMD_MODE_SET         = 0x22  # 模式设置
    CMD_STREAM_GET       = 0x23  # 监测值获取
    CMD_STREAM_SET       = 0x25  # 数据流设置
    CMD_SET_ZERO_POS     = 0x26  # 设置零点
    CMD_SET_LIMIT_POS    = 0x27  # 设置极限位置
    CMD_SYSTEM_RESET     = 0x30  # 系统复位
    CMD_IAP_ENTER        = 0x31  # 进入IAP模式
    CMD_IAP_ERASE_FLASH  = 0x32  # 擦除Flash
    CMD_IAP_WRITE_FLASH  = 0x33  # 写入Flash
    CMD_IAP_VERIFY_FLASH = 0x34  # 校验Flash
    CMD_IAP_EXIT         = 0x35  # 退出IAP模式

class Fidx:
    FEEDBACK_EXECUTE = 0xf0  # 成功
    FEEDBACK_FAILURE = 0xfe  # 失败

class Pkt:
    USB_PACKET_HEAD  = 0x3A
    USB_PACKET_TAIL  = 0x0D
    PACKET_HEAD      = 0x55
    PACKET_TAIL      = 0xAA
    MAX_FRAME_LENGTH = 128

class Sidx:
    TUNE_STATE  = 0  # 整定状态
    FOC_STATE   = 1  # FOC状态
    FAULT       = 2  # 故障
    WARNING     = 3  # 警告
    TEMPERATURE = 4  # 温度
    VBUS        = 5  # 母线电压

class Midx:
    sensor_mode = ["编码反馈", "无感观测", "混合模式"]
    encoder_chip = ["MT6816", "MT6835", "AS5047", "芯片数量"]
    run_mode = ["电流模式", "速度模式", "位置模式", "开环模式"]
    target_type = ["拖动电流/A", "目标速度/rpm", "目标位置/deg", "无"]
    can_mode = ["实时处理", "队列处理", "实时反馈", "队列反馈"]
    vague_PID_mode = ["禁用", "启动"]
    pvt_mode = ["禁用", "PV", "PT"]
    traj_type = ["禁用", "梯形", "S形"]
    tune_state = ["INIT", "IDLE", "电阻校准", "电感校准", "编码器校准", "电气参数校准", "机械参数校准", "完成", "失败"]
    foc_state = ["IDLE", "TUNE", "RESET", "ENABLE", "DISABLE", "RUNNING", "SHUTDOWN", "FAULT", "WARNING"]
    fault_state = ["NONE", "FLASH离线", "整定电流异常", "极对数不匹配", "电机堵转", "电阻电感校准失败", "编码器校准失败", "电气参数校准失败", "机械参数校准失败", "过电压", "低电压", "过电流", "CAN初始化失败", "CAN通信异常"]
    warning_state = ["NONE", "过温", "超速", "位置超限", "编码器无响应", "编码器通信错误"]
    drive_state = ["离线", "在线", "运行错误", "运行正常"]
    data_select = ["NONE", "U相电流", "V相电流", "W相电流", "q轴电压", "d轴电压", "α轴电流", "β轴电流", "q轴电流", "d轴电流", "Iq_ref", "Id_ref", "速度", "目标速度", "电角度", "机械角度", "位置", "目标位置"]

