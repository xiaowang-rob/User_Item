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

# ============ 设备配置 (直接写在这里或从 JSON 读取) ============
DEVICE_CONFIG = {
    'vendor_id': '0483',
    'devices': {
        '5741': {'name': 'XDr-P', 'full_name': 'X motor Drive Power'},
        '5742': {'name': 'XDr-S', 'full_name': 'X motor Drive Standard'},
        '5740': {'name': 'XDr-bl', 'full_name': 'XDr Bootloader'},
    }
}
# =============================================================


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
        self._port_devices = []
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
        """根据连接状态更新 UI"""
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
        """刷新可用串口列表，根据 PID 识别设备"""
        ports_info = serial.tools.list_ports.comports()
        current_devices = [p.device for p in ports_info]
        
        if current_devices == self._current_ports:
            return
        
        self._current_ports = current_devices
        combo = self.comport_list
        
        current_index = combo.currentIndex()
        selected_device = None
        if current_index >= 0 and len(self._port_devices) > current_index:
            selected_device = self._port_devices[current_index]
        
        combo.clear()
        self._port_devices = []
        
        vid = DEVICE_CONFIG['vendor_id'].upper()  # 0483
        devices = DEVICE_CONFIG['devices']
        
        for port in ports_info:
            device_name = port.device
            hwid = port.hwid.upper()

            matched_pid = None
            device_info = None
        
            
            for pid, info in devices.items():
                pid_upper = pid.upper()
                
                # 搜索多种格式
                search_patterns = [
                    f"={vid}:{pid_upper}",      # VID:PID=0483:5741
                    f"VID:{vid}:PID:{pid_upper}",  # 带标签
                    f"VID_{vid}&PID_{pid_upper}",  # 下划线格式
                    f"VID:{vid}&PID:{pid_upper}",  # 混合格式
                ]
                
                for pattern in search_patterns:
                    if pattern in hwid:
                        matched_pid = pid_upper
                        device_info = info
                        break
                
                if matched_pid:
                    break
            
            # 构造显示文本
            if matched_pid and device_info:
                display_text = f"{device_name} ({device_info['name']})"
            else:
                display_text = device_name
            
            combo.addItem(display_text)
            self._port_devices.append(device_name)
        
        if selected_device and selected_device in self._port_devices:
            combo.setCurrentIndex(self._port_devices.index(selected_device))
        elif combo.count() > 0:
            combo.setCurrentIndex(0)
        
        if not self.is_connected:
            self._update_ui_state()

    def connect(self):
        """建立串口连接"""
        btn = self.connect_but
        btn.setText("连接中...")
        QApplication.processEvents()

        try:
            index = self.comport_list.currentIndex()
            if index < 0 or index >= len(self._port_devices):
                raise ValueError("未选择串口")
            port = self._port_devices[index]
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
            send_simple_message(MSG_TYPE_SUCCESS, f"已连接 {self.comport_list.currentText()}", True, 1500)
            return True
            
        except Exception as e:
            error_msg = str(e)
            print(f"连接异常：{error_msg}")
            
            disconnect_keywords = [
                'PermissionError', '拒绝访问', 'Access is denied',
                'WriteFile failed', '串口已断开', 'port is closed',
                '设备未就绪', 'could not open port'
            ]
            if any(kw in error_msg for kw in disconnect_keywords):
                send_simple_message(MSG_TYPE_ERROR, f"串口被占用或不存在：{port}", True, 3000)
            else:
                send_simple_message(MSG_TYPE_WARNING, f"连接失败：{error_msg}", True, 2000)
            self._update_ui_state()
            self.disconnect()
            return False
    def connect_by_pid(self, target_pid, timeout=5.0):
        """
        根据 PID 自动扫描并连接，不依赖 ComboBox 选择
        用于 IAP 模式下自动重连 Bootloader
        """
        start_time = time.time()
        while time.time() - start_time < timeout:
            ports_info = serial.tools.list_ports.comports()
            vid = DEVICE_CONFIG['vendor_id'].upper()
            
            for port in ports_info:
                hwid = port.hwid.upper()
                # 匹配 PID
                if f"PID:{target_pid.upper()}" in hwid or f"PID_{target_pid.upper()}" in hwid:
                    try:
                        # 尝试打开串口
                        self.serial_port = serial.Serial(
                            port=port.device,
                            baudrate=115200,
                            bytesize=serial.EIGHTBITS,
                            parity=serial.PARITY_NONE,
                            stopbits=serial.STOPBITS_ONE,
                            timeout=0.1,
                            write_timeout=0.5,
                        )
                        self.is_connected = True
                        self._connect_time = time.time()
                        
                        # 启动收发线程
                        self._stop_recv.clear()
                        self._stop_sender.clear()
                        self._recv_thread = threading.Thread(target=self._receive_loop, daemon=True)
                        self._sender_thread = threading.Thread(target=self._sender_loop, daemon=True)
                        self._recv_thread.start()
                        self._sender_thread.start()
                        
                        # 【重要】同步更新 UI 的 ComboBox，防止后续操作找不到端口
                        # 注意：在子线程调用 UI 控件有风险，但为了兼容你的架构，这里保持原逻辑
                        # 建议在实际项目中通过 signal 通知主线程更新
                        idx = self.comport_list.findText(port.device)
                        if idx < 0:
                            self.comport_list.addItem(port.device)
                            idx = self.comport_list.count() - 1
                        self.comport_list.setCurrentIndex(idx)
                        self._update_ui_state()
                        
                        return True
                    except Exception as e:
                        # 如果打开失败（如端口被占用），继续循环等待
                        print(f"尝试连接 {port.device} 失败：{e}")
                        time.sleep(0.5) 
                        break # 跳出 for 循环，进入 while 下一次重试
            
            time.sleep(0.3) # 扫描间隔
        return False
    
    def disconnect(self, force=False):
        """断开串口连接"""
        if not self.is_connected and not force:
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
                self.serial_port = None  # 设为 None 让 GC 回收
        except:
            pass
        
        self.serial_port = None 
        self.is_connected = False
        self._last_status_time = 0.0
        self._connect_time = 0.0
        
        # 只有非 force 模式才更新 UI 和弹窗
        if not force:
            self._update_ui_state()
            send_simple_message(MSG_TYPE_SUCCESS, "已断开连接", True, 1000)
        else:
            self._update_ui_state()

    def _sender_loop(self):
        """发送线程主循环"""
        while not self._stop_sender.is_set():
            try:
                try:
                    packet = self._send_queue.get(timeout=0.1)
                except Empty:
                    continue
                
                if packet is None:
                    break
                
                self._lock.lock()
                try:
                    if not self._is_port_valid():
                        self._handle_connection_lost("串口已关闭")
                        continue
                    
                    if hasattr(self.serial_port, 'out_waiting'):
                        if self.serial_port.out_waiting > 2048:
                            time.sleep(0.05)
                            self._send_queue.put(packet)
                            continue
                    
                    self.serial_port.write(packet)
                    self.serial_port.flush()
                except (serial.SerialTimeoutException, OSError) as e:
                    error_str = str(e)
                    if "timed out" in error_str or "PermissionError" in error_str or "Access is denied" in error_str:
                        self._handle_connection_lost(f"发送失败：{error_str}")
                    else:
                        print(f"[串口] 发送 OS 错误：{error_str}")
                finally:
                    self._lock.unlock()
            
            except KeyboardInterrupt:
                break
            except Exception as e:
                if not self._stop_sender.is_set():
                    print(f"[串口] 发送线程未知异常：{type(e).__name__}: {e}")

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
                print(f"[串口] 连接断开：{reason}")
                self.connection_lost.emit(reason)
        finally:
            self._lock.unlock()
    
    def _on_connection_lost_ui(self, reason):
        """在主线程处理连接丢失的 UI 更新"""
        self.disconnect()
        send_simple_message(MSG_TYPE_ERROR, f"串口断开：{reason}", True, 3000)

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
        """发送数据包"""
        if not self.is_connected:
            return False
        
        if not isinstance(cmd_id, int) or not (0 <= cmd_id <= 255):
            raise ValueError("cmd_id 必须是 0-255 的整数")
        
        if not isinstance(data_bytes, (bytes, bytearray, list)):
            raise TypeError("data_bytes 必须是 bytes/bytearray/list")
        
        data_bytes = bytes(data_bytes)
        length = len(data_bytes)
        if length > 255:
            raise ValueError("数据长度超过 255 字节限制")
        
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
            print(f"[串口警告] 发送队列满，丢弃 cmd={cmd_id} 的数据包")
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
                        self._handle_connection_lost(f"接收失败：{error_str}")
                    else:
                        print(f"[串口警告] 非致命接收错误：{e}")
                break
            except Exception as e:
                if not self._stop_recv.is_set():
                    print(f"接收线程未知异常：{e}")
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
                print(f"[串口] 校验失败：cmd={cmd_id}, exp={calculated_checksum:02X}, got={received_checksum:02X}")
            
            del buffer[:min_packet_len]
        
        if len(buffer) > 2048:
            buffer.clear()