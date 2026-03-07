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

# ============ 设备配置 ============
DEVICE_CONFIG = {
    'vendor_id': '0483',
    'devices': {
        '5741': {'name': 'XDr-P', 'full_name': 'X motor Drive Power'},
        '5742': {'name': 'XDr-S', 'full_name': 'X motor Drive Standard'},
        '5740': {'name': 'XDr-bl', 'full_name': 'XDr Bootloader'},
    }
}

class ComPort(QObject):
    """串口通信模块"""
    packet_valid = pyqtSignal(int, bytes)
    connection_lost = pyqtSignal(str, bool)
    ui_message = pyqtSignal(int, str, bool, int) 
    
    def __init__(self, mainwindow):
        super().__init__()
        self.mw = mainwindow
        self.widget = mainwindow.top_area
        self.comport_list = self.widget.com_port
        self.connect_but = self.widget.connect_but
        self.connect_but.clicked.connect(self._handleConnectBut)

        self.serial_port = None
        self.is_connected = False
        self._current_ports = []
        self._port_devices = []
        self._port_is_bl = []
        self._matched_ports = []  
        
        # 【自动连接状态控制】
        self._auto_connect_enabled = True 
        self._scanned_ports = set()
        
        self._lock = QMutex()
        self._stop_recv = threading.Event()
        self._stop_sender = threading.Event()
        self._recv_thread = None
        self._sender_thread = None
        
        self._send_queue = Queue(maxsize=50)
        
        self._last_status_time = 0.0
        self._connect_time = 0.0
        self._status_timeout = 5.0

        self.packet_valid.connect(self.mw.data_process.handle_received_data)
        self.connection_lost.connect(self._on_connection_lost_ui)
        self.ui_message.connect(self._on_ui_message) 

        self._refresh_ports()
        self.refresh_timer = QTimer()
        self.refresh_timer.timeout.connect(self._refresh_ports)
        self.refresh_timer.start(2000)
        
        self.monitor_timer = QTimer()
        self.monitor_timer.timeout.connect(self._monitor_connection)
        self.monitor_timer.start(1000)

    def _handleConnectBut(self):
        if self.connect_but.isChecked():
            self._auto_connect_enabled = False
            self.connect()
        else:
            self._auto_connect_enabled = False
            self.disconnect(manual=True)

    def reset_auto_connect(self):
        """重置并开启自动连接（子线程安全）"""
        self._lock.lock()
        try:
            self._auto_connect_enabled = True
            self._scanned_ports.clear()
        finally:
            self._lock.unlock()

    def _update_ui_state(self):
        btn = self.connect_but
        if self.is_connected:
            btn.setText("已连接")
            if hasattr(btn, 'setValue'): btn.setValue("断开")
            btn.setChecked(True)
            self.comport_list.setEnabled(False)
        else:
            port = self.comport_list.currentText()
            if port:
                btn.setText("未连接")
                if hasattr(btn, 'setValue'): btn.setValue("连接")
            else:
                btn.setText("无可用端口")
                if hasattr(btn, 'setValue'): btn.setValue("连接")
            btn.setChecked(False)
            self.comport_list.setEnabled(True)

    def _refresh_ports(self):
        """刷新可用串口列表，处理自动连接逻辑"""
        if self.is_connected:
            return

        ports_info = serial.tools.list_ports.comports()
        current_devices = [p.device for p in ports_info]

        if current_devices == self._current_ports:
            return
        
        # 保存当前选中的设备
        combo = self.comport_list
        current_index = combo.currentIndex()
        selected_device = None
        if current_index >= 0 and len(self._port_devices) > current_index:
            selected_device = self._port_devices[current_index]
        
        # 重建列表
        combo.clear()
        self._port_devices = []
        self._port_is_bl = []
        self._matched_ports = []  # 【关键】每次刷新清空匹配列表
        
        vid = DEVICE_CONFIG['vendor_id'].upper()
        devices = DEVICE_CONFIG['devices']
        
        for port in ports_info:
            device_name = port.device
            hwid = port.hwid.upper()

            matched_pid = None
            device_info = None
            is_bl_pid = False
        
            for pid, info in devices.items():
                pid_upper = pid.upper()
                search_patterns = [
                    f"={vid}:{pid_upper}", f"VID:{vid}:PID:{pid_upper}",
                    f"VID_{vid}&PID_{pid_upper}", f"VID:{vid}&PID_{pid_upper}",
                ]
                
                for pattern in search_patterns:
                    if pattern in hwid:
                        matched_pid = pid_upper
                        device_info = info
                        if pid_upper == '5740':
                            is_bl_pid = True
                        break
                if matched_pid:
                    break
            
            # 【关键】将匹配到的设备存入列表，供自动连接直接使用
            if matched_pid:
                self._matched_ports.append(device_name)
            
            if matched_pid and device_info:
                display_text = f"{device_name} ({device_info['name']})"
            else:
                display_text = device_name
            
            combo.addItem(display_text)
            self._port_devices.append(device_name)
            self._port_is_bl.append(is_bl_pid)
        
        if selected_device and selected_device in self._port_devices:
            combo.setCurrentIndex(self._port_devices.index(selected_device))
        elif combo.count() > 0:
            combo.setCurrentIndex(0)
        
        self._current_ports = current_devices
        
        # 尝试自动连接
        if self._auto_connect_enabled and not self.is_connected:
            print(f"[调试] 自动连接启用，尝试连接，端口数={len(ports_info)}")
            self._try_auto_connect()  # 不再传递 ports_info
            
        if not self.is_connected:
            self._update_ui_state()

    def _try_auto_connect(self):
        """尝试自动连接符合条件的设备"""
        if not self._auto_connect_enabled or self.is_connected:
            return

        # 【关键】直接使用 _refresh_ports 准备好的匹配列表，不再重复解析 HWID
        for device_name in self._matched_ports:
            if device_name in self._scanned_ports:
                continue
                
            if device_name in self._port_devices:
                idx = self._port_devices.index(device_name)
                self.comport_list.setCurrentIndex(idx)
                if self.connect():
                    self._scanned_ports.add(device_name)
                    return 

    def connect(self):
        """建立串口连接"""
        btn = self.connect_but
        btn.setText("连接中...")

        try:
            index = self.comport_list.currentIndex()
            if index < 0 or index >= len(self._port_devices):
                raise ValueError("未选择串口")
            port = self._port_devices[index]
            if not port:
                raise ValueError("未选择串口")
            
            self._is_bootloader_mode = self._port_is_bl[index] if index < len(self._port_is_bl) else False

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
            
            # 连接成功后，关闭自动连接功能
            self._auto_connect_enabled = False
            
            if not self._is_bootloader_mode:
                self._send_with_retry(Cidx.UC_CONNECT, bytes(), retries=2)
                self.ui_message.emit(MSG_TYPE_SUCCESS, f"已连接 {self.comport_list.currentText()}", True, 1500)
            else:
                self._send_with_retry(Cidx.CMD_BL_CONNECT, bytes(), retries=2)
                self.ui_message.emit(MSG_TYPE_SUCCESS, f"连接 bootloader，进入 IAP 模式", True, 2000)
            
            self._update_ui_state()
            return True
            
        except Exception as e:
            error_msg = str(e)
            print(f"连接异常：{error_msg}")
            
            disconnect_keywords = ['PermissionError', '拒绝访问', 'Access is denied']
            if any(kw in error_msg for kw in disconnect_keywords):
                self._auto_connect_enabled = False
                try:
                    p_name = self._port_devices[self.comport_list.currentIndex()]
                except:
                    p_name = "Unknown"
                self.ui_message.emit(MSG_TYPE_ERROR, f"串口被占用或不存在：{p_name}", True, 3000)
            else:
                self.ui_message.emit(MSG_TYPE_WARNING, f"连接失败：{error_msg}", True, 2000)
            
            self.is_connected = False
            self._update_ui_state()
            return False

    def disconnect(self, manual=True):
        if not self.is_connected and not self.serial_port:
            if manual:
                self._auto_connect_enabled = False
                self._update_ui_state()
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
        self._is_bootloader_mode = False
        
        if manual:
            self._auto_connect_enabled = False
            self._update_ui_state()
            self.ui_message.emit(MSG_TYPE_SUCCESS, "已断开连接", True, 1000)
        else:
            self._update_ui_state()

    def _build_packet(self, cmd_id, data_bytes):
        if not isinstance(data_bytes, (bytes, bytearray, list)):
            data_bytes = bytes(0)
        else:
            data_bytes = bytes(data_bytes)
            
        length = len(data_bytes)
        if length > 255:
            length = 255
            
        packet = bytearray()
        packet.append(HEAD)
        packet.append(cmd_id)
        packet.append(length)
        packet.extend(data_bytes[:length])
        packet.append(sum(data_bytes[:length]) & 0x01)
        packet.append(FOOT)
        return packet

    def _sender_loop(self):
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
                    if not self.serial_port or not self.serial_port.is_open:
                        self._handle_connection_lost("串口已关闭", is_physical=True)
                        continue
                    
                    if hasattr(self.serial_port, 'out_waiting'):
                        if self.serial_port.out_waiting > 2048:
                            self._send_queue.put(packet)
                            time.sleep(0.05)
                            continue
                    
                    self.serial_port.write(packet)
                    self.serial_port.flush()
                except (serial.SerialTimeoutException, OSError, serial.SerialException) as e:
                    self._handle_connection_lost(f"发送失败：{str(e)}", is_physical=True)
                finally:
                    self._lock.unlock()
            
            except Exception as e:
                if not self._stop_sender.is_set():
                    print(f"[串口] 发送线程未知异常：{type(e).__name__}: {e}")

    def _handle_connection_lost(self, reason, is_physical=False):
        if not self.is_connected:
            return
        
        if is_physical and self.serial_port and self.serial_port.portstr:
            ports = [p.device for p in serial.tools.list_ports.comports()]
            if self.serial_port.portstr not in ports:
                is_physical = True
            else:
                is_physical = True 

        self._lock.lock()
        try:
            if self.is_connected:
                self.is_connected = False
                print(f"[串口] 连接断开：{reason}")
                
                if is_physical:
                    self._auto_connect_enabled = True
                    self._scanned_ports.clear()
                    print("[系统] 检测到物理断开，已启用自动重连")
                else:
                    self._auto_connect_enabled = False
                    print("[系统] 检测到逻辑超时/错误，禁用自动重连")
                
                self._is_bootloader_mode = False
                self.connection_lost.emit(reason, is_physical)
        finally:
            self._lock.unlock()
    
    def _on_connection_lost_ui(self, reason, is_physical):
        self.disconnect(manual=False)
        
        if is_physical:
            send_simple_message(MSG_TYPE_WARNING, f"设备已移除：{reason}", True, 3000)
        else:
            send_simple_message(MSG_TYPE_ERROR, f"通信异常：{reason}", True, 3000)

    def _on_ui_message(self, msg_type, text, show_once, duration):
        """在主线程显示消息（信号槽机制）"""
        send_simple_message(msg_type, text, show_once, duration)

    def _monitor_connection(self):
        if not self.is_connected:
            return
        
        if self._is_bootloader_mode:
            return
        
        if not self.serial_port or not self.serial_port.is_open:
            self._handle_connection_lost("物理连接断开", is_physical=True)
            return
        
        current_time = time.time()
        
        if self._last_status_time > 0:
            if current_time - self._last_status_time > self._status_timeout:
                self._handle_connection_lost(
                    f"状态包超时（{self._status_timeout}秒内未收到下位机状态包）",
                    is_physical=False
                )
        else:
            if current_time - self._connect_time > self._status_timeout * 2:
                self._handle_connection_lost(
                    f"连接后{self._status_timeout * 2}秒内未收到首个状态包",
                    is_physical=False
                )

    def update_status_time(self):
        self._last_status_time = time.time()

    def send_packet(self, cmd_id, data_bytes):
        if not self.is_connected:
            return False
        
        if not isinstance(cmd_id, int) or not (0 <= cmd_id <= 255):
            raise ValueError("cmd_id 必须是 0-255 的整数")
        
        packet = self._build_packet(cmd_id, data_bytes)
        
        try:
            self._send_queue.put_nowait(packet)
            return True
        except Full:
            print(f"[串口警告] 发送队列满，丢弃 cmd={cmd_id} 的数据包")
            self.ui_message.emit(MSG_TYPE_WARNING, "发送队列满，数据包已丢弃", True, 1000)
            return False

    def _send_nowait(self, cmd_id, data_bytes):
        if not self.is_connected:
            return False
        
        packet = self._build_packet(cmd_id, data_bytes)

        try:
            self._lock.lock()
            try:
                if self.serial_port and self.serial_port.is_open:
                    self.serial_port.write(packet)
                    self.serial_port.flush()
                    return True
            finally:
                self._lock.unlock()
        except:
            pass
        return False

    def _send_with_retry(self, cmd_id, data_bytes, retries=2):
        for i in range(retries + 1):
            if self._send_nowait(cmd_id, data_bytes):
                return True
            if i < retries:
                time.sleep(0.1 * (i + 1))
        return False

    def _receive_loop(self):
        buffer = bytearray()
        while not self._stop_recv.is_set() and self.is_connected:
            try:
                if not self.serial_port or not self.serial_port.is_open:
                    print("[调试] 接收线程检测到端口关闭") 
                    self._handle_connection_lost("接收线程检测到端口关闭", is_physical=True)
                    break

                if self.serial_port.in_waiting > 0:
                    data = self.serial_port.read(min(self.serial_port.in_waiting, 4096))
                    if data:
                        buffer.extend(data)
                        self._parse_buffer(buffer)
                else:
                    self._stop_recv.wait(timeout=0.01)
            except (serial.SerialException, OSError) as e:
                if not self._stop_recv.is_set():
                    self._handle_connection_lost(f"接收失败：{str(e)}", is_physical=True)
                break
            except Exception as e:
                if not self._stop_recv.is_set():
                    print(f"接收线程未知异常：{e}")
                break
        
        buffer.clear()

    def _parse_buffer(self, buffer):
        while len(buffer) >= 5:
            header_idx = buffer.find(HEAD)
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
            
            if buffer[min_packet_len - 1] != FOOT:
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