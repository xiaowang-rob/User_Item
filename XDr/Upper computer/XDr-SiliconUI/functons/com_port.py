import serial
import serial.tools.list_ports
import threading
import time
from queue import Queue, Full, Empty
from PyQt5.QtCore import QTimer, QObject, pyqtSignal, QMutex
from PyQt5.QtWidgets import QApplication
from functons.message_show import (
    send_simple_message,
    MSG_TYPE_ERROR,
    MSG_TYPE_SUCCESS,
    MSG_TYPE_WARNING,
)
from UI.data_ui_map import Cidx

HEAD = 0x3A  # 协议包头 ':'
FOOT = 0x0D  # 协议包尾 '\r'


class ComPort(QObject):
    """串口通信模块"""
    packet_valid = pyqtSignal(int, bytes)
    connection_lost = pyqtSignal(str)
    
    def __init__(self, widget):
        super().__init__()
        self.widget = widget
        self.comport_list = self.widget.com_port
        self.connect_but = self.widget.connect_but
        self.connect_but.clicked.connect(self._handleConnectBut)

        self.serial_port = None
        self.is_connected = False
        self._current_ports = []
        self.HEADER = HEAD
        self.FOOTER = FOOT
        
        self._lock = QMutex()
        self._stop_recv = threading.Event()
        self._stop_sender = threading.Event()
        self._recv_thread = None
        self._sender_thread = None
        
        self._send_queue = Queue(maxsize=50)
        
        self._last_status_time = 0.0
        self._connect_time = 0.0
        self._status_timeout = 5.0
        
        self._refresh_ports()
        self.refresh_timer = QTimer()
        self.refresh_timer.timeout.connect(self._refresh_ports)
        self.refresh_timer.start(2000)
        
        self.monitor_timer = QTimer()
        self.monitor_timer.timeout.connect(self._monitor_connection)
        self.monitor_timer.start(1000)
        
        self.connection_lost.connect(self._on_connection_lost_ui)

    def _handleConnectBut(self):
        if self.connect_but.isChecked():
            self.connect()
        else:
            self.disconnect()

    def _update_ui_state(self):
        """根据连接状态更新UI"""
        btn = self.connect_but
        if self.is_connected:
            btn.setText("已连接")
            btn.setValue("断开")
            btn.setChecked(True)
            self.comport_list.setEnabled(False)
        else:
            port = self.comport_list.currentText()
            if port:
                btn.setText("未连接")
                btn.setValue("连接")
            else:
                btn.setText("无可用端口")
                btn.setValue("连接")
            btn.setChecked(False)
            self.comport_list.setEnabled(True)

    def _refresh_ports(self):
        """刷新可用串口列表"""
        ports = [p.device for p in serial.tools.list_ports.comports()]
        if ports == self._current_ports:
            return
        
        self._current_ports = ports
        combo = self.comport_list
        current = combo.currentText()
        combo.clear()
        combo.addItems(ports)
        
        if current in ports:
            combo.setCurrentText(current)
        elif ports:
            combo.setCurrentIndex(0)
        
        if not self.is_connected:
            self._update_ui_state()

    def connect(self):
        """建立串口连接"""
        btn = self.connect_but
        btn.setText("连接中...")
        QApplication.processEvents()

        try:
            port = self.comport_list.currentText()
            if not port:
                raise ValueError("未选择串口")
            
            self.serial_port = serial.Serial(
                port=port,
                baudrate=115200,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.1,
                write_timeout=0.5,
                xonxoff=False,
                rtscts=False,
                dsrdtr=False
            )
            
            self.is_connected = True
            self._connect_time = time.time()
            self._last_status_time = 0.0
            
            self._stop_recv.clear()
            self._stop_sender.clear()
            self._recv_thread = threading.Thread(target=self._receive_loop, daemon=True, name="SerialReceiver")
            self._sender_thread = threading.Thread(target=self._sender_loop, daemon=True, name="SerialSender")
            self._recv_thread.start()
            self._sender_thread.start()
            
            self._send_with_retry(Cidx.UC_CONNECT, bytes(), retries=2)
            
            self._update_ui_state()
            send_simple_message(MSG_TYPE_SUCCESS, f"已连接 {port}", True, 1500)
            return True
            
        except Exception as e:
            error_msg = str(e)
            print(f"连接异常: {error_msg}")
            
            disconnect_keywords = [
                'PermissionError', '拒绝访问', 'Access is denied',
                'WriteFile failed', '串口已断开', 'port is closed',
                '设备未就绪', 'could not open port'
            ]
            if any(kw in error_msg for kw in disconnect_keywords):
                send_simple_message(MSG_TYPE_ERROR, f"串口被占用或不存在: {port}", True, 3000)
            else:
                send_simple_message(MSG_TYPE_WARNING, f"连接失败: {error_msg}", True, 2000)
            self._update_ui_state()
            self.disconnect()
            return False

    def disconnect(self):
        """断开串口连接"""
        if not self.is_connected:
            return
        
        self._stop_recv.set()
        self._stop_sender.set()
        
        if self._recv_thread and self._recv_thread.is_alive():
            self._recv_thread.join(timeout=1.0)
        if self._sender_thread and self._sender_thread.is_alive():
            self._sender_thread.join(timeout=1.0)
        
        try:
            if self.serial_port and self.serial_port.is_open:
                self.serial_port.close()
        except:
            pass
        
        self.serial_port = None
        self.is_connected = False
        self._last_status_time = 0.0
        self._connect_time = 0.0
        self._update_ui_state()
        send_simple_message(MSG_TYPE_SUCCESS, "已断开连接", True, 1000)

    def _sender_loop(self):
        """发送线程主循环"""
        from queue import Empty  # 显式导入Empty异常
        
        while not self._stop_sender.is_set():
            try:
                # 尝试从队列获取数据包，超时返回继续循环（正常空闲状态）
                try:
                    packet = self._send_queue.get(timeout=0.1)
                except Empty:
                    # 队列为空是正常状态，不视为异常
                    continue
                
                if packet is None:
                    break
                
                self._lock.lock()
                try:
                    # 检查串口有效性（防御性编程）
                    if not self._is_port_valid():
                        self._handle_connection_lost("串口已关闭")
                        continue
                    
                    # 流量控制：检查输出缓冲区是否积压
                    if hasattr(self.serial_port, 'out_waiting'):
                        if self.serial_port.out_waiting > 2048:
                            time.sleep(0.05)
                            self._send_queue.put(packet)  # 重新入队稍后重试
                            continue
                    
                    # 执行发送
                    self.serial_port.write(packet)
                    self.serial_port.flush()
                except (serial.SerialTimeoutException, OSError) as e:
                    error_str = str(e)
                    # 识别物理断开错误
                    if "timed out" in error_str or "PermissionError" in error_str or "Access is denied" in error_str:
                        self._handle_connection_lost(f"发送失败: {error_str}")
                    else:
                        print(f"[串口] 发送OS错误: {error_str}")
                finally:
                    self._lock.unlock()
            
            except KeyboardInterrupt:
                break
            except Exception as e:
                if not self._stop_sender.is_set():
                    print(f"[串口] 发送线程未知异常: {type(e).__name__}: {e}")

    def _is_port_valid(self):
        """检查串口是否有效"""
        return self.serial_port and self.serial_port.is_open

    def _handle_connection_lost(self, reason):
        """处理连接丢失事件"""
        if not self.is_connected:
            return
        
        self._lock.lock()
        try:
            if self.is_connected:
                self.is_connected = False
                print(f"[串口] 连接断开: {reason}")
                self.connection_lost.emit(reason)
        finally:
            self._lock.unlock()
    
    def _on_connection_lost_ui(self, reason):
        """在主线程处理连接丢失的UI更新"""
        self.disconnect()
        send_simple_message(MSG_TYPE_ERROR, f"串口断开: {reason}", True, 3000)

    def _monitor_connection(self):
        """监控连接状态"""
        if not self.is_connected:
            return
        
        if not self._is_port_valid():
            self._handle_connection_lost("物理连接断开")
            return
        
        current_time = time.time()
        
        if self._last_status_time > 0:
            if current_time - self._last_status_time > self._status_timeout:
                self._handle_connection_lost(
                    f"状态包超时（{self._status_timeout}秒内未收到下位机状态包）"
                )
        else:
            if current_time - self._connect_time > self._status_timeout * 2:
                self._handle_connection_lost(
                    f"连接后{self._status_timeout * 2}秒内未收到首个状态包"
                )

    def update_status_time(self):
        """更新最后收到状态包的时间戳"""
        self._last_status_time = time.time()

    def send_packet(self, cmd_id, data_bytes):
        """
        发送数据包
        协议格式：包头(1b) | ID(1b) | Len(1b) | Data(nb) | Checksum(1b) | 包尾(1b)
        """
        if not self.is_connected:
            return False
        
        if not isinstance(cmd_id, int) or not (0 <= cmd_id <= 255):
            raise ValueError("cmd_id 必须是0-255的整数")
        
        if not isinstance(data_bytes, (bytes, bytearray, list)):
            raise TypeError("data_bytes 必须是bytes/bytearray/list")
        
        data_bytes = bytes(data_bytes)
        length = len(data_bytes)
        if length > 255:
            raise ValueError("数据长度超过255字节限制")
        
        packet = bytearray()
        packet.append(self.HEADER)
        packet.append(cmd_id)
        packet.append(length)
        packet.extend(data_bytes)
        
        checksum = sum(data_bytes) & 0x01
        packet.append(checksum)
        packet.append(self.FOOTER)
        
        try:
            self._send_queue.put_nowait(packet)
            return True
        except Full:
            print(f"[串口警告] 发送队列满，丢弃cmd={cmd_id}的数据包")
            send_simple_message(MSG_TYPE_WARNING, "发送队列满，数据包已丢弃", True, 1000)
            return False

    def _send_nowait(self, cmd_id, data_bytes):
        """立即发送数据包（不入队）"""
        if not self.is_connected:
            return False
        
        packet = bytearray()
        packet.append(self.HEADER)
        packet.append(cmd_id)
        packet.append(len(data_bytes))
        packet.extend(data_bytes)
        packet.append(sum(data_bytes) & 0x01)
        packet.append(self.FOOTER)
        
        try:
            self._lock.lock()
            try:
                if self._is_port_valid():
                    self.serial_port.write(packet)
                    self.serial_port.flush()
                    return True
            finally:
                self._lock.unlock()
        except:
            pass
        return False

    def _send_with_retry(self, cmd_id, data_bytes, retries=2):
        """带重试机制的发送"""
        for i in range(retries + 1):
            if self._send_nowait(cmd_id, data_bytes):
                return True
            if i < retries:
                time.sleep(0.1 * (i + 1))
        return False

    def _receive_loop(self):
        """接收线程主循环"""
        buffer = bytearray()
        while not self._stop_recv.is_set() and self.is_connected:
            try:
                if self.serial_port.in_waiting > 0:
                    data = self.serial_port.read(min(self.serial_port.in_waiting, 4096))
                    if data:
                        buffer.extend(data)
                        self._parse_buffer(buffer)
                else:
                    self._stop_recv.wait(timeout=0.01)
            except (serial.SerialException, OSError) as e:
                if not self._stop_recv.is_set():
                    error_str = str(e)
                    if "read failed" in error_str or "PermissionError" in error_str or "Access is denied" in error_str:
                        self._handle_connection_lost(f"接收失败: {error_str}")
                    else:
                        print(f"[串口警告] 非致命接收错误: {e}")
                break
            except Exception as e:
                if not self._stop_recv.is_set():
                    print(f"接收线程未知异常: {e}")
                break
        
        buffer.clear()

    def _parse_buffer(self, buffer):
        """从缓冲区解析数据包"""
        while len(buffer) >= 5:
            header_idx = buffer.find(self.HEADER)
            if header_idx == -1:
                buffer.clear()
                return
            
            if header_idx > 0:
                del buffer[:header_idx]
            
            if len(buffer) < 5:
                return
            
            cmd_id = buffer[1]
            length = buffer[2]
            min_packet_len = 5 + length
            
            if len(buffer) < min_packet_len:
                return
            
            if buffer[min_packet_len - 1] != self.FOOTER:
                del buffer[0]
                continue
            
            data_bytes = buffer[3:3 + length]
            received_checksum = buffer[3 + length]
            calculated_checksum = sum(data_bytes) & 0x01
            
            if received_checksum == calculated_checksum:
                cmd_id_int = int(cmd_id)
                self.packet_valid.emit(cmd_id_int, bytes(data_bytes))
            else:
                print(f"[串口] 校验失败: cmd={cmd_id}, exp={calculated_checksum:02X}, got={received_checksum:02X}")
            
            del buffer[:min_packet_len]
        
        if len(buffer) > 2048:
            buffer.clear()