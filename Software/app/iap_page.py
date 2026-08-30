from PyQt5.QtWidgets import QVBoxLayout, QHBoxLayout, QWidget, QTextEdit
from PyQt5.QtCore import Qt
from qfluentwidgets import (
    HeaderCardWidget, CardWidget, PushButton, ToolButton, LineEdit, ProgressBar,
    TitleLabel, BodyLabel, StrongBodyLabel, FluentIcon as FIF,
    PrimaryPushButton, ScrollArea
)


class IAPPage(HeaderCardWidget):
    """固件升级页面 —— 左：系统信息 | 右：固件升级"""

    def __init__(self):
        super().__init__()
        self.setObjectName("iap_page")
        self.setTitle("设备信息 · 固件升级")

        # 用 viewLayout 分左右两栏
        left_card = CardWidget()
        left_layout = QVBoxLayout(left_card)
        left_layout.setContentsMargins(16, 16, 16, 16)
        left_layout.setSpacing(12)

        left_title = StrongBodyLabel("系统信息")
        left_title.setStyleSheet("font-size:16px;")
        left_layout.addWidget(left_title)

        self.info_display = QTextEdit()
        self.info_display.setReadOnly(True)
        self.info_display.setPlaceholderText("连接设备后将在此显示系统信息…")
        self.info_display.setStyleSheet("""
            QTextEdit {
                background: transparent;
                border: 1px solid rgba(255,255,255,0.1);
                border-radius: 6px;
                padding: 12px;
                font-size: 13px;
                line-height: 1.6;
            }
        """)
        self.info_display.setMinimumHeight(200)
        left_layout.addWidget(self.info_display)
        left_layout.addStretch()

        # ── 右栏：固件升级 ──
        right_card = CardWidget()
        right_layout = QVBoxLayout(right_card)
        right_layout.setContentsMargins(16, 16, 16, 16)
        right_layout.setSpacing(16)

        right_title = StrongBodyLabel("固件升级")
        right_title.setStyleSheet("font-size:16px;")
        right_layout.addWidget(right_title)

        # 当前固件版本
        self.current_version = LineEdit()
        self.current_version.setReadOnly(True)
        self.current_version.setPlaceholderText("当前固件：")
        self.current_version.setMinimumWidth(240)
        right_layout.addWidget(self.current_version)

        # 文件选择
        file_layout = QHBoxLayout()
        file_layout.setContentsMargins(0, 0, 0, 0)
        file_layout.setSpacing(8)

        self.new_bootloader = LineEdit()
        self.new_bootloader.setReadOnly(True)
        self.new_bootloader.setPlaceholderText("选择固件：")
        self.new_bootloader.setMinimumWidth(200)

        self.file_select_button = ToolButton(FIF.FOLDER)
        self.file_select_button.setToolTip("浏览")

        file_layout.addWidget(self.new_bootloader, 1)
        file_layout.addWidget(self.file_select_button)
        right_layout.addLayout(file_layout)

        # 下载按钮 + 状态
        btn_status_layout = QHBoxLayout()
        btn_status_layout.setContentsMargins(0, 0, 0, 0)
        btn_status_layout.setSpacing(12)

        self.download_button = PrimaryPushButton()
        self.download_button.setText("开始升级")
        self.download_button.setToolTip("开始固件升级")
        self.download_button.setFixedWidth(120)

        self.download_status = LineEdit()
        self.download_status.setReadOnly(True)
        self.download_status.setPlaceholderText("进程：")
        self.download_status.setText("空闲")
        self.download_status.setFixedWidth(160)

        btn_status_layout.addWidget(self.download_button)
        btn_status_layout.addWidget(self.download_status)
        right_layout.addLayout(btn_status_layout)

        # 进度条
        self.progess_bar = ProgressBar()
        self.progess_bar.setRange(0, 100)
        self.progess_bar.setValue(0)
        self.progess_bar.setFixedHeight(8)
        right_layout.addWidget(self.progess_bar)

        right_layout.addStretch()

        # 组装到 viewLayout
        self.viewLayout.addWidget(left_card, 1)
        self.viewLayout.addWidget(right_card, 1)

    def set_system_info(self, info: str):
        """更新系统信息显示"""
        self.info_display.setText(info)
