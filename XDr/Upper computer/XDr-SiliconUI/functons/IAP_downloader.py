from siui.core import SiGlobal
import serial
import serial.tools.list_ports
import os
import struct
import time
from PyQt5.QtCore import QTimer, QObject, pyqtSignal, QMutex, QThread
from PyQt5.QtWidgets import QApplication, QFileDialog, QMessageBox
from functons.message_show import (
    send_simple_message,
    send_titled_message,
    MSG_TYPE_ERROR,
    MSG_TYPE_SUCCESS,
    MSG_TYPE_WARNING,
    MSG_TYPE_INFO,
)
from UI.data_ui_map import Cidx
from queue import Queue


# ========== 协议常量 ==========
RESP_FAIL = 0
RESP_SUCCESS = 1
WRITE_BLOCK_SIZE = 251      # 单次写入最大数据字节 (255包长-4地址)
FLASH_BASE_ADDR = 0x08004000  # 根据Bootloader实际配置修改
RETRY_COUNT = 3
RESP_TIMEOUT = 2.0          # 响应超时(秒)
BL_PID = '5740'             # Bootloader设备PID


class IAPWorker(QThread):
    """IAP下载工作线程 - 处理耗时操作，避免阻塞UI"""
    progress_updated = pyqtSignal(int, str)  # (进度%, 状态文本)
    status_updated = pyqtSignal(str)          # 状态消息
    finished = pyqtSignal(bool, str)          # (成功/失败, 消息)
    
    def __init__(self, comport, file_path, parent=None):
        super().__init__(parent)
        self.comport = comport
        self.file_path = file_path
        self._stop_flag = False
        self._response_queue = Queue()
        
    def stop(self):
        """外部调用停止下载"""
        self._stop_flag = True
        
    def _wait_response(self, cmd_id, timeout=RESP_TIMEOUT):
        """等待指定命令的响应，返回bool表示成功/失败"""
        start_time = time.time()
        while time.time() - start_time < timeout:
            if self._stop_flag:
                return False
            if not self._response_queue.empty():
                resp_cmd, resp_data = self._response_queue.get()
                if resp_cmd == cmd_id and len(resp_data) >= 1:
                    return resp_data[0] == RESP_SUCCESS
            time.sleep(0.01)
        return False
    
    def _send_command(self, cmd_id, data=bytes(), retries=RETRY_COUNT):
        """发送命令并等待响应，带重试机制"""
        for attempt in range(retries):
            if self._stop_flag:
                return False
            # 清空旧响应
            while not self._response_queue.empty():
                self._response_queue.get()
            # 发送
            if not self.comport.send_packet(cmd_id, data):
                continue
            # 等待响应
            if self._wait_response(cmd_id):
                return True
            time.sleep(0.1 * (attempt + 1))  # 指数退避
        return False
    
    def _reconnect_to_bl(self, original_port_name):
        """检测设备 PID 切换并重新连接到 Bootloader"""
        self.status_updated.emit("检测设备进入 Bootloader 模式...")
        
        # 先彻底断开旧连接，释放 OS 句柄，防止"端口被占用"
        # 使用 silent=True 避免触发主界面的"连接断开"错误弹窗
        if self.comport.is_connected:
            self.comport.disconnect(silent=True)
        
        # 等待 USB 物理复位和枚举 (Windows 通常需要 1~2 秒)
        # 这段时间内旧 COM 口会消失，新 COM 口会出现
        # 改为循环检测，而不是固定 sleep
        start_time = time.time()
        timeout = 5.0 
        while time.time() - start_time < timeout:
            if self._stop_flag: return False
            
            self.status_updated.emit("扫描 Bootloader 设备...")
            if self.comport.connect_by_pid(target_pid=BL_PID, timeout=0.5):
                self.status_updated.emit(f"✓ 已重连 Bootloader")
                return True
            
            time.sleep(0.3) # 每次间隔 300ms
            
        self.status_updated.emit("⚠ 未检测到 Bootloader")
        return False
    
    def run(self):
        """下载主流程"""
        try:
            # ===== 1. 读取固件文件 =====
            self.status_updated.emit(f"📄 读取: {os.path.basename(self.file_path)}")
            with open(self.file_path, 'rb') as f:
                firmware_data = f.read()
            if not firmware_data:
                self.finished.emit(False, "固件文件为空")
                return
            file_size = len(firmware_data)
            self.progress_updated.emit(5, "准备进入IAP模式...")
            
            # ===== 2. 发送进入IAP命令（APP模式下） =====
            self.status_updated.emit("🔄 请求进入升级模式...")
            firmware_size_payload = struct.pack('<I', file_size)
            if not self._send_command(Cidx.CMD_IAP_ENTER,firmware_size_payload):
                self.finished.emit(False, "❌ 进入IAP模式失败\n请确认: 1.设备支持升级 2.固件版本匹配")
                return
            
            self.progress_updated.emit(10, "等待设备重启...")
            self.status_updated.emit("⏳ 设备重启中，等待Bootloader枚举...")
            time.sleep(1.2)  # 给设备重启+USB枚举时间
            
            # ===== 3. 重连到Bootloader模式 =====
            original_port = self.comport.serial_port.port if self.comport.serial_port else None
            if not self._reconnect_to_bl(original_port):
                self.finished.emit(False, "❌ 重连Bootloader失败")
                return
            
            # ===== 4. 擦除Flash =====
            self.progress_updated.emit(15, "🧹 擦除Flash...")
            self.status_updated.emit("正在擦除Flash，请稍候(约2-5秒)...")
            if not self._send_command(Cidx.CMD_IAP_ERASE):
                self.finished.emit(False, "❌ Flash擦除失败")
                return
            
            # ===== 5. 分块写入固件 =====
            self.status_updated.emit("📤 正在写入固件...")
            written_bytes = 0
            offset = 0
            
            while offset < file_size:
                if self._stop_flag:
                    self.finished.emit(False, "⚠ 下载已取消")
                    return
                # 计算本次写入大小
                chunk_size = min(WRITE_BLOCK_SIZE, file_size - offset)
                chunk_data = firmware_data[offset:offset + chunk_size]
                # 构造payload: 4字节小端地址 + 数据
                addr = FLASH_BASE_ADDR + offset
                write_payload = struct.pack('<I', addr) + chunk_data
                # 发送写入命令
                if not self._send_command(Cidx.CMD_IAP_WRITE, write_payload):
                    self.finished.emit(False, f"❌ 写入失败 @ 0x{addr:08X}")
                    return
                # 更新进度
                written_bytes += chunk_size
                offset += chunk_size
                progress = 15 + int(70 * written_bytes / file_size)  # 15%~85%
                self.progress_updated.emit(progress, f"已写入 {written_bytes}/{file_size} B")
            
            # ===== 6. 校验Flash（可选，根据需求启用） =====
            # self.progress_updated.emit(85, "✅ 校验Flash...")
            # if not self._send_command(Cidx.CMD_IAP_VERIFY, struct.pack('<I', file_size)):
            #     self.finished.emit(False, "❌ 校验失败")
            #     return
            
            # ===== 7. 退出IAP模式，重启设备 =====
            self.progress_updated.emit(90, "🔄 完成写入，重启设备...")
            self.status_updated.emit("发送退出命令，设备将自动重启...")
            self._send_command(Cidx.CMD_IAP_EXIT)  # 不等待响应，设备会立即重启
            
            # ===== 8. 完成 =====
            time.sleep(0.5)
            self.progress_updated.emit(100, "✨ 升级完成！")
            self.status_updated.emit("✅ 固件升级成功，设备已重启")
            self.finished.emit(True, "固件升级成功！")
            
        except FileNotFoundError:
            self.finished.emit(False, "❌ 固件文件未找到")
        except PermissionError:
            self.finished.emit(False, "❌ 无法读取文件，请检查权限")
        except Exception as e:
            self.finished.emit(False, f"❌ 异常: {str(e)}")


class IAP_downloader:
    def __init__(self, mainwindow):
        self.mw = mainwindow
        self.widget = self.mw.download_page
        # UI控件引用
        self.current_version_show = self.widget.current_version
        self.new_bootloader_input = self.widget.new_bootloader
        self.file_select_button = self.widget.file_select_button
        self.download_start_button = self.widget.download_button
        self.download_status = self.widget.download_status
        self.download_progress = self.widget.progess_bar

        self.comport = self.mw.comport
        self.download = self.mw.top_area.download_but
        self.download.clicked.connect(self.handle_download_but_clicked)
        
        # 绑定按钮信号
        self.file_select_button.clicked.connect(self._handle_file_select)
        self.download_start_button.clicked.connect(self._handle_start_download)
        
        
        # 状态管理
        self._iap_worker = None
        self._selected_file = None

    def set_current_version(self, version: str):
        """设置当前固件版本"""
        self.current_version_show.setText(version)

    def handle_download_but_clicked(self):
        """顶部下载菜单按钮点击"""
        self.mw.showChildPage()
    
    def _handle_file_select(self):
        """选择固件文件"""
        file_path, _ = QFileDialog.getOpenFileName(
            self.mw, "选择固件文件", "", "Binary Files (*.bin);;All Files (*)"
        )
        if file_path:
            self._selected_file = file_path
            name = os.path.basename(file_path)
            size = os.path.getsize(file_path)
            self.new_bootloader_input.setText(f"{name} ({size/1024:.1f}KB)")
            send_simple_message(MSG_TYPE_SUCCESS, f"✓ 已选择: {name}", True, 1500)
    
    def _handle_start_download(self):
        """开始下载按钮"""
        if not self._selected_file:
            send_simple_message(MSG_TYPE_WARNING, "请先选择固件文件", True, 2000)
            return
        if not self.comport.is_connected:
            send_simple_message(MSG_TYPE_WARNING, "请先连接设备", True, 2000)
            return
        
        # 确认对话框
        reply = QMessageBox.question(
            self.mw, "确认固件升级",
            f"确定升级固件吗？\n📄 文件: {os.path.basename(self._selected_file)}\n\n⚠ 升级过程中请勿断开USB或重启设备！",
            QMessageBox.Yes | QMessageBox.No, QMessageBox.No
        )
        if reply != QMessageBox.Yes:
            return
        
        # 锁定UI
        self._set_ui_enabled(False)
        self.download_progress.setValue(0)
        self.download_status.setText("初始化...")
        
        # 启动工作线程
        self._iap_worker = IAPWorker(self.comport, self._selected_file)
        self._iap_worker.progress_updated.connect(self._on_progress_update)
        self._iap_worker.status_updated.connect(self._on_status_update)
        self._iap_worker.finished.connect(self._on_download_finished)
        self._iap_worker.start()
    
    def iap_cmd_received(self, cmd_id: int, data: bytes):
        """串口收到有效数据包 - 分发处理"""
        # IAP命令响应 → 转发给工作线程
        if self._iap_worker and self._iap_worker.isRunning():
            self._iap_worker._response_queue.put((cmd_id, data))
 
    
    def _on_progress_update(self, value: int, text: str):
        """更新进度条"""
        self.download_progress.setValue(value)
        self.download_status.setText(text)
    
    def _on_status_update(self, text: str):
        """更新状态文本"""
        self.download_status.setText(text)
        QApplication.processEvents()  # 强制刷新UI
    
    def _on_download_finished(self, success: bool, message: str):
        """下载完成回调"""
        if success:
            send_titled_message(MSG_TYPE_SUCCESS, "🎉 升级成功", message, True, 4000)
            # 可选：自动刷新版本显示
            # self._refresh_version_info()
        else:
            send_titled_message(MSG_TYPE_ERROR, "❌ 升级失败", message, True, 6000)
        self._set_ui_enabled(True)
        self._iap_worker = None
    
    def _set_ui_enabled(self, enabled: bool):
        """批量设置UI控件状态"""
        self.file_select_button.setEnabled(enabled)
        self.download_start_button.setEnabled(enabled)
        self.download_start_button.setText("开始升级" if enabled else "升级中...")
        self.new_bootloader_input.setEnabled(enabled)