from .data_show import status
from .parameter import Mode
import struct
from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import QListWidgetItem

class LIdx:
    num = 0          # 序号
    time = 1         # 发生时间
    fault = 2        # 错误
    warning = 3      # 警告
    foc_mode = 4     # FOC模式
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


class LogManager:
    def __init__(self, main_window, com_port):
        self.mw = main_window
        self.com = com_port
        self.loglist=self.mw.ui.LogList
        self.loglist.itemClicked.connect(self.show_log)
        self.logs = []

        # 建立 索引(Int) -> UI控件 的映射
        self.log_map = {
            LIdx.num: self.mw.ui.num,
            LIdx.time: self.mw.ui.time,
            LIdx.fault: self.mw.ui.log_error,
            LIdx.warning: self.mw.ui.log_warning,
            LIdx.foc_mode: self.mw.ui.log_FOCmode,
            LIdx.loop_mode: self.mw.ui.log_loopmode,
            LIdx.usb_status: self.mw.ui.usb_state,
            LIdx.can_status: self.mw.ui.can_state,
            LIdx.flash_status: self.mw.ui.flash_state,
            LIdx.encode_status: self.mw.ui.encoder_state,
            LIdx.voltage: self.mw.ui.log_voltage,
            LIdx.temperature: self.mw.ui.log_temperature,
            LIdx.iu: self.mw.ui.log_iu,
            LIdx.iv: self.mw.ui.log_iv,
            LIdx.iw: self.mw.ui.log_iw,
            LIdx.id: self.mw.ui.log_id,
            LIdx.id_ref: self.mw.ui.log_id_ref,
            LIdx.iq: self.mw.ui.log_iq,
            LIdx.iq_ref: self.mw.ui.log_iq_ref,
            LIdx.speed: self.mw.ui.log_speed,
            LIdx.target_speed: self.mw.ui.log_speed_ref,
            LIdx.position: self.mw.ui.log_position,
            LIdx.target_position: self.mw.ui.log_pos_ref,
        }
    
    def add_log(self, log_bytes):
        if len(log_bytes) < 10:
            raise ValueError("log_bytes too short, need at least 10 bytes")

        new_log = []

        # 前10个字节：作为整数（或保持为 bytes，根据需求）
        for i in range(10):
            new_log.append(log_bytes[i])  # 这是 int 类型（Python 3 中 bytes[i] 是 int）

        #由于结构体的对齐方式，这里需要空两个字节
        # 剩余部分：每4字节解析为一个 float
        remaining = log_bytes[12:]
        if len(remaining) % 4 != 0:
            raise ValueError("Remaining bytes after 10 must be multiple of 4 for float32")

        # 解析 float
        num_floats = len(remaining) // 4
        for j in range(num_floats):
            start = j * 4
            val = struct.unpack('<f', remaining[start:start+4])[0]  # [0] 取出 float 值
            new_log.append(val)

        self.logs.append(new_log)
        index=new_log[LIdx.num]

        item = QListWidgetItem("第  "+str(index)+"  条日志")
        item.setData(Qt.UserRole, index)  # 存储序号到 item
        self.loglist.addItem(item)
        

    def show_log(self, item):
        
        index=item.data(Qt.UserRole)
        self.log_map[LIdx.num].setText(str(self.logs[index][LIdx.num]))
        hour=int(self.logs[index][LIdx.time]/3600)
        minute=int(self.logs[index][LIdx.time]%3600/60)
        second=int(self.logs[index][LIdx.time]%60)
        time=str(hour)+':'+str(minute)+':'+str(second)
        self.log_map[LIdx.time].setText(time)
        self.log_map[LIdx.fault].setText(status.fault_state[int(self.logs[index][LIdx.fault])])
        self.log_map[LIdx.warning].setText(status.warning_state[int(self.logs[index][LIdx.warning])])
        self.log_map[LIdx.foc_mode].setText(Mode.run_mode[int(self.logs[index][LIdx.foc_mode])])
        self.log_map[LIdx.loop_mode].setText(Mode.loop_mode[int(self.logs[index][LIdx.loop_mode])])
        self.log_map[LIdx.usb_status].setText(status.drive_state[int(self.logs[index][LIdx.usb_status])])
        self.log_map[LIdx.can_status].setText(status.drive_state[int(self.logs[index][LIdx.can_status])])
        self.log_map[LIdx.flash_status].setText(status.drive_state[int(self.logs[index][LIdx.flash_status])])
        self.log_map[LIdx.encode_status].setText(status.drive_state[int(self.logs[index][LIdx.encode_status])])
        for i in range(10, 23):
            print(i)
            self.log_map[i].setText(str(self.logs[index][i]))