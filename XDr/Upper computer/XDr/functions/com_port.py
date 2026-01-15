import serial
import serial.tools.list_ports  # 修复：必须显式导入 tools 子模块
import threading
from PyQt5.QtCore import QTimer

class ComPort:
    def __init__(self, main_window):
        self.mw = main_window
        self.serial_port = None
        self.is_connected = False
        
        # 协议定义
        self.HEADER = 0x3A  # 示例包头 ':'
        self.FOOTER = 0x0D  # 示例包尾 '\r'
        
        # 自动刷新串口列表定时器
        self.refresh_timer = QTimer()
        self.refresh_timer.timeout.connect(self._refresh_ports)
        self.refresh_timer.start(2000)

    def _refresh_ports(self):
        """更新 UI 中的串口列表"""
        ports = [p.device for p in serial.tools.list_ports.comports()]
        combo = self.mw.ui.comport_list
        current = combo.currentText()
        combo.clear()
        combo.addItems(ports)
        if current in ports:
            combo.setCurrentText(current)

    def connect(self):
        """建立连接"""
        try:
            port = self.mw.ui.comport_list.currentText()
            baud = int(self.mw.ui.baudrate.currentText())
            self.serial_port = serial.Serial(port, baud, timeout=0.5)
            self.is_connected = True
            return True
        except Exception as e:
            self.mw.ui.warnningshow.setText(f"连接失败: {e}")
            return False

    def disconnect(self):
        """断开连接"""
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        self.is_connected = False

    def send_packet(self, cmd_id, data_bytes):
        """
        协议封装：包头(1b) | ID(1b) | Len(1b) | Data(nb) | Checksum(1b) | 包尾(1b)
        """
        if not self.is_connected: return
        
        length = len(data_bytes)
        packet = bytearray()
        packet.append(self.HEADER)
        packet.append(cmd_id)
        packet.append(length)
        packet.extend(data_bytes)
        
        # 校验和：ID + Len + Data所有字节的累加
        checksum = (cmd_id + length + sum(data_bytes)) & 0xFF
        packet.append(checksum)
        packet.append(self.FOOTER)
        
        try:
            self.serial_port.write(packet)
            self.serial_port.flush()
        except Exception as e:
            print(f"发送异常: {e}")