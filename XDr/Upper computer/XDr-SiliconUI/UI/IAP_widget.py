
from siui.components.page.child_page import SiChildPage
from PyQt5.QtWidgets import QVBoxLayout, QHBoxLayout
from PyQt5.QtCore import Qt

from siui.components.widgets import SiPushButton,SiWidget
from siui.core import SiGlobal
from siui.gui.icons.parser import GlobalIconPack
from siui.components.editbox import SiCapsuleLineEdit
from siui.components.button import (
    SiCapsuleButton,
    SiLongPressButtonRefactor,
    SiPushButtonRefactor
)
from siui.components.progress_bar_ import SiProgressBarRefactor

class DownloadPage(SiChildPage):
    def __init__(self, mainwindow):
        super().__init__(mainwindow) 
        
        self.mw=mainwindow

        # 设置页面尺寸比例（可选）
        self.setSizeRatio(0.6, 0.5)  # 宽度占窗口70%，高度50%

        # 设置标题
        self.content().setTitle("固件升级")

        #创建主垂直布局
        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(100, 80, 100, 160)
        main_layout.setSpacing(30)      

        self.current_version = SiCapsuleLineEdit()
        self.current_version.setReadOnly(True)
        self.current_version.resize(240,40)
        self.current_version.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.current_version.setTitleFixedWidth(80) 
        self.current_version.setAlignment(Qt.AlignCenter) 
        self.current_version.setTitle("当前固件：")
        main_layout.addWidget(self.current_version)

        file_select_layout = QHBoxLayout()
        file_select_layout.setContentsMargins(0, 0, 0, 0)
        file_select_layout.setSpacing(30)

        self.new_bootloader = SiCapsuleLineEdit()
        self.new_bootloader.setReadOnly(True)
        self.new_bootloader.resize(240,40)
        self.new_bootloader.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.new_bootloader.setTitleFixedWidth(80) 
        self.new_bootloader.setAlignment(Qt.AlignCenter) 
        self.new_bootloader.setTitle("选择固件：")

        self.file_select_button = SiPushButtonRefactor()
        self.file_select_button.setSvgIcon(self.mw.icon.get("ic_fluent_window_filled","#DFDFDF"))
        self.file_select_button.setToolTip("浏览")
        self.file_select_button.adjustSize()       

        file_select_layout.addWidget(self.new_bootloader,10)
        file_select_layout.addWidget(self.file_select_button,1)

        main_layout.addLayout(file_select_layout)

        button_layout = QHBoxLayout()
        button_layout.setContentsMargins(0, 0, 0, 0)
        button_layout.setSpacing(0)
        button_layout.setAlignment(Qt.AlignCenter)

        self.download_button = SiLongPressButtonRefactor()
        self.download_button.setText("开始下载")
        self.download_button.setToolTip("长按以确定")
        self.download_button.adjustSize()
        self.download_button.setFixedWidth(100)
        button_layout.addWidget(self.download_button,1)

        self.download_status=SiCapsuleLineEdit()
        self.download_status.setReadOnly(True)
        self.download_status.resize(180,40)
        self.download_status.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.download_status.setTitleFixedWidth(40) 
        self.download_status.setAlignment(Qt.AlignCenter) 
        self.download_status.setTitle("进程：")
        self.download_status.setText("空闲")
        button_layout.addWidget(self.download_status,3)

        main_layout.addLayout(button_layout)


        self.progess_bar=SiProgressBarRefactor()
        self.progess_bar.setMaximum(100)
        self.progess_bar.setMinimum(0)
        self.progess_bar.setValue(0)

        main_layout.addWidget(self.progess_bar)
