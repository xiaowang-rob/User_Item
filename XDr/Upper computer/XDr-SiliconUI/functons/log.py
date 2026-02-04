

from PyQt5.QtWidgets import QListWidgetItem
from PyQt5.QtCore import Qt
from UI.data_ui_map import Lidx,Midx,Cidx
import struct

class LogManager:
    def __init__(self, main_window):
        self.mw = main_window
        self.com=self.mw.comport

        self.loglist=self.mw.log_page.log_list
        self.read_log=self.mw.log_page.read_log_btn
        self.clear_log=self.mw.log_page.clear_log_btn

        self.read_log.clicked.connect(self.read_log_clicked)
        self.clear_log.longPressed.connect(self.clear_log_clicked)

        self.log_map=self.mw.ui_map.log_map
        self.loglist.itemClicked.connect(self.show_log)
        self.logs = []


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
        index=new_log[Lidx.num]

        item = QListWidgetItem("第  "+str(index)+"  条日志")
        item.setData(Qt.UserRole, index)  # 存储序号到 item
        self.loglist.addItem(item)
        

    def show_log(self, item):
        
        index=item.data(Qt.UserRole)
        self.log_map[Lidx.num].setText(str(self.logs[index][Lidx.num]))
        hour=int(self.logs[index][Lidx.time]/3600)
        minute=int(self.logs[index][Lidx.time]%3600/60)
        second=int(self.logs[index][Lidx.time]%60)
        time=str(hour)+':'+str(minute)+':'+str(second)
        self.log_map[Lidx.time].setText(time)
        self.log_map[Lidx.fault].setText(Midx.fault_state[int(self.logs[index][Lidx.fault])])
        self.log_map[Lidx.warning].setText(Midx.warning_state[int(self.logs[index][Lidx.warning])])
        self.log_map[Lidx.sensor_mode].setText(Midx.sensor_mode[int(self.logs[index][Lidx.sensor_mode])])
        self.log_map[Lidx.loop_mode].setText(Midx.loop_mode[int(self.logs[index][Lidx.loop_mode])])
        self.log_map[Lidx.usb_status].setText(Midx.drive_state[int(self.logs[index][Lidx.usb_status])])
        self.log_map[Lidx.can_status].setText(Midx.drive_state[int(self.logs[index][Lidx.can_status])])
        self.log_map[Lidx.flash_status].setText(Midx.drive_state[int(self.logs[index][Lidx.flash_status])])
        self.log_map[Lidx.encode_status].setText(Midx.drive_state[int(self.logs[index][Lidx.encode_status])])
        for i in range(10, 23):
            val=str(f"{self.logs[index][i]:.3f}")
            self.log_map[i].setText(val)


    def read_log_clicked(self):
        self.com.send_packet(Cidx.LOG_GET,bytes())

    def clear_log_clicked(self):
        self.loglist.clear()
        self.logs.clear()
        self.com.send_packet(Cidx.LOG_ERASE,bytes())