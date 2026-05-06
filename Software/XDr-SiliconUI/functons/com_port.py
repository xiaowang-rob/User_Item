import serial
import serial.tools.list_ports
import threading
import time
from queue import Queue, Full, Empty
from PyQt5.QtCore import QTimer, QObject, pyqtSignal, QMutex
from functons.message_show import (
    send_simple_message,
    send_titled_message,
    MSG_TYPE_NORMAL,
    MSG_TYPE_SUCCESS,
    MSG_TYPE_INFO,
    MSG_TYPE_WARNING,
    MSG_TYPE_ERROR,
)
from shared_constants import Cidx, Fidx

import struct

HEAD = 0x3A  # 协议包头 ':'
FOOT = 0x0D  # 协议包尾 '\r'

# ============ 设备配置 ============
DEVICE_CONFIG = {
    "vendor_id": "0483",
    "devices": {
        "5741": {"name": "XDr-P", "full_name": "X motor Drive Power"},
        "5742": {"name": "XDr-S", "full_name": "X motor Drive Standard"},
        "5740": {"name": "XDr-bl", "full_name": "XDr Bootloader"},
    },
}
# 预编译已知设备名称集合，用于快速过滤
KNOWN_DEVICE_NAMES = {info["name"] for info in DEVICE_CONFIG["devices"].values()}


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
        self._current_ports = []  # 记录当前扫描到的原始端口号 (如 ['COM3', 'COM7'])
        self._port_devices = []  # 与下拉框索引对齐的端口号列表
        self._port_types = []  # 与下拉框索引对齐的类型/名称列表
        self._is_bootloader_mode = False  # 当前连接会话的运行模式
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
        self._status_timeout = 8.0

        # 蓝牙握手状态
        self._pending_handshake = False
        self._handshake_timer = None

        self.packet_valid.connect(self.handle_received_data)
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
        self._lock.lock()
        try:
            self._auto_connect_enabled = True
            self._scanned_ports.clear()
        finally:
            self._lock.unlock()

    def _update_ui_state(self):
        btn = self.connect_but
        if hasattr(btn, "setValue"):
            btn.setValue("断开" if self.is_connected else "连接")
        if self.is_connected:
            btn.setText("已连接")
        else:
            port = self.comport_list.currentText()
            btn.setText("未连接" if port else "无可用端口")
            btn.setChecked(False)
            self.comport_list.setEnabled(True)
            return  # 未连接时直接返回，不执行下方已连接的逻辑

        btn.setChecked(True)
        self.comport_list.setEnabled(False)

    def _refresh_ports(self):
        if self.is_connected:
            return

        ports_info = serial.tools.list_ports.comports()
        current_devices = [p.device for p in ports_info]

        if current_devices == self._current_ports:
            return

        self._current_ports = current_devices

        combo = self.comport_list
        current_index = combo.currentIndex()
        selected_device = (
            self._port_devices[current_index]
            if 0 <= current_index < len(self._port_devices)
            else None
        )

        vid = DEVICE_CONFIG["vendor_id"].upper()
        devices = DEVICE_CONFIG["devices"]

        new_entries, existing_entries = [], []

        for port in ports_info:
            device_name = port.device
            hwid = port.hwid.upper()
            desc = port.description.upper()
            port_type = "unknown"

            # 1. 匹配已知设备
            for pid, info in devices.items():
                pid_upper = pid.upper()
                patterns = [
                    f"={vid}:{pid_upper}",
                    f"VID:{vid}:PID:{pid_upper}",
                    f"VID_{vid}&PID_{pid_upper}",
                ]
                if any(p in hwid for p in patterns):
                    port_type = info["name"]
                    break

            # 2. 识别蓝牙
            if port_type == "unknown" and any(
                k in hwid or k in desc for k in ["BTH", "BLUETOOTH", "蓝牙", "RFCOMM"]
            ):
                port_type = "bluetooth"

            entry = {
                "device": device_name,
                "type": port_type,
                "display": f"{device_name} ({port_type})",
            }
            if device_name in self._current_ports and device_name not in [
                e["device"] for e in new_entries
            ]:
                existing_entries.append(entry)
            else:
                new_entries.append(entry)

        # 重建列表（新设备优先）
        combo.clear()
        self._port_devices = []
        self._port_types = []

        for entry in new_entries + existing_entries:
            combo.addItem(entry["display"])
            self._port_devices.append(entry["device"])
            self._port_types.append(entry["type"])

        # 恢复选中
        if selected_device and selected_device in self._port_devices:
            combo.setCurrentIndex(self._port_devices.index(selected_device))
        elif combo.count() > 0:
            combo.setCurrentIndex(0)

        # 自动连接
        if self._auto_connect_enabled and not self.is_connected:
            self._try_auto_connect()

        if not self.is_connected:
            self._update_ui_state()

    def _try_auto_connect(self):
        if not self._auto_connect_enabled or self.is_connected:
            return
        # 直接通过 _port_types 过滤已知有线设备
        for i, p_type in enumerate(self._port_types):
            if p_type in KNOWN_DEVICE_NAMES:
                device = self._port_devices[i]
                if device in self._scanned_ports:
                    continue
                self.comport_list.setCurrentIndex(i)
                if self.connect():
                    self._scanned_ports.add(device)
                    return

    def connect(self):
        btn = self.connect_but
        btn.setText("连接中...")

        try:
            index = self.comport_list.currentIndex()
            if index < 0 or index >= len(self._port_devices):
                raise ValueError("未选择串口")

            port_type = self._port_types[index]

            # 【拦截未知有线设备】
            if port_type == "unknown":
                self.ui_message.emit(
                    MSG_TYPE_ERROR,
                    "未识别设备：仅支持连接配置列表中的有线设备",
                    True,
                    3000,
                )
                self._update_ui_state()
                return False

            # 【通过类型直接判断 Bootloader 模式】
            self._is_bootloader_mode = port_type == "XDr-bl"

            self.serial_port = serial.Serial(
                port=self._port_devices[index],
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

            self._stop_recv.clear()
            self._stop_sender.clear()
            self._recv_thread = threading.Thread(
                target=self._receive_loop, daemon=True, name="SerialReceiver"
            )
            self._sender_thread = threading.Thread(
                target=self._sender_loop, daemon=True, name="SerialSender"
            )
            self._recv_thread.start()
            self._sender_thread.start()

            self._auto_connect_enabled = False

            # 【蓝牙握手流程】
            if port_type == "bluetooth":
                self._pending_handshake = True
                self._handshake_timer = QTimer()
                self._handshake_timer.setSingleShot(True)
                self._handshake_timer.timeout.connect(self._on_handshake_timeout)
                self._handshake_timer.start(5000)

                self._send_with_retry(Cidx.UC_CONNECT, bytes(), retries=2)
                self._update_ui_state()
                return True

            # 【已知有线设备直连】
            if self._is_bootloader_mode:
                self._send_with_retry(Cidx.CMD_BL_CONNECT, bytes(), retries=2)
                self.ui_message.emit(
                    MSG_TYPE_SUCCESS, "连接 bootloader，进入 IAP 模式", True, 2000
                )
            else:
                self._send_with_retry(Cidx.UC_CONNECT, bytes(), retries=2)
                self.ui_message.emit(
                    MSG_TYPE_SUCCESS,
                    f"已连接 {self.comport_list.currentText()}",
                    True,
                    1500,
                )

            self._update_ui_state()
            return True

        except Exception as e:
            error_msg = str(e)
            print(f"连接异常：{error_msg}")

            if any(
                kw in error_msg
                for kw in ["PermissionError", "拒绝访问", "Access is denied"]
            ):
                self._auto_connect_enabled = False
                p_name = (
                    self._port_devices[self.comport_list.currentIndex()]
                    if 0 <= self.comport_list.currentIndex() < len(self._port_devices)
                    else "Unknown"
                )
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
        if not self.is_connected and not self.serial_port:
            if manual:
                self._auto_connect_enabled = False
                self._update_ui_state()
            return

        if self._pending_handshake:
            self._pending_handshake = False
            if self._handshake_timer:
                self._handshake_timer.stop()
                self._handshake_timer = None

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

    # ==========================================
    # 对外 API：蓝牙/特殊设备握手确认
    # ==========================================
    def confirm_device_handshake(self):
        if not self._pending_handshake:
            return False
        # 可在此处添加业务层协议校验
        if self._handshake_timer:
            self._handshake_timer.stop()
            self._handshake_timer = None

        self._pending_handshake = False
        self._update_ui_state()
        self.ui_message.emit(
            MSG_TYPE_SUCCESS,
            f"设备 {self.comport_list.currentText()} 握手成功，已连接",
            True,
            1500,
        )
        return True

    def _on_handshake_timeout(self):
        if self._pending_handshake:
            self._pending_handshake = False
            self.disconnect(manual=False)
            self.ui_message.emit(
                MSG_TYPE_ERROR, "设备握手超时（5s内未收到响应），请重试", True, 3000
            )

    def _build_packet(self, cmd_id, data_bytes):
        data_bytes = (
            bytes(data_bytes)
            if isinstance(data_bytes, (bytes, bytearray, list))
            else bytes(0)
        )
        length = min(len(data_bytes), 255)
        packet = bytearray([HEAD, cmd_id, length])
        packet.extend(data_bytes[:length])
        packet.append(sum(data_bytes[:length]) & 0xFF)
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
                    if (
                        hasattr(self.serial_port, "out_waiting")
                        and self.serial_port.out_waiting > 2048
                    ):
                        self._send_queue.put(packet)
                        time.sleep(0.05)
                        continue
                    self.serial_port.write(packet)
                    self.serial_port.flush()
                except (
                    serial.SerialTimeoutException,
                    OSError,
                    serial.SerialException,
                ) as e:
                    self._handle_connection_lost(f"发送失败：{e}", is_physical=True)
                finally:
                    self._lock.unlock()
            except Exception as e:
                if not self._stop_sender.is_set():
                    print(f"[串口] 发送线程异常：{e}")

    def _handle_connection_lost(self, reason, is_physical=False):
        if not self.is_connected:
            return

        self._lock.lock()
        try:
            if self.is_connected:
                self.is_connected = False
                print(f"[串口] 连接断开：{reason}")

                self._auto_connect_enabled = is_physical
                if is_physical:
                    self._scanned_ports.clear()
                    print("[系统] 物理断开，启用自动重连")
                else:
                    print("[系统] 逻辑断开，禁用自动重连")

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
        send_simple_message(msg_type, text, show_once, duration)

    def _monitor_connection(self):
        if not self.is_connected:
            return
        if not self.serial_port or not self.serial_port.is_open:
            self._handle_connection_lost("物理连接断开", is_physical=True)
            return

        current_time = time.time()
        timeout = self._status_timeout
        if self._last_status_time > 0:
            if current_time - self._last_status_time > timeout:
                self._handle_connection_lost(
                    f"状态包超时（{timeout}秒）", is_physical=False
                )
        elif current_time - self._connect_time > timeout * 2:
            self._handle_connection_lost(
                f"连接后{timeout * 2}秒未收到首个状态包", is_physical=False
            )

    def update_status_time(self):
        self._last_status_time = time.time()

    def send_packet(self, cmd_id, data_bytes):
        if not self.is_connected:
            return False
        if not isinstance(cmd_id, int) or not (0 <= cmd_id <= 255):
            raise ValueError("cmd_id 必须是 0-255 的整数")
        try:
            self._send_queue.put_nowait(self._build_packet(cmd_id, data_bytes))
            return True
        except Full:
            print(f"[串口警告] 发送队列满，丢弃 cmd={cmd_id}")
            self.ui_message.emit(
                MSG_TYPE_WARNING, "发送队列满，数据包已丢弃", True, 1000
            )
            return False

    def _send_nowait(self, cmd_id, data_bytes):
        if not self.is_connected:
            return False
        packet = self._build_packet(cmd_id, data_bytes)
        self._lock.lock()
        try:
            if self.serial_port and self.serial_port.is_open:
                self.serial_port.write(packet)
                self.serial_port.flush()
                return True
        except:
            pass
        finally:
            self._lock.unlock()
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
                    self._handle_connection_lost(
                        "接收线程检测到端口关闭", is_physical=True
                    )
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
                    self._handle_connection_lost(f"接收失败：{e}", is_physical=True)
                break
            except Exception as e:
                if not self._stop_recv.is_set():
                    print(f"接收线程异常：{e}")
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
            min_len = 5 + length
            if len(buffer) < min_len:
                return
            if buffer[min_len - 1] != FOOT:
                del buffer[0]
                continue

            data_bytes = buffer[3 : 3 + length]
            calc_chk = sum(data_bytes) & 0xFF
            if buffer[3 + length] == calc_chk:
                self.packet_valid.emit(int(cmd_id), bytes(data_bytes))
            else:
                print(f"[串口] 校验失败：cmd={cmd_id}, exp={calc_chk:02X}")
            del buffer[:min_len]

        if len(buffer) > 2048:
            buffer.clear()

    def handle_received_data(self, cmd_id: int, data: bytes):
        """处理已解析的有效数据包"""
        if self._is_bootloader_mode:
            try:
                self.mw.IAP._iap_cmd_received(cmd_id, data)
            except Exception as e:
                print(f"Bootloader 模式数据处理异常: {e}")
        else:
            try:
                match cmd_id:
                    case Cidx.UC_CONNECT:  # UC连接成功 返回系统参数
                        self.mw.comport.update_status_time()
                        byte_len = int(len(data) / 4) + 3
                        if byte_len == 6:  # 已经连接 则接收状态并反馈
                            for i in range(byte_len):
                                if i < 4:
                                    self.mw.data_show.set_status(i, data[i])
                                else:
                                    self.mw.data_show.set_status(
                                        i,
                                        struct.unpack(
                                            "<f", data[(i - 3) * 4 : (i - 2) * 4]
                                        )[0],
                                    )
                            self.mw.data_show.show_status()
                            self.mw.comport.send_packet(Cidx.UC_CONNECT, bytes())

                        else:
                            # 解析 system_message 字符串，按逗号分隔
                            sys_msg_in = data.decode()
                            if not sys_msg_in:
                                sys_msg = ""
                            else:
                                sys_msg = "".join(
                                    c for c in sys_msg_in if 32 <= ord(c) <= 126
                                ).strip()
                            print(f"系统消息: {sys_msg}")
                            parts = sys_msg.split(",")
                            version = parts[0].strip() + " " + parts[1].strip()
                            self.mw.IAP.set_current_version(version)
                            # 定义字段标签（项目名称）
                            labels = [
                                "设备名称",
                                "版本",
                                "作者",
                                "基频",
                                "电流环频率",
                                "速度环频率",
                                "位置环频率",
                                "最大电流",
                                "输入电压",
                                "最大温度",
                            ]
                            units = ["", "", "", "Hz", "Hz", "Hz", "Hz", "A", "V", "°C"]
                            # 构建带标签的多行字符串
                            formatted_lines = []
                            for i, label in enumerate(labels):
                                value = parts[i].strip() if i < len(parts) else ""
                                formatted_lines.append(f"{label}: {value}{units[i]}")

                            # 用换行符连接所有行
                            self.mw.system_message = "\n".join(formatted_lines)

                            # todo:这里蓝牙握手要改
                            self.mw.comport.confirm_device_handshake()
                        return
                    case Cidx.LOG_GET:  # 日志读取返回
                        self.mw.log.add_log(data)
                    case Cidx.LOG_ERASE:
                        if data[0] == Fidx.SUCCESS:
                            send_titled_message(
                                MSG_TYPE_SUCCESS, "提示", "日志已清除", True, 1000
                            )
                        else:
                            send_titled_message(MSG_TYPE_ERROR, "错误", "日志清除失败")
                        return
                    case Cidx.PARAM_ERASE:  # 参数读取返回
                        if data[0] == Fidx.SUCCESS:
                            send_titled_message(
                                MSG_TYPE_SUCCESS, "提示", "参数已清除", True, 1000
                            )
                        else:
                            send_titled_message(MSG_TYPE_ERROR, "错误", "参数清除失败")
                        return
                    case Cidx.PARAM_SAVE:  # 参数保存返回
                        if data[0] == Fidx.SUCCESS:
                            send_titled_message(
                                MSG_TYPE_SUCCESS, "提示", "参数已保存", True, 1000
                            )
                        else:
                            send_titled_message(MSG_TYPE_ERROR, "错误", "参数保存失败")
                        return
                    case Cidx.PARAM_READ:
                        idx = data[0]
                        self.mw.param_manager.add_param(idx, data[1:])
                    case Cidx.CMD_STREAM_SET:  # 监控值返回
                        byte_len = int(len(data) / 4)
                        for i in range(byte_len):
                            self.mw.wave.add_data_by_index(
                                i, struct.unpack("<f", data[i * 4 : (i + 1) * 4])[0]
                            )
                        return

            except Exception as e:
                print(f"数据处理异常: {e}")
