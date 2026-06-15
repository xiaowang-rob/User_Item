"""IAP 固件升级 —— 烧录/升级/强制升级"""

import struct
import os
import time
import logging
from queue import Queue, Full, Empty
from PyQt5.QtCore import QThread, pyqtSignal, QObject
from PyQt5.QtWidgets import QFileDialog
from functions.message_show import (
    send_simple_message,
    send_titled_message,
    MSG_TYPE_ERROR,
    MSG_TYPE_SUCCESS,
    MSG_TYPE_WARNING,
)
from protocol import Cidx, Fidx

# ---------- 日志配置 ----------
logger = logging.getLogger("IAP")
logger.setLevel(logging.DEBUG)
if not logger.handlers:
    _handler = logging.StreamHandler()
    _handler.setFormatter(logging.Formatter("[%(levelname)s] %(name)s: %(message)s"))
    logger.addHandler(_handler)

# ========== 协议常量 ==========
WRITE_BLOCK_SIZE = 44      # 单次写入最大数据字节
FLASH_BASE_ADDR = 0x08004000
RETRY_COUNT = 5
WAIT_TIME_S = 0.8
RESP_TIMEOUT = 2.0


class IAPWorker(QThread):
    """IAP 下载工作线程（后台烧录）"""

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

    # ── 响应等待 ──

    def _wait_response(self, cmd_id, timeout=RESP_TIMEOUT):
        """等待指定命令的成功响应（检查内容）"""
        start = time.time()
        while time.time() - start < timeout:
            if self._stop_flag:
                return False
            try:
                cmd, data = self._response_queue.get(timeout=0.05)
                if cmd == cmd_id and len(data) >= 1:
                    self._last_response[cmd_id] = data
                    return data[0] == Fidx.RESP_SUCCESS
            except Empty:
                continue
        return False

    def _wait_response_raw(self, cmd_id, timeout=RESP_TIMEOUT):
        """等待响应（仅确认到达，不检查内容）"""
        start = time.time()
        while time.time() - start < timeout:
            if self._stop_flag:
                return False
            try:
                cmd, data = self._response_queue.get(timeout=0.05)
                if cmd == cmd_id:
                    self._last_response[cmd_id] = data
                    return True
            except Empty:
                continue
        return False

    def _get_response_data(self, cmd_id):
        """获取指定命令的最后一次响应数据"""
        return self._last_response.get(cmd_id, None)

    def stop(self):
        """外部请求停止下载"""
        self._stop_flag = True

    # ── 命令发送（带重试）──

    def _send_command(self, cmd_id, data=bytes(), retries=RETRY_COUNT):
        """发送命令并等待响应，失败重试"""
        for attempt in range(retries):
            if self._stop_flag:
                return False
            # 清空旧队列，防误判
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

        logger.error("命令 %s 发送失败（已重试 %d 次）", cmd_id, retries)
        return False

    def _wait_for_reconnect(self, timeout=10.0):
        """等待 ComPort 自动重连（APP→Bootloader 切换后）"""
        from PyQt5.QtWidgets import QApplication

        start = time.time()
        self.status_updated.emit("等待设备重启并自动重连...")
        QApplication.processEvents()
        time.sleep(0.5)

        while time.time() - start < timeout:
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

    # ── 烧录主流程 ──

    def run(self):
        """IAP 烧录主流程"""
        try:
            logger.info("=== 开始升级，文件: %s ===", os.path.basename(self.file_path))

            # ════ 1. 读取固件 ════
            self.status_updated.emit("📄 读取固件...")
            try:
                with open(self.file_path, "rb") as f:
                    firmware_data = f.read()
            except Exception as e:
                self.finished.emit(False, f"读取文件失败：{str(e)}")
                return

            if not firmware_data:
                self.finished.emit(False, "固件文件为空")
                return

            file_size = len(firmware_data)
            logger.info("固件大小: %d 字节", file_size)

            # ════ 2. 进入 IAP 模式 ════
            if self.bl_mode:
                # ├─ BL 直连模式 ─
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
                # ├─ APP 模式（需复位进 BL）─
                self.progress_updated.emit(5, "准备进入 IAP 模式...")
                self.status_updated.emit("🔄 请求进入升级模式...")

                if not self._send_command(Cidx.CMD_IAP_ENTER):
                    self.finished.emit(False, "❌ 进入 IAP 模式失败")
                    return

                resp_data = self._get_response_data(Cidx.CMD_IAP_ENTER)
                if resp_data and len(resp_data) >= 1:
                    if resp_data[0] == Fidx.RESP_SUCCESS:
                        self.status_updated.emit("✓ 升级标志设置成功，设备将重启")
                        self.comport.send_packet(Cidx.CMD_SYSTEM_RESET, bytes())
                    elif resp_data[0] == Fidx.RESP_FAIL:
                        self.finished.emit(False, "❌ 升级标志设置失败")
                        return

                self.comport.reset_auto_connect()
                self.progress_updated.emit(10, "等待设备重启...")

                # 清空队列，防残留数据干扰
                while not self._response_queue.empty():
                    try:
                        self._response_queue.get_nowait()
                    except Empty:
                        break

                if not self._wait_for_reconnect():
                    self.finished.emit(False, "❌ 重连 Bootloader 失败")
                    return

                logger.info("设备重启成功")
                self.status_updated.emit("🔍 确认 Bootloader 就绪...")
                fw_size_payload = struct.pack("<I", file_size)
                if not self._send_command(Cidx.CMD_IAP_ENTER, fw_size_payload):
                    self.finished.emit(False, "❌ Bootloader 无响应")
                    return

                bl_resp = self._get_response_data(Cidx.CMD_IAP_ENTER)
                if bl_resp and len(bl_resp) >= 1 and bl_resp[0] == Fidx.RESP_SUCCESS:
                    self.status_updated.emit("✓ Bootloader 已就绪")
                else:
                    self.finished.emit(False, "❌ Bootloader 未就绪")
                    return
                self.progress_updated.emit(15, "准备擦除 Flash...")

            # ════ 3. 擦除 Flash ════
            self.status_updated.emit("正在擦除 Flash...")
            if not self._send_command(Cidx.CMD_IAP_ERASE):
                self.finished.emit(False, "❌ Flash 擦除失败")
                return

            # ════ 4. 逐块写入 ════
            self.status_updated.emit("📤 正在写入固件...")
            written = 0
            offset = 0

            while offset < file_size:
                if self._stop_flag:
                    self.finished.emit(False, "⚠ 下载已取消")
                    return

                chunk = min(WRITE_BLOCK_SIZE, file_size - offset)
                data = firmware_data[offset: offset + chunk]
                addr = FLASH_BASE_ADDR + offset
                payload = struct.pack("<I", addr) + data

                if not self._send_command(Cidx.CMD_IAP_WRITE, payload):
                    self.finished.emit(False, f"❌ 写入失败 @ 0x{addr:08X}")
                    return

                written += chunk
                offset += chunk
                progress = 15 + int(70 * written / file_size)
                self.progress_updated.emit(progress, f"已写入 {written}/{file_size} B")

            # ════ 5. 退出 IAP ════
            self.progress_updated.emit(90, "🔄 完成写入，重启设备...")
            self._send_command(Cidx.CMD_IAP_EXIT)  # 不需严格等待响应
            time.sleep(0.5)

            self.progress_updated.emit(100, "✨ 升级完成！")
            self.finished.emit(True, "固件升级成功！")
            logger.info("=== 升级成功 ===")

        except Exception as e:
            logger.exception("升级异常")
            self.finished.emit(False, f"❌ 异常：{str(e)}")


class IAP_downloader(QObject):
    """IAP 下载 UI 控制器 —— 文件选择 / 版本校验 / 启动烧录"""

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

    # ── 版本 / 文件 ──

    def set_current_version(self, version: str):
        self.current_version_show.setText(version)

    def handle_download_but_clicked(self):
        """顶部下载按钮 → 切换页面"""
        self.mw.showChildPage()

    @staticmethod
    def _clean_version_string(text: str) -> str:
        """只保留 ASCII 可打印字符"""
        return "".join(c for c in (text or "") if 32 <= ord(c) <= 126).strip()

    def _validate_firmware_compatibility(self, cur_ver, file_path):
        """验证固件型号兼容性（前缀匹配）"""
        old = self._clean_version_string(cur_ver)
        if not old:
            return False, "", ""

        new = os.path.basename(file_path)
        if new.lower().endswith(".bin"):
            new = new[:-4]
        new = self._clean_version_string(new)

        old_pre = old.rsplit("_", 1)[0] if "_" in old else old
        new_pre = new.rsplit("_", 1)[0] if "_" in new else new
        return old_pre == new_pre, old_pre, new_pre

    def _handle_file_select(self):
        """选择固件 .bin 文件"""
        path, _ = QFileDialog.getOpenFileName(
            self.mw, "选择固件文件", "", "Binary Files (*.bin);;All Files (*)"
        )
        if path:
            self._selected_file = path
            size = os.path.getsize(path)
            self.new_bootloader_input.setText(f"{os.path.basename(path)} ({size / 1024:.1f}KB)")
            send_simple_message(MSG_TYPE_SUCCESS, f"✓ 已选择：{os.path.basename(path)}", True, 1500)
            logger.info("已选择固件: %s (%d KB)", path, size // 1024)

    # ── 启动升级 ──

    def _handle_force_download(self):
        """单击 → 累计强制计数（三击后长按可强制烧录）"""
        self.force_download_tic += 1
        logger.debug("强制下载计数: %d", self.force_download_tic)

    def _handle_start_download(self):
        """长按 → 开始升级（含兼容性检查）"""
        if not self._selected_file:
            send_simple_message(MSG_TYPE_WARNING, "请先选择固件文件", True, 2000)
            return
        if not self.comport.is_connected:
            send_simple_message(MSG_TYPE_WARNING, "请先连接设备", True, 2000)
            return

        cur_ver = self.current_version_show.text()
        compat, old_pre, new_pre = self._validate_firmware_compatibility(cur_ver, self._selected_file)

        if self.force_download_tic < 2:
            self.force_download_tic = 0
            if not compat:
                msg = ("⚠ 无法获取当前设备版本信息\n请确认设备型号是否与固件匹配"
                       if not old_pre else
                       f"⚠ 该固件与设备不适配\n\n期望前缀：{old_pre}\n文件前缀：{new_pre}\n\n请确认固件型号是否正确")
                send_simple_message(MSG_TYPE_WARNING, msg, True, 4000)
                return
        self.force_download_tic = 0

        send_simple_message(MSG_TYPE_WARNING,
                            f"接下来会烧录以下固件\n📄 文件：{os.path.basename(self._selected_file)}\n\n"
                            "⚠ 升级过程中请勿断开 USB 或重启设备！", True, 2000)

        self._set_ui_enabled(False)
        self.download_progress.setValue(0)
        self.download_status.setText("初始化...")

        self._iap_worker = IAPWorker(self.comport, self._selected_file, bl_mode=self.bl_mode_ready)
        self._iap_worker.progress_updated.connect(self._on_progress_update)
        self._iap_worker.status_updated.connect(self._on_status_update)
        self._iap_worker.finished.connect(self._on_download_finished)
        self._iap_worker.start()
        self.bl_mode_ready = False
        logger.info("启动烧录线程: bl_mode=%s", self.bl_mode_ready)

    def _iap_cmd_received(self, cmd_id: int, data: bytes):
        """串口数据包回调 —— 送入 IAP 响应队列"""
        try:
            logger.debug("IAP 收到命令 %s, 数据 %s", cmd_id, data.hex()[:40])

            if cmd_id == Cidx.CMD_BL_CONNECT and data:
                info = data.rstrip(b"\x00\xff").decode("utf-8", errors="ignore").strip()
                if info:
                    logger.info("Bootloader 版本: %s", info)
                    self.set_current_version(info)
                    self.bl_mode_ready = True
                return

            if self._iap_worker and self._iap_worker.isRunning():
                try:
                    self._iap_worker._response_queue.put((cmd_id, data), block=False)
                except Full:
                    logger.warning("IAP 响应队列已满，丢弃命令 %s", cmd_id)
        except Exception as e:
            logger.exception("_iap_cmd_received 异常")

    # ── UI 回调 ──

    def _on_progress_update(self, value: int, text: str):
        self.download_progress.setValue(value)
        self.download_status.setText(text)

    def _on_status_update(self, text: str):
        self.download_status.setText(text)

    def _on_download_finished(self, success: bool, message: str):
        """烧录完成回调"""
        if success:
            send_titled_message(MSG_TYPE_SUCCESS, "🎉 升级成功", message, True, 4000)
            logger.info("烧录成功: %s", message)
        else:
            self.download_progress.setValue(0)
            self.download_status.setText(f"❌ 失败：{message}")
            send_titled_message(MSG_TYPE_ERROR, "❌ 升级失败", message, True, 6000)
            logger.error("烧录失败: %s", message)

        self._set_ui_enabled(True)
        self._iap_worker = None

    def _set_ui_enabled(self, enabled: bool):
        """切换 UI 控件可用状态"""
        self.file_select_button.setEnabled(enabled)
        self.download_start_button.setEnabled(enabled)
        self.download_start_button.setText("开始升级" if enabled else "升级中...")
        self.new_bootloader_input.setEnabled(enabled)
