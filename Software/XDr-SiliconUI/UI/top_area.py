from siui.components.combobox_ import SiCapsuleComboBox
from PyQt5.QtWidgets import QVBoxLayout, QHBoxLayout, QWidget
from PyQt5.QtCore import Qt
from siui.components.button import (
    SiCapsuleButton,
    SiLongPressButtonRefactor,
    SiPushButtonRefactor,
)
from siui.components.editbox import SiCapsuleLineEdit


class TopArea:
    def __init__(self, main_window):
        self.mw = main_window
        self.wideget = main_window.ui.top_area

        main_layout = QHBoxLayout(self.wideget)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(12)

        connect_area = QWidget()
        but_area = QWidget()
        status_area = QWidget()
        config_area = QWidget()

        # 统一四个角的圆角半径
        connect_area.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;  
        """)
        but_area.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;  
        """)
        status_area.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;
        """)
        config_area.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;
        """)

        main_layout.addWidget(connect_area, 3)
        main_layout.addWidget(status_area, 5)
        main_layout.addWidget(but_area, 2)
        main_layout.addWidget(config_area, 2)

        # 创建连接区域按钮
        ###################################################################
        self.download_but = SiPushButtonRefactor()
        self.download_but.setSvgIcon(
            self.mw.icon.get("ic_fluent_arrow_download_filled", "#DFDFDF")
        )
        self.download_but.setToolTip("烧录固件")
        self.download_but.adjustSize()

        self.com_port = SiCapsuleComboBox()
        self.com_port.setTitle("端口")
        self.com_port.setMinimumHeight(36)
        self.com_port.setEditable(False)

        self.system_message = SiPushButtonRefactor()
        self.system_message.setSvgIcon(
            self.mw.icon.get("ic_fluent_memory_regular", "#DFDFDF")
        )
        self.system_message.setToolTip("系统信息")
        self.system_message.adjustSize()

        self.connect_but = SiCapsuleButton()
        self.connect_but.setText("未连接")
        self.connect_but.setValue("连接")
        # 创建主垂直布局
        connect_H_layout = QVBoxLayout(connect_area)
        connect_H_layout.setContentsMargins(6, 6, 6, 6)  # 根据需要调整边距
        connect_H_layout.setSpacing(8)  # 行间距

        # 第一行：端口选择框
        device_W_layout = QHBoxLayout()
        device_W_layout.setContentsMargins(0, 0, 0, 0)  # 根据需要调整边距
        device_W_layout.setSpacing(16)  # 行间距
        device_W_layout.addWidget(self.download_but, 1)
        device_W_layout.addWidget(self.com_port, 4)
        connect_H_layout.addLayout(device_W_layout)

        # 第二行：水平布局包含两个按钮
        connect_W_layout = QHBoxLayout()
        connect_W_layout.setSpacing(16)
        connect_W_layout.addWidget(self.system_message, 1)
        connect_W_layout.addWidget(self.connect_but, 4)

        connect_H_layout.addLayout(connect_W_layout)

        #############################################################################
        # 状态区域显示控件
        title_W = 70
        all_W = 210

        self.sensormode_show = SiCapsuleLineEdit()
        self.sensormode_show.resize(all_W, 40)
        self.sensormode_show.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.sensormode_show.setTitleFixedWidth(title_W)
        self.sensormode_show.setAlignment(Qt.AlignCenter)
        self.sensormode_show.setReadOnly(True)
        self.sensormode_show.setTitle("感应模式")

        self.runmode_show = SiCapsuleLineEdit()
        self.runmode_show.resize(all_W, 40)
        self.runmode_show.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.runmode_show.setTitleFixedWidth(title_W)
        self.runmode_show.setAlignment(Qt.AlignCenter)
        self.runmode_show.setReadOnly(True)
        self.runmode_show.setTitle("运行模式")

        self.state_show = SiCapsuleLineEdit()
        self.state_show.resize(all_W, 40)
        self.state_show.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.state_show.setTitleFixedWidth(title_W)
        self.state_show.setAlignment(Qt.AlignCenter)
        self.state_show.setReadOnly(True)
        self.state_show.setTitle("状态")
        self.state_show.setText("IDLE")

        self.Vbus_show = SiCapsuleLineEdit()
        self.Vbus_show.resize(all_W, 40)
        self.Vbus_show.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.Vbus_show.setTitleFixedWidth(title_W)
        self.Vbus_show.setAlignment(Qt.AlignCenter)
        self.Vbus_show.setReadOnly(True)
        self.Vbus_show.setTitle("电压")
        self.Vbus_show.setText("0.0V")

        self.temp_show = SiCapsuleLineEdit()
        self.temp_show.resize(all_W, 40)
        self.temp_show.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.temp_show.setTitleFixedWidth(title_W)
        self.temp_show.setAlignment(Qt.AlignCenter)
        self.temp_show.setReadOnly(True)
        self.temp_show.setTitle("温度")
        self.temp_show.setText("0°C")

        self.fault_warnning_show = SiCapsuleLineEdit()
        self.fault_warnning_show.resize(all_W, 40)
        self.fault_warnning_show.setTitleWidthMode(
            SiCapsuleLineEdit.TitleWidthMode.Fixed
        )
        self.fault_warnning_show.setTitleFixedWidth(title_W)
        self.fault_warnning_show.setAlignment(Qt.AlignCenter)
        self.fault_warnning_show.setReadOnly(True)
        self.fault_warnning_show.setTitle("错误")
        self.fault_warnning_show.setText("None")

        # 创建主水平布局
        status_H_layout = QHBoxLayout(status_area)
        status_H_layout.setContentsMargins(12, 6, 12, 6)  # 根据需要调整边距
        status_H_layout.setSpacing(16)
        # 创建状态区域垂直布局
        status_W_layout_t = QVBoxLayout()
        status_W_layout_t.setContentsMargins(0, 0, 0, 0)  # 根据需要调整边距
        status_W_layout_t.setSpacing(8)  # 行间距
        status_W_layout_t.addWidget(self.sensormode_show)
        status_W_layout_t.addWidget(self.runmode_show)
        status_W_layout_t.addWidget(self.state_show)

        status_W_layout_b = QVBoxLayout()
        status_W_layout_b.setContentsMargins(0, 0, 0, 0)  # 根据需要调整边距
        status_W_layout_b.setSpacing(8)  # 行间距
        status_W_layout_b.addWidget(self.Vbus_show)
        status_W_layout_b.addWidget(self.temp_show)
        status_W_layout_b.addWidget(self.fault_warnning_show)

        status_H_layout.addLayout(status_W_layout_t, 1)
        status_H_layout.addLayout(status_W_layout_b, 1)

        #############################################################################
        # 创建复位按钮

        self.reset_button = SiPushButtonRefactor()
        self.reset_button.setText("FOC复位")
        self.reset_button.adjustSize()

        self.system_reset_button = SiPushButtonRefactor()
        self.system_reset_button.setText("系统复位")
        self.system_reset_button.adjustSize()

        # 创建主垂直布局
        mode_V_layout = QVBoxLayout(but_area)
        mode_V_layout.setContentsMargins(12, 6, 12, 6)  # 根据需要调整边距
        mode_V_layout.setSpacing(8)  # 行间距

        mode_V_layout.addWidget(self.system_reset_button)
        mode_V_layout.addWidget(self.reset_button)

        #############################################################################
        # 配置文件区域显示控件
        self.config_file = SiCapsuleComboBox()
        self.config_file.setTitle("配置")
        self.config_file.setMinimumHeight(30)
        self.config_file.setEditable(False)
        self.config_file.addItem("默认")

        self.load_config = SiPushButtonRefactor()
        self.load_config.setText("加载配置")
        self.load_config.setFixedHeight(30)
        self.load_config.adjustSize()

        self.save_config = SiPushButtonRefactor()
        self.save_config.setText("保存")
        self.save_config.setFixedHeight(30)
        self.save_config.adjustSize()

        self.remove_config = SiLongPressButtonRefactor()
        self.remove_config.setText("删除")
        self.remove_config.setFixedHeight(30)
        self.remove_config.setToolTip("长按删除配置")
        self.remove_config.adjustSize()

        # 创建主垂直布局
        config_H_layout = QVBoxLayout(config_area)
        config_H_layout.setContentsMargins(12, 6, 12, 6)  # 根据需要调整边距
        config_H_layout.setSpacing(8)
        # 创建配置区域水平布局
        config_W_layout_b = QHBoxLayout()
        config_W_layout_b.setContentsMargins(0, 0, 0, 0)  # 根据需要调整边距
        config_W_layout_b.setSpacing(8)
        config_W_layout_b.addWidget(self.save_config)
        config_W_layout_b.addWidget(self.remove_config)

        config_H_layout.addWidget(self.config_file, 1)
        config_H_layout.addWidget(self.load_config, 1)
        config_H_layout.addLayout(config_W_layout_b, 1)
