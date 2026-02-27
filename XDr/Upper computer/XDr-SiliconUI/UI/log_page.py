from PyQt5.QtWidgets import QWidget, QHBoxLayout, QVBoxLayout,QListWidget
from PyQt5.QtCore import Qt
from siui.components.button import (
    SiLongPressButtonRefactor,
    SiPushButtonRefactor,
)
from siui.components.editbox import SiCapsuleLineEdit


title_W = 100
all_W = 300
class LogPage():
    def __init__(self, main_window):
        self.widget =main_window.ui.log_page


        main_layout=QHBoxLayout(self.widget)
        main_layout.setContentsMargins(68,12,68,0)
        main_layout.setSpacing(128)


        self.scroll_layout = QVBoxLayout()
        self.scroll_layout.setContentsMargins(12,68,12,0)
        self.scroll_layout.setSpacing(36)

        self.scroll_H_layout = QHBoxLayout()
        self.scroll_H_layout.setContentsMargins(0,0,0,0)
        self.scroll_H_layout.setSpacing(12)

        self.read_log_btn = SiPushButtonRefactor()
        self.read_log_btn.setText("读取日志")
        self.read_log_btn.adjustSize()
        self.read_log_btn.setFixedHeight(30)
        self.scroll_H_layout.addWidget(self.read_log_btn)

        self.clear_log_btn = SiLongPressButtonRefactor()
        self.clear_log_btn.setText("清空日志")
        self.clear_log_btn.setToolTip("长按清空日志")
        self.clear_log_btn.adjustSize()
        self.clear_log_btn.setFixedHeight(30)
        self.scroll_H_layout.addWidget(self.clear_log_btn)

        
        self.log_list = QListWidget()
        self.log_list.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;  
        """)
        
        self.scroll_layout.addLayout(self.scroll_H_layout)
        self.scroll_layout.addWidget(self.log_list)

        self.log_show_area1 = QWidget()
        self.log_show_area1.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;  
        """)

        self.log_show_area2 = QWidget()
        self.log_show_area2.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;  
        """)

        main_layout.addLayout(self.scroll_layout,1)
        main_layout.addWidget(self.log_show_area1,2)
        main_layout.addWidget(self.log_show_area2,2)

        self.num = SiCapsuleLineEdit()
        self.num.setReadOnly(True)
        self.num.resize(all_W, 40)
        self.num.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.num.setTitleFixedWidth(title_W)
        self.num.setAlignment(Qt.AlignCenter)
        self.num.setTitle("序号")

        self.time = SiCapsuleLineEdit()
        self.time.setReadOnly(True)
        self.time.resize(all_W, 40)
        self.time.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.time.setTitleFixedWidth(title_W)
        self.time.setAlignment(Qt.AlignCenter)
        self.time.setTitle("发生时间/分")

        self.fault = SiCapsuleLineEdit()
        self.fault.setReadOnly(True)
        self.fault.resize(all_W, 40)
        self.fault.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.fault.setTitleFixedWidth(title_W)
        self.fault.setAlignment(Qt.AlignCenter)
        self.fault.setTitle("错误")

        self.warning = SiCapsuleLineEdit()
        self.warning.setReadOnly(True)
        self.warning.resize(all_W, 40)
        self.warning.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.warning.setTitleFixedWidth(title_W)
        self.warning.setAlignment(Qt.AlignCenter)
        self.warning.setTitle("警告")

        self.sensor_mode = SiCapsuleLineEdit()
        self.sensor_mode.setReadOnly(True)
        self.sensor_mode.resize(all_W, 40)
        self.sensor_mode.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.sensor_mode.setTitleFixedWidth(title_W)
        self.sensor_mode.setAlignment(Qt.AlignCenter)
        self.sensor_mode.setTitle("感应模式")

        self.run_mode = SiCapsuleLineEdit()
        self.run_mode.setReadOnly(True)
        self.run_mode.resize(all_W, 40)
        self.run_mode.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.run_mode.setTitleFixedWidth(title_W)
        self.run_mode.setAlignment(Qt.AlignCenter)
        self.run_mode.setTitle("运行模式")

        self.usb_status = SiCapsuleLineEdit()
        self.usb_status.setReadOnly(True)
        self.usb_status.resize(all_W, 40)
        self.usb_status.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.usb_status.setTitleFixedWidth(title_W)
        self.usb_status.setAlignment(Qt.AlignCenter)
        self.usb_status.setTitle("USB状态")

        self.can_status = SiCapsuleLineEdit()
        self.can_status.setReadOnly(True)
        self.can_status.resize(all_W, 40)
        self.can_status.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.can_status.setTitleFixedWidth(title_W)
        self.can_status.setAlignment(Qt.AlignCenter)
        self.can_status.setTitle("CAN状态")

        self.flash_status = SiCapsuleLineEdit()
        self.flash_status.setReadOnly(True)
        self.flash_status.resize(all_W, 40)
        self.flash_status.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.flash_status.setTitleFixedWidth(title_W)
        self.flash_status.setAlignment(Qt.AlignCenter)
        self.flash_status.setTitle("闪存状态")

        self.encode_status = SiCapsuleLineEdit()
        self.encode_status.setReadOnly(True)
        self.encode_status.resize(all_W, 40)
        self.encode_status.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.encode_status.setTitleFixedWidth(title_W)
        self.encode_status.setAlignment(Qt.AlignCenter)
        self.encode_status.setTitle("编码状态")

        self.voltage = SiCapsuleLineEdit()
        self.voltage.setReadOnly(True)
        self.voltage.resize(all_W, 40)
        self.voltage.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.voltage.setTitleFixedWidth(title_W)
        self.voltage.setAlignment(Qt.AlignCenter)
        self.voltage.setTitle("电压")

        self.temperature = SiCapsuleLineEdit()
        self.temperature.setReadOnly(True)
        self.temperature.resize(all_W, 40)
        self.temperature.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.temperature.setTitleFixedWidth(title_W)
        self.temperature.setAlignment(Qt.AlignCenter)
        self.temperature.setTitle("温度")

        self.iu = SiCapsuleLineEdit()
        self.iu.setReadOnly(True)
        self.iu.resize(all_W, 40)
        self.iu.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.iu.setTitleFixedWidth(title_W)
        self.iu.setAlignment(Qt.AlignCenter)
        self.iu.setTitle("相电流 U")

        self.iv = SiCapsuleLineEdit()
        self.iv.setReadOnly(True)
        self.iv.resize(all_W, 40)
        self.iv.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.iv.setTitleFixedWidth(title_W)
        self.iv.setAlignment(Qt.AlignCenter)
        self.iv.setTitle("相电流 V")

        self.iw = SiCapsuleLineEdit()
        self.iw.setReadOnly(True)
        self.iw.resize(all_W, 40)
        self.iw.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.iw.setTitleFixedWidth(title_W)
        self.iw.setAlignment(Qt.AlignCenter)
        self.iw.setTitle("相电流 W")

        self.id = SiCapsuleLineEdit()
        self.id.setReadOnly(True)
        self.id.resize(all_W, 40)
        self.id.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.id.setTitleFixedWidth(title_W)
        self.id.setAlignment(Qt.AlignCenter)
        self.id.setTitle("d轴电流")

        self.id_ref = SiCapsuleLineEdit()
        self.id_ref.setReadOnly(True)
        self.id_ref.resize(all_W, 40)
        self.id_ref.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.id_ref.setTitleFixedWidth(title_W)
        self.id_ref.setAlignment(Qt.AlignCenter)
        self.id_ref.setTitle("d轴电流目标值")

        self.iq = SiCapsuleLineEdit()
        self.iq.setReadOnly(True)
        self.iq.resize(all_W, 40)
        self.iq.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.iq.setTitleFixedWidth(title_W)
        self.iq.setAlignment(Qt.AlignCenter)
        self.iq.setTitle("q轴电流")

        self.iq_ref = SiCapsuleLineEdit()
        self.iq_ref.setReadOnly(True)
        self.iq_ref.resize(all_W, 40)
        self.iq_ref.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.iq_ref.setTitleFixedWidth(title_W)
        self.iq_ref.setAlignment(Qt.AlignCenter)
        self.iq_ref.setTitle("q轴电流目标值")

        self.speed = SiCapsuleLineEdit()
        self.speed.setReadOnly(True)
        self.speed.resize(all_W, 40)
        self.speed.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.speed.setTitleFixedWidth(title_W)
        self.speed.setAlignment(Qt.AlignCenter)
        self.speed.setTitle("速度/RPM")

        self.target_speed = SiCapsuleLineEdit()
        self.target_speed.setReadOnly(True)
        self.target_speed.resize(all_W, 40)
        self.target_speed.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.target_speed.setTitleFixedWidth(title_W)
        self.target_speed.setAlignment(Qt.AlignCenter)
        self.target_speed.setTitle("目标速度/RPM")

        self.position = SiCapsuleLineEdit()
        self.position.setReadOnly(True)
        self.position.resize(all_W, 40)
        self.position.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.position.setTitleFixedWidth(title_W)
        self.position.setAlignment(Qt.AlignCenter)
        self.position.setTitle("位置/°")

        self.target_position = SiCapsuleLineEdit()
        self.target_position.setReadOnly(True)
        self.target_position.resize(all_W, 40)
        self.target_position.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.target_position.setTitleFixedWidth(title_W)
        self.target_position.setAlignment(Qt.AlignCenter)
        self.target_position.setTitle("目标位置/°")

        log_show_layout1 = QVBoxLayout(self.log_show_area1)
        log_show_layout1.setContentsMargins(36,36,36,36)
        log_show_layout1.setSpacing(24)
        log_show_layout1.setAlignment(Qt.AlignTop)

        log_show_layout2 = QVBoxLayout(self.log_show_area2)
        log_show_layout2.setContentsMargins(36,36,36,36)
        log_show_layout2.setSpacing(24)
        log_show_layout2.setAlignment(Qt.AlignTop)

        log_show_layout1.addWidget(self.num)
        log_show_layout1.addWidget(self.time)
        log_show_layout1.addWidget(self.fault)
        log_show_layout1.addWidget(self.warning)
        log_show_layout1.addWidget(self.sensor_mode)
        log_show_layout1.addWidget(self.run_mode)
        log_show_layout1.addWidget(self.usb_status)
        log_show_layout1.addWidget(self.can_status)
        log_show_layout1.addWidget(self.flash_status)
        log_show_layout1.addWidget(self.encode_status)

        log_show_layout2.addWidget(self.voltage)
        log_show_layout2.addWidget(self.temperature)
        log_show_layout2.addWidget(self.iu)
        log_show_layout2.addWidget(self.iv)
        log_show_layout2.addWidget(self.iw)
        log_show_layout2.addWidget(self.id)
        log_show_layout2.addWidget(self.id_ref)
        log_show_layout2.addWidget(self.iq)
        log_show_layout2.addWidget(self.iq_ref)
        log_show_layout2.addWidget(self.speed)
        log_show_layout2.addWidget(self.target_speed)
        log_show_layout2.addWidget(self.position)
        log_show_layout2.addWidget(self.target_position)

