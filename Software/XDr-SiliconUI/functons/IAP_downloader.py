import struct
import os
import time
from queue import Queue, Full, Empty
from PyQt5.QtCore import QThread, pyqtSignal, QObject
from PyQt5.QtWidgets import QFileDialog
from functons.message_show import (
    send_simple_message,
    send_titled_message,
    MSG_TYPE_ERROR,
    MSG_TYPE_SUCCESS,
    MSG_TYPE_WARNING,
)
from shared_constants import Cidx,Fidx

# ========== 协议常量 ==========
WRITE_BLOCK_SIZE = 44  # 单次写入最大数据字节
FLASH_BASE_ADDR = 0x08004000
RETRY_COUNT = 5
WAIT_TIME_S = 0.8
RESP_TIMEOUT = 2.0


class IAPWorker(QThread):
    """IAP 下载工作线程"""

    progress_updated = pyqtSignal(int, str)
    status_updated = pyqtSignal(str)
    finished = pyqtSignal(bool, str)

    def __init__(self, comport, file_path, bl_mode=False, parent=None):
        super().__init__(parent)
        self.comport = comport
        self.file_path = file_path
        self.bl_mode = bl_mode
        self._stop_flag = False
        self._response_queue = Queue()
        self._last_response = {}

    def _wait_response(self, cmd_id, timeout=RESP_TIMEOUT):
        """等待指定命令的响应 (检查内容)"""
        start_time = time.time()
        while time.time() - start_time < timeout:
            if self._stop_flag:
                return False
            try:
                resp_cmd, resp_data = self._response_queue.get(timeout=0.05)
                if resp_cmd == cmd_id and len(resp_data) >= 1:
                    self._last_response[cmd_id] = resp_data
                    return resp_data[0] == Fidx.RESP_SUCCESS
            except Empty:
                continue
        return False

    def _wait_response_raw(self, cmd_id, timeout=RESP_TIMEOUT):
        """等待响应 (不检查内容，仅确认到达)"""
        start_time = time.time()
        while time.time() - start_time < timeout:
            if self._stop_flag:
                return False
            try:
                resp_cmd, resp_data = self._response_queue.get(timeout=0.05)
                if resp_cmd == cmd_id:
                    self._last_response[cmd_id] = resp_data
                    return True
            except Empty:
                continue
        return False

    def _get_response_data(self, cmd_id):
        """获取指定命令的最后一次响应数据"""
        return self._last_response.get(cmd_id, None)

    def stop(self):
        """外部调用停止下载"""
        self._stop_flag = True

    def _send_command(self, cmd_id, data=bytes(), retries=RETRY_COUNT):
        """发送命令并等待响应，带重试机制"""
        for attempt in range(retries):
            if self._stop_flag:
                return False
            # 清空旧数据防止误判
            while not self._response_queue.empty():
                try:
                    self._response_queue.get_nowait()
                except Empty:
                    break

            if not self.comport.send_packet(cmd_id, data):
                time.sleep(0.1 * (attempt + 1))
                continue

            if self._wait_response(cmd_id):
                return True
            time.sleep(0.5 * (attempt + 1))
        return False

    def _wait_for_reconnect(self, timeout=10.0):
        """等待 ComPort 自动重连完成"""
        from PyQt5.QtWidgets import QApplication

        start_time = time.time()
        self.status_updated.emit("等待设备重启并自动重连...")

        # 让 UI 线程处理信号
        QApplication.processEvents()
        time.sleep(0.5)

        while time.time() - start_time < timeout:
            if self._stop_flag:
                return False

            QApplication.processEvents()
            time.sleep(0.3)

            if self.comport.is_connected and self.comport._is_bootloader_mode:
                time.sleep(0.5)
                self.status_updated.emit("✓ 设备重连成功")
                return True

        self.status_updated.emit("⚠ 设备重连超时")
        return False

    def run(self):
        """下载主流程"""
        try:
            print(f"[IAP] === 开始升级 === 文件：{os.path.basename(self.file_path)}")

            # ===== 1. 读取固件文件 =====
            self.status_updated.emit(f"📄 读取固件...")
            try:
                with open(self.file_path, "rb") as f:
                    firmware_data = f.read()
            except Exception as e:
                self.finished.emit(False, f"读取文件失败：{str(e)}")
                return

            # 【修复】修复变量名错误 firmware_ -> firmware_data
            if not firmware_data:
                self.finished.emit(False, "固件文件为空")
                return

            file_size = len(firmware_data)
            print(f"[IAP] 固件大小：{file_size} 字节")

            # ✅ 分支判断：BL 直连模式 vs 普通 APP 模式
            if self.bl_mode:
                # ===== BL 直连模式 =====
                self.status_updated.emit("🔒 验证 Bootloader...")

                payload = struct.pack("<I", file_size)
                if not self._send_command(Cidx.CMD_IAP_ENTER, payload):
                    self.finished.emit(False, "❌ 验证失败")
                    return

                bl_resp = self._get_response_data(Cidx.CMD_IAP_ENTER)
                if not (bl_resp and len(bl_resp) >= 1 and bl_resp[0] == Fidx.RESP_SUCCESS):
                    self.finished.emit(False, "❌ Bootloader 未就绪")
                    return
                self.progress_updated.emit(15, "准备擦除 Flash...")

            else:
                # ===== 原有 APP 模式流程 (需复位) =====
                self.progress_updated.emit(5, "准备进入 IAP 模式...")
                self.status_updated.emit("🔄 请求进入升级模式...")

                # 1. 通知 APP 准备复位
                if not self._send_command(Cidx.CMD_IAP_ENTER):
                    self.finished.emit(False, "❌ 进入 IAP 模式失败")
                    return

                resp_data = self._get_response_data(Cidx.CMD_IAP_ENTER)
                if resp_data and len(resp_data) >= 1:
                    if resp_data[0] == Fidx.RESP_SUCCESS:
                        self.status_updated.emit("✓ 升级标志设置成功，设备将重启")
                        # 发送复位命令
                        self.comport.send_packet(Cidx.CMD_SYSTEM_RESET, bytes())
                    elif resp_data[0] == Fidx.RESP_FAIL:
                        self.finished.emit(False, "❌ 升级标志设置失败")
                        return
                # 2. 启用 ComPort 自动重连，并等待重连
                # 此时 ComPort 会检测到物理断开，并自动尝试重连 Bootloader PID
                self.comport.reset_auto_connect()

                self.progress_updated.emit(10, "等待设备重启...")

                # 清空队列，防止复位前的残留数据干扰
                while not self._response_queue.empty():
                    try:
                        self._response_queue.get_nowait()
                    except Empty:
                        break

                if not self._wait_for_reconnect():
                    self.finished.emit(False, "❌ 重连 Bootloader 失败")
                    return
                print(f"[IAP] 设备重启成功")
                # 3. 重连后再次握手
                self.status_updated.emit("🔍 确认 Bootloader 就绪...")
                firmware_size_payload = struct.pack("<I", file_size)
                if not self._send_command(Cidx.CMD_IAP_ENTER, firmware_size_payload):
                    self.finished.emit(False, "❌ Bootloader 无响应")
                    return

                bl_resp = self._get_response_data(Cidx.CMD_IAP_ENTER)
                if bl_resp and len(bl_resp) >= 1 and bl_resp[0] == Fidx.RESP_SUCCESS:
                    self.status_updated.emit("✓ Bootloader 已就绪")
                else:
                    self.finished.emit(False, "❌ Bootloader 未就绪")
                    return

                self.progress_updated.emit(15, "准备擦除 Flash...")

            # ===== 以下流程共用 (擦除→写入→退出) =====
            self.status_updated.emit("正在擦除 Flash...")
            if not self._send_command(Cidx.CMD_IAP_ERASE):
                self.finished.emit(False, "❌ Flash 擦除失败")
                return

            self.status_updated.emit("📤 正在写入固件...")
            written_bytes = 0
            offset = 0

            while offset < file_size:
                if self._stop_flag:
                    self.finished.emit(False, "⚠ 下载已取消")
                    return

                chunk_size = min(WRITE_BLOCK_SIZE, file_size - offset)
                chunk_data = firmware_data[offset : offset + chunk_size]
                addr = FLASH_BASE_ADDR + offset
                # 构造 payload: 4 字节地址 + 数据
                write_payload = struct.pack("<I", addr) + chunk_data

                if not self._send_command(Cidx.CMD_IAP_WRITE, write_payload):
                    self.finished.emit(False, f"❌ 写入失败 @ 0x{addr:08X}")
                    return

                written_bytes += chunk_size
                offset += chunk_size
                progress = 15 + int(70 * written_bytes / file_size)
                self.progress_updated.emit(
                    progress, f"已写入 {written_bytes}/{file_size} B"
                )

            self.progress_updated.emit(90, "🔄 完成写入，重启设备...")
            self._send_command(Cidx.CMD_IAP_EXIT)  # 这里不需要严格等待响应
            time.sleep(0.5)

            self.progress_updated.emit(100, "✨ 升级完成！")
            self.finished.emit(True, "固件升级成功！")
            print(f"[IAP] === 升级成功 ===")

        except Exception as e:
            print(f"[IAP] === 异常：{type(e).__name__}: {str(e)} ===")
            self.finished.emit(False, f"❌ 异常：{str(e)}")


class IAP_downloader(QObject):
    def __init__(self, mainwindow):
        super().__init__()
        self.mw = mainwindow
        self.widget = self.mw.download_page
        self.current_version_show = self.widget.current_version
        self.new_bootloader_input = self.widget.new_bootloader
        self.file_select_button = self.widget.file_select_button
        self.download_start_button = self.widget.download_button
        self.download_status = self.widget.download_status
        self.download_progress = self.widget.progess_bar

        self.comport = self.mw.comport
        self.download = self.mw.top_area.download_but
        self.download.clicked.connect(self.handle_download_but_clicked)

        self.file_select_button.clicked.connect(self._handle_file_select)

        self.download_start_button.longPressed.connect(self._handle_start_download)
        self.download_start_button.clicked.connect(self._handle_force_download)

        self._iap_worker = None
        self._selected_file = None
        self.bl_mode_ready = False
        self.force_download_tic = 0

    def set_current_version(self, version: str):
        """设置当前固件版本"""
        self.current_version_show.setText(version)

    def handle_download_but_clicked(self):
        """顶部下载菜单按钮点击"""
        self.mw.showChildPage()

    def _clean_version_string(self, text: str) -> str:
        """只保留 ASCII 可打印字符"""
        if not text:
            return ""
        return "".join(c for c in text if 32 <= ord(c) <= 126).strip()

    def _validate_firmware_compatibility(self, current_version_str, new_file_path):
        """验证固件兼容性"""
        old_name = self._clean_version_string(current_version_str)
        if not old_name:
            return (
                False,
                "未知设备版本，如果要强制烧录固件，请自行确认硬件版本后先三击<开始升级>按钮，再长按进行强制烧录",
                "",
            )

        new_name = os.path.basename(new_file_path)
        if new_name.lower().endswith(".bin"):
            new_name = new_name[:-4]
        new_name = self._clean_version_string(new_name)

        old_prefix = old_name.rsplit("_", 1)[0] if "_" in old_name else old_name
        new_prefix = new_name.rsplit("_", 1)[0] if "_" in new_name else new_name

        if old_prefix != new_prefix:
            return False, old_prefix, new_prefix

        return True, old_prefix, new_prefix

    def _handle_file_select(self):
        """选择固件文件"""
        file_path, _ = QFileDialog.getOpenFileName(
            self.mw, "选择固件文件", "", "Binary Files (*.bin);;All Files (*)"
        )
        if file_path:
            self._selected_file = file_path
            name = os.path.basename(file_path)
            size = os.path.getsize(file_path)
            self.new_bootloader_input.setText(f"{name} ({size / 1024:.1f}KB)")
            send_simple_message(MSG_TYPE_SUCCESS, f"✓ 已选择：{name}", True, 1500)

    def _handle_force_download(self):
        """强制下载"""
        self.force_download_tic += 1

    def _handle_start_download(self):
        """开始下载按钮"""

        if not self._selected_file:
            send_simple_message(MSG_TYPE_WARNING, "请先选择固件文件", True, 2000)
            return
        if not self.comport.is_connected:
            send_simple_message(MSG_TYPE_WARNING, "请先连接设备", True, 2000)
            return

        current_version_text = self.current_version_show.text()
        is_compat, old_prefix, new_prefix = self._validate_firmware_compatibility(
            current_version_text, self._selected_file
        )

        if self.force_download_tic < 2:
            self.force_download_tic = 0
            if not is_compat:
                if not old_prefix:
                    msg = "⚠ 无法获取当前设备版本信息\n请确认设备型号是否与固件匹配"
                else:
                    msg = f"⚠ 该固件与设备不适配\n\n期望前缀：{old_prefix}\n文件前缀：{new_prefix}\n\n请确认固件型号是否正确"
                send_simple_message(MSG_TYPE_WARNING, msg, True, 4000)
                return
        self.force_download_tic = 0
        msg = f"接下来会烧录以下固件\n📄 文件：{os.path.basename(self._selected_file)}\n\n⚠ 升级过程中请勿断开 USB 或重启设备！"
        send_simple_message(MSG_TYPE_WARNING, msg, True, 2000)

        self._set_ui_enabled(False)
        self.download_progress.setValue(0)
        self.download_status.setText("初始化...")

        self._iap_worker = IAPWorker(
            self.comport, self._selected_file, bl_mode=self.bl_mode_ready
        )

        self._iap_worker.progress_updated.connect(self._on_progress_update)
        self._iap_worker.status_updated.connect(self._on_status_update)
        self._iap_worker.finished.connect(self._on_download_finished)
        self._iap_worker.start()

        self.bl_mode_ready = False

    def _iap_cmd_received(self, cmd_id: int, data: bytes):
        """串口收到有效数据包 - 直接送入 IAP 队列"""
        # 特殊处理：如果是 Bootloader 版本信息，更新 UI
        try:
            print(f"[IAP] 收到命令：{cmd_id}，数据：{data.hex()}")
            if cmd_id == Cidx.CMD_BL_CONNECT and data:
                fw_info = (
                    data.rstrip(b"\x00\xff").decode("utf-8", errors="ignore").strip()
                )
                if fw_info:
                    print(f"[IAP] 收到版本信息：{fw_info}")
                    self.set_current_version(fw_info)
                    self.bl_mode_ready = True
                return

            if self._iap_worker and self._iap_worker.isRunning():
                try:
                    self._iap_worker._response_queue.put((cmd_id, data), block=False)
                except Full:
                    pass
        except Exception as e:
            print(f"[IAP] _iap_cmd_received 异常：{e}")  # ← 添加异常捕获

    def _on_progress_update(self, value: int, text: str):
        """更新进度条"""
        self.download_progress.setValue(value)
        self.download_status.setText(text)

    def _on_status_update(self, text: str):
        """更新状态文本"""
        self.download_status.setText(text)

    def _on_download_finished(self, success: bool, message: str):
        """下载完成回调"""
        # 【移除】不再在这里恢复信号连接，保持您原有的数据流转逻辑

        if success:
            send_titled_message(MSG_TYPE_SUCCESS, "🎉 升级成功", message, True, 4000)
        else:
            self.download_progress.setValue(0)
            self.download_status.setText(f"❌ 失败：{message}")
            send_titled_message(MSG_TYPE_ERROR, "❌ 升级失败", message, True, 6000)

        self._set_ui_enabled(True)
        self._iap_worker = None

    def _set_ui_enabled(self, enabled: bool):
        """批量设置 UI 控件状态"""
        self.file_select_button.setEnabled(enabled)
        self.download_start_button.setEnabled(enabled)
        self.download_start_button.setText("开始升级" if enabled else "升级中...")
        self.new_bootloader_input.setEnabled(enabled)
