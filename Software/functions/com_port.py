import serial
import serial.tools.list_ports
import threading
import time
import struct
import logging
from queue import Queue, Full, Empty
from PyQt5.QtCore import QTimer, QObject, pyqtSignal

from protocol import Pkt, Cidx, Fidx

from functions.message_show import (
    send_simple_message,
    send_titled_message,
    MSG_TYPE_NORMAL,
    MSG_TYPE_SUCCESS,
    MSG_TYPE_INFO,
    MSG_TYPE_WARNING,
    MSG_TYPE_ERROR,
)


# ---------- 日志配置 ----------
logger = logging.getLogger("ComPort")
logger.setLevel(logging.INFO)
if not logger.handlers:
    _handler = logging.StreamHandler()
    _handler.setFormatter(logging.Formatter(
        "[%(levelname)s] %(name)s: %(message)s" 
    ))
    logger.addHandler(_handler)


# ============ 设备配置 ============
DEVICE_CONFIG = {
    "vendor_id": "0483",          # ST 的 USB VID
    "devices": {
        "5741": {"name": "XDr-P", "full_name": "X motor Drive Power"},
        "5742": {"name": "XDr-S", "full_name": "X motor Drive Standard"},
        "5740": {"name": "XDr-bl", "full_name": "XDr Bootloader"},
    },
}
# 预编译已知设备名称集合，用于快速过滤
KNOWN_DEVICE_NAMES = {info["name"] for info in DEVICE_CONFIG["devices"].values()}


class ComPort(QObject):
    """串口通信模块（重构版）"""

    # 信号
    packet_valid = pyqtSignal(int, bytes)          # 有效数据包
    connection_lost = pyqtSignal(str, bool)        # 连接丢失（原因，是否物理断开）
    ui_message = pyqtSignal(int, str, bool, int)   # UI 消息（类型，文本，只显示一次，时长ms）

    def __init__(self, mainwindow):
        super().__init__()
        self.mw = mainwindow
        self.widget = mainwindow.top_area
        self.comport_list = self.widget.com_port   # QComboBox
        self.connect_but = self.widget.connect_but # QPushButton（带 checkable）
        self.connect_but.clicked.connect(self._handleConnectBut)

        # 串口对象与状态
        self.serial_port = None
        self.is_connected = False
        self._current_ports = []                   # 当前扫描到的端口设备名列表
        self._port_info = []                       # 下拉框数据：[{device, type, display}, ...]
        self._is_bootloader_mode = False           # 是否进入 Bootloader 模式
        self._auto_connect_enabled = True          # 是否允许自动连接（仅物理断线时开启）
        self._scanned_ports = set()                # 已尝试过自动连接的端口（防止反复重连）

        # 线程控制
        self._lock = threading.Lock()
        self._stop_recv = threading.Event()
        self._stop_sender = threading.Event()
        self._recv_thread = None
        self._sender_thread = None
        self._send_queue = Queue(maxsize=50)       # 发送队列

        # 状态监测
        self._last_status_time = 0.0               # 最后一次收到状态包的时间
        self._connect_time = 0.0                   # 连接建立时间
        self._status_timeout = 8.0                 # 状态包超时阈值（秒）

        # 蓝牙握手
        self._pending_handshake = False
        self._handshake_timer = None

        # 包头尾常量 默认 USB 协议 蓝牙模式下 切换 串口的协议包头尾
        self.HEAD = Pkt.USB_PACKET_HEAD
        self.FOOT = Pkt.USB_PACKET_TAIL

        # 连接信号槽
        self._handlers = {}
        self._handlers = {}
        self.packet_valid.connect(self.handle_received_data)
        self.connection_lost.connect(self._on_connection_lost_ui)
        self.ui_message.connect(self._on_ui_message)

        # 定时器
        self.refresh_timer = QTimer()
        self.refresh_timer.timeout.connect(self._refresh_ports)
        self.refresh_timer.start(2000)

        self.monitor_timer = QTimer()
        self.monitor_timer.timeout.connect(self._monitor_connection)
        self.monitor_timer.start(1000)

        # 初始扫描
        self._refresh_ports()

    # ==================== COMMAND DISPATCH ====================
    def register_handler(self, cmd_id, callback):
        """Register a command handler"""
        self._handlers[cmd_id] = callback

    # ==================== UI 交互 ====================
    def _handleConnectBut(self):
        """连接/断开按钮"""
        if self.connect_but.isChecked():
            self._auto_connect_enabled = False
            self.connect()
        else:
            self._auto_connect_enabled = False
            self.disconnect(manual=True)

    def reset_auto_connect(self):
        """外部调用：重置自动连接状态（例如物理断开后恢复）"""
        with self._lock:
            self._auto_connect_enabled = True
            self._scanned_ports.clear()
        logger.debug("自动连接状态已重置")

    def _update_ui_state(self):
        """根据连接状态更新按钮文字和下拉框状态"""
        btn = self.connect_but
        if self.is_connected:
            btn.setText("已连接")
            btn.setValue("断开")
            btn.setChecked(True)
            self.comport_list.setEnabled(False)
        else:
            port = self.comport_list.currentText()
            btn.setText("未连接" if port else "无可用端口")
            btn.setValue("连接")
            btn.setChecked(False)
            self.comport_list.setEnabled(True)

    # ==================== 端口扫描与自动连接 ====================
    def _refresh_ports(self):
        """扫描串口并更新下拉列表（仅保留已知设备及蓝牙）"""
        if self.is_connected:
            return

        ports_info = list(serial.tools.list_ports.comports())
        current_devices = [p.device for p in ports_info]

        # 端口列表无变化则跳过
        if current_devices == self._current_ports:
            return

        self._current_ports = current_devices
        logger.debug(f"扫描到端口: {current_devices}")

        combo = self.comport_list
        old_device = self._port_info[combo.currentIndex()]["device"] \
                     if 0 <= combo.currentIndex() < len(self._port_info) else None

        # 解析端口类型（已知设备 / 蓝牙 / unknown）
        new_entries = []
        for port in ports_info:
            device_name = port.device
            hwid = port.hwid.upper() if port.hwid else ""
            desc = port.description.upper() if port.description else ""
            port_type = self._identify_device(port)

            # 丢弃无法识别的设备（如 Linux 的 /dev/ttyS*）
            if port_type == "unknown":
                continue

            entry = {
                "device": device_name,
                "type": port_type,
                "display": f"{device_name} ({port_type})",
            }
            new_entries.append(entry)

        # 重建下拉框
        combo.clear()
        self._port_info = new_entries
        for entry in new_entries:
            combo.addItem(entry["display"])

        # 尝试恢复上次选中项
        if old_device:
            for i, entry in enumerate(self._port_info):
                if entry["device"] == old_device:
                    combo.setCurrentIndex(i)
                    break
        elif combo.count() > 0:
            combo.setCurrentIndex(0)

        # 自动连接逻辑
        if self._auto_connect_enabled and not self.is_connected:
            self._try_auto_connect()

        self._update_ui_state()

    def _identify_device(self, port) -> str:
        """识别设备类型：已知设备名 / 'bluetooth' / 'unknown'"""
        hwid = port.hwid.upper() if port.hwid else ""
        desc = port.description.upper() if port.description else ""
        vid = DEVICE_CONFIG["vendor_id"].upper()
        devices = DEVICE_CONFIG["devices"]

        # 遍历已知 PID 进行匹配
        for pid, info in devices.items():
            pid_upper = pid.upper()
            # 支持多种 HWID 格式（Windows 和 Linux）
            patterns = [
                f"={vid}:{pid_upper}",          # Linux 常见格式：VID:PID=0483:5741
                f"VID:{vid}:PID:{pid_upper}",
                f"VID_{vid}&PID_{pid_upper}",   # Windows 格式
                f"{vid}:{pid_upper}",           # 某些 Linux 可能简化
            ]
            if any(p in hwid for p in patterns):
                return info["name"]

        # 蓝牙检测（Linux 下通常为 /dev/rfcommX，description 中可能有 "rfcomm"）
        if any(k in hwid or k in desc for k in ("BTH", "BLUETOOTH", "蓝牙", "RFCOMM")):
            return "bluetooth"

        return "unknown"

    def _try_auto_connect(self):
        """遍历已知设备尝试自动连接（仅当允许自动连接且未连接时）"""
        if not self._auto_connect_enabled or self.is_connected:
            return
        for i, info in enumerate(self._port_info):
            if info["type"] in KNOWN_DEVICE_NAMES and info["device"] not in self._scanned_ports:
                self.comport_list.setCurrentIndex(i)
                logger.info(f"尝试自动连接 {info['device']} ({info['type']})")
                if self.connect():
                    self._scanned_ports.add(info["device"])
                    return

    # ==================== 连接与断开 ====================
    def connect(self):
        """建立串口连接（返回 True/False）"""
        self.connect_but.setText("连接中...")

        try:
            index = self.comport_list.currentIndex()
            if index < 0 or index >= len(self._port_info):
                raise ValueError("未选择串口")

            port_info = self._port_info[index]
            device = port_info["device"]
            port_type = port_info["type"]

            # 拦截未知设备（理论上列表已过滤，但保留保护）
            if port_type == "unknown":
                self.ui_message.emit(
                    MSG_TYPE_ERROR,
                    "未识别设备：仅支持连接配置列表中的有线设备",
                    True, 3000
                )
                self._update_ui_state()
                return False

            # 判断是否为 Bootloader 模式
            self._is_bootloader_mode = (port_type == "XDr-bl")

            # 打开串口
            self.serial_port = serial.Serial(
                port=device,
                baudrate=115200,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.1,
                write_timeout=0.5,
                xonxoff=False,
                rtscts=False,
                dsrdtr=False,
            )

            self.is_connected = True
            self._connect_time = time.time()
            self._last_status_time = 0.0

            # 根据端口类型切换帧格式
            # USB: HEAD=0x3A TAIL=0x0D, UART/蓝牙: HEAD=0x55 TAIL=0xAA
            if port_type == "bluetooth":
                self.HEAD = Pkt.PACKET_HEAD
                self.FOOT = Pkt.PACKET_TAIL
            else:
                self.HEAD = Pkt.USB_PACKET_HEAD
                self.FOOT = Pkt.USB_PACKET_TAIL

            # 启动收发线程
            self._stop_recv.clear()
            self._stop_sender.clear()
            self._recv_thread = threading.Thread(
                target=self._receive_loop, daemon=True, name="SerialRecv"
            )
            self._sender_thread = threading.Thread(
                target=self._sender_loop, daemon=True, name="SerialSend"
            )
            self._recv_thread.start()
            self._sender_thread.start()

            self._auto_connect_enabled = False
            logger.info(f"串口 {device} 已打开，类型：{port_type}")

            # 根据设备类型发送握手命令
            if port_type == "bluetooth":
                self._pending_handshake = True
                self._handshake_timer = QTimer()
                self._handshake_timer.setSingleShot(True)
                self._handshake_timer.timeout.connect(self._on_handshake_timeout)
                self._handshake_timer.start(5000)
                self._send_direct_with_retry(Cidx.UC_CONNECT, bytes(), retries=2)
                logger.debug("蓝牙握手命令已发送")
            elif self._is_bootloader_mode:
                self._send_direct_with_retry(Cidx.CMD_BL_CONNECT, bytes(), retries=2)
                self.ui_message.emit(
                    MSG_TYPE_SUCCESS, "连接 bootloader，进入 IAP 模式", True, 2000
                )
                logger.debug("Bootloader 连接命令已发送")
            else:
                self._send_direct_with_retry(Cidx.UC_CONNECT, bytes(), retries=2)
                self.ui_message.emit(
                    MSG_TYPE_SUCCESS,
                    f"已连接 {device} ({port_type})",
                    True, 1500
                )
                logger.debug("有线设备连接命令已发送")

            self._update_ui_state()
            return True

        except Exception as e:
            error_msg = str(e)
            logger.error(f"连接失败：{error_msg}")

            # 特殊处理权限错误
            if any(kw in error_msg for kw in ("PermissionError", "拒绝访问", "Access is denied")):
                self._auto_connect_enabled = False
                p_name = self._port_info[self.comport_list.currentIndex()]["device"] \
                         if self.comport_list.currentIndex() >= 0 else "Unknown"
                self.ui_message.emit(
                    MSG_TYPE_ERROR, f"串口被占用或不存在：{p_name}", True, 3000
                )
            else:
                self.ui_message.emit(
                    MSG_TYPE_WARNING, f"连接失败：{error_msg}", True, 2000
                )

            self.is_connected = False
            self._update_ui_state()
            return False

    def disconnect(self, manual=True):
        """断开连接（manual 表示是否手动触发）"""
        if not self.is_connected and not self.serial_port:
            if manual:
                self._auto_connect_enabled = False
                self._update_ui_state()
            return

        # 清除蓝牙握手等待
        if self._pending_handshake:
            self._pending_handshake = False
            if self._handshake_timer:
                self._handshake_timer.stop()
                self._handshake_timer = None

        # 停止收发线程
        self._stop_recv.set()
        self._stop_sender.set()
        if self._recv_thread and self._recv_thread.is_alive():
            self._recv_thread.join(timeout=1.0)
        if self._sender_thread and self._sender_thread.is_alive():
            self._sender_thread.join(timeout=1.0)

        # 关闭串口
        try:
            if self.serial_port and self.serial_port.is_open:
                self.serial_port.close()
        except Exception as e:
            logger.debug(f"关闭串口时异常（可忽略）：{e}")

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

        logger.info("串口已断开")

    # ==================== 直接发送（用于握手，绕开队列） ====================
    def _send_direct(self, cmd_id, data_bytes) -> bool:
        """直接写入串口（不带队列，用于连接阶段）"""
        if not self.serial_port or not self.serial_port.is_open:
            return False
        packet = self._build_packet(cmd_id, data_bytes)
        with self._lock:
            try:
                self.serial_port.write(packet)
                self.serial_port.flush()
                return True
            except Exception as e:
                logger.debug(f"直接发送失败：{e}")
                return False

    def _send_direct_with_retry(self, cmd_id, data_bytes, retries=2):
        """带重试的直接发送"""
        for i in range(retries + 1):
            if self._send_direct(cmd_id, data_bytes):
                return True
            if i < retries:
                time.sleep(0.1 * (i + 1))
        logger.warning(f"直接发送重试{retries}次后仍失败 cmd={cmd_id}")
        return False

    # ==================== 公共发送接口（通过队列） ====================
    def send_packet(self, cmd_id: int, data_bytes) -> bool:
        """将数据包压入发送队列（线程安全）"""
        if not self.is_connected:
            logger.warning("尚未连接，发送被忽略")
            return False
        if not isinstance(cmd_id, int) or not (0 <= cmd_id <= 255):
            raise ValueError("cmd_id 必须是 0-255 的整数")
        try:
            self._send_queue.put_nowait(self._build_packet(cmd_id, data_bytes))
            return True
        except Full:
            logger.warning(f"发送队列满，丢弃 cmd={cmd_id}")
            self.ui_message.emit(MSG_TYPE_WARNING, "发送队列满，数据包已丢弃", True, 1000)
            return False

    @staticmethod
    def _crc8(data):
        """CRC-8/ATM: poly=0x07"""
        crc = 0
        for byte in data:
            crc ^= byte
            for _ in range(8):
                crc = ((crc << 1) ^ 0x07) if (crc & 0x80) else (crc << 1)
            crc &= 0xFF
        return crc

    def _build_packet(self, cmd_id, data_bytes):
        """构造协议包：HEAD + cmd_id + length + data + checksum + FOOT"""
        data_bytes = bytes(data_bytes) if isinstance(data_bytes, (bytes, bytearray, list)) else bytes()
        length = min(len(data_bytes), 255)
        packet = bytearray([self.HEAD, cmd_id, length])
        packet.extend(data_bytes[:length])
        packet.append(self._crc8(data_bytes[:length]))
        packet.append(self.FOOT)
        return packet

    # ==================== 收发线程 ====================
    def _sender_loop(self):
        """发送线程：从队列取包并写入串口"""
        while not self._stop_sender.is_set():
            try:
                packet = self._send_queue.get(timeout=0.1)
            except Empty:
                continue
            if packet is None:
                break
            with self._lock:
                try:
                    if not self.serial_port or not self.serial_port.is_open:
                        self._handle_connection_lost("串口已关闭", is_physical=True)
                        continue
                    # 流控：若缓冲区过大则稍后重试
                    if hasattr(self.serial_port, "out_waiting") and self.serial_port.out_waiting > 2048:
                        self._send_queue.put(packet)
                        time.sleep(0.05)
                        continue
                    self.serial_port.write(packet)
                    self.serial_port.flush()
                except (serial.SerialTimeoutException, OSError, serial.SerialException) as e:
                    self._handle_connection_lost(f"发送异常：{e}", is_physical=True)
        logger.debug("发送线程退出")

    def _receive_loop(self):
        """接收线程：读取串口数据并解析协议包"""
        buffer = bytearray()
        while not self._stop_recv.is_set() and self.is_connected:
            try:
                if not self.serial_port or not self.serial_port.is_open:
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
                    self._handle_connection_lost(f"接收异常：{e}", is_physical=True)
                break
            except Exception as e:
                if not self._stop_recv.is_set():
                    logger.error(f"接收线程未知异常：{e}")
                break
        buffer.clear()
        logger.debug("接收线程退出")

    def _parse_buffer(self, buffer: bytearray):
        """从缓冲区提取完整协议包并发射信号"""
        while len(buffer) >= 5:
            header_idx = buffer.find(self.HEAD)
            if header_idx == -1:
                buffer.clear()
                return
            if header_idx > 0:
                del buffer[:header_idx]
            if len(buffer) < 5:
                return

            cmd_id = buffer[1]
            length = buffer[2]
            min_len = 5 + length
            if len(buffer) < min_len:
                return
            if buffer[min_len - 1] != self.FOOT:
                del buffer[0]
                continue

            data_bytes = buffer[3: 3 + length]
            calc_chk = self._crc8(data_bytes)
            if buffer[3 + length] == calc_chk:
                self.packet_valid.emit(int(cmd_id), bytes(data_bytes))
            else:
                logger.debug(f"校验失败：cmd={cmd_id}, 期望={calc_chk:02X}, 实际={buffer[3+length]:02X}")
            del buffer[:min_len]

        # 防止缓冲区无限增长
        if len(buffer) > 2048:
            logger.warning("接收缓冲区过大，已清空")
            buffer.clear()

    # ==================== 连接状态监测 ====================
    def _handle_connection_lost(self, reason: str, is_physical: bool = False):
        """内部处理连接丢失"""
        if not self.is_connected:
            return
        with self._lock:
            if self.is_connected:
                self.is_connected = False
                logger.warning(f"连接丢失：{reason}，物理断开={is_physical}")

                # 物理断开时允许自动重连
                self._auto_connect_enabled = is_physical
                if is_physical:
                    self._scanned_ports.clear()

                self.connection_lost.emit(reason, is_physical)

    def _on_connection_lost_ui(self, reason, is_physical):
        """连接丢失信号的 UI 处理"""
        self.disconnect(manual=False)
        if is_physical:
            send_simple_message(MSG_TYPE_WARNING, f"设备已移除：{reason}", True, 3000)
        else:
            send_simple_message(MSG_TYPE_ERROR, f"通信异常：{reason}", True, 3000)

    def _on_ui_message(self, msg_type, text, show_once, duration):
        """UI 消息信号的转发"""
        send_simple_message(msg_type, text, show_once, duration)

    def _monitor_connection(self):
        """定时检查连接健康状态（超时监测）"""
        if not self.is_connected:
            return
        if not self.serial_port or not self.serial_port.is_open:
            self._handle_connection_lost("物理连接断开", is_physical=True)
            return

        now = time.time()
        timeout = self._status_timeout
        if self._last_status_time > 0:
            if now - self._last_status_time > timeout:
                self._handle_connection_lost(f"状态包超时（{timeout}秒）", is_physical=False)
        elif now - self._connect_time > timeout * 2:
            self._handle_connection_lost(f"连接后{timeout*2}秒未收到首个状态包", is_physical=False)

    def update_status_time(self):
        """收到有效状态包时更新最后活跃时间"""
        self._last_status_time = time.time()

    # ==================== 蓝牙握手相关 ====================
    def confirm_device_handshake(self):
        """设备返回握手成功时调用，完成连接确认"""
        if not self._pending_handshake:
            return False
        if self._handshake_timer:
            self._handshake_timer.stop()
            self._handshake_timer = None

        self._pending_handshake = False
        self._update_ui_state()
        self.ui_message.emit(
            MSG_TYPE_SUCCESS,
            f"设备 {self.comport_list.currentText()} 握手成功，已连接",
            True, 1500
        )
        logger.info("蓝牙握手成功")
        return True

    def _on_handshake_timeout(self):
        """蓝牙握手超时处理"""
        if self._pending_handshake:
            self._pending_handshake = False
            self.disconnect(manual=False)
            self.ui_message.emit(
                MSG_TYPE_ERROR, "设备握手超时（5s内未收到响应），请重试", True, 3000
            )
            logger.warning("蓝牙握手超时")

    # ==================== 数据分派 ====================
    def handle_received_data(self, cmd_id: int, data: bytes):
        """Dispatch to registered handlers"""
        if self._is_bootloader_mode:
            try:
                self.mw.IAP._iap_cmd_received(cmd_id, data)
            except Exception as e:
                logger.error(f"Bootloader data error: {e}")
            return

        handler = self._handlers.get(cmd_id)
        if handler:
            try:
                handler(data)
            except Exception as e:
                logger.error(f"Handler error cmd={cmd_id:#x}: {e}")
        else:
            logger.debug(f"Unhandled cmd={cmd_id:#x}")
