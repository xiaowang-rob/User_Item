import serial
import serial.tools.list_ports  # 修复：必须显式导入 tools 子模块
import threading
from PyQt5.QtCore import QTimer,QObject,pyqtSignal

class ComPort(QObject):
    # 定义信号：参数为 (cmd_id: int, data: bytes)
    packet_valid = pyqtSignal(int, bytes)
    def __init__(self, main_window):
        super().__init__()  # 必须调用父类构造

        self.mw = main_window # 主窗口对象
        self.serial_port = None  # 串口对象
        self.is_connected = False  # 串口连接状态
        
        # 协议定义
        self.HEADER = 0x3A  # 示例包头 ':'
        self.FOOTER = 0x0D  # 示例包尾 '\r'

        self._recv_thread = None
        self._stop_recv = threading.Event()

        # 自动刷新串口列表定时器
        self.refresh_timer = QTimer()
        self.refresh_timer.timeout.connect(self._refresh_ports)
        self.refresh_timer.start(2000)

    def _refresh_ports(self):
        """更新 UI 中的串口列表"""
        ports = [p.device for p in serial.tools.list_ports.comports()]
        combo = self.mw.ui.comboBox
        current = combo.currentText()
        combo.clear()
        combo.addItems(ports)
        if current in ports:
            combo.setCurrentText(current)

    def connect(self):
        """建立连接"""
        try:
            port = self.mw.ui.comboBox.currentText()
            if not port:
                print("未选择串口")
                return False
            baud = 115200
            self.serial_port = serial.Serial(
                port=port,
                baudrate=baud,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.1,  # 注意：接收线程用短超时
                xonxoff=False,
                rtscts=False,
                dsrdtr=False
            )
            self.is_connected = True
            
            # 启动接收线程
            self._stop_recv.clear()
            self._recv_thread = threading.Thread(target=self._receive_loop, daemon=True)
            self._recv_thread.start()
            
            return True
        except Exception as e:
            print(f"连接异常: {e}")
            return False
        
    def disconnect(self):
        """断开连接"""
        self._stop_recv.set()
        if self._recv_thread and self._recv_thread.is_alive():
            self._recv_thread.join(timeout=1.0)
        
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
        checksum = sum(data_bytes) & 0x01
        packet.append(checksum)
        packet.append(self.FOOTER)
        try:
            self.serial_port.write(packet)
            self.serial_port.flush()
        except Exception as e:
            print(f"发送异常: {e}")
    def _receive_loop(self):
        """接收线程主循环"""
        buffer = bytearray()
        while not self._stop_recv.is_set() and self.is_connected:
            try:
                if self.serial_port.in_waiting > 0:
                    data = self.serial_port.read(self.serial_port.in_waiting)
                    buffer.extend(data)
                    self._parse_buffer(buffer)
                else:
                    # 避免 CPU 占用过高
                    self._stop_recv.wait(timeout=0.01)
            except Exception as e:
                if not self._stop_recv.is_set():
                    print(f"接收线程异常: {e}")
                break
        # 清理
        buffer.clear()

    def _parse_buffer(self, buffer):
        """从缓冲区中解析完整数据包"""
        while len(buffer) >= 5:  # 最小包长度：HEAD(1) + ID(1) + LEN(1) + CHECK(1) + FOOT(1)
            # 查找包头
            header_idx = buffer.find(self.HEADER)
            if header_idx == -1:
                buffer.clear()  # 没有包头，清空
                return
            
            if header_idx > 0:
                # 丢弃包头前的垃圾数据
                del buffer[:header_idx]
            
            if len(buffer) < 5:
                return  # 数据不足
            
            cmd_id = buffer[1]
            length = buffer[2]
            min_packet_len = 5 + length  # HEAD + ID + LEN + DATA + CHECK + FOOT
            
            if len(buffer) < min_packet_len:
                return  # 数据不完整，等待更多
            
            # 检查包尾
            if buffer[min_packet_len - 1] != self.FOOTER:
                # 包尾不对，可能是错包，跳过当前字节
                del buffer[0]
                continue
            
            # 提取数据和校验和
            data_bytes = buffer[3:3 + length]
            received_checksum = buffer[3 + length]
            
            calculated_checksum = sum(data_bytes) & 0x01
            
            if received_checksum == calculated_checksum:
                # 校验成功！交给上层处理
                self._on_packet_received(cmd_id, bytes(data_bytes))
            else:
                print(f"校验失败: expected {calculated_checksum:02X}, got {received_checksum:02X}")
            
            # 移除已处理的包
            del buffer[:min_packet_len]
        
        # 如果 buffer 很大但无法解析，防止内存泄漏
        if len(buffer) > 1024:
            buffer.clear()
    def _on_packet_received(self, cmd_id, data):
        """校验成功后，发出信号（线程安全）"""
        self.packet_valid.emit(cmd_id, data)