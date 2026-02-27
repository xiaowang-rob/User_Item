from siui.components.combobox_ import SiCapsuleComboBox
from PyQt5.QtWidgets import QVBoxLayout, QHBoxLayout
from PyQt5.QtCore import Qt
from siui.components.button import (
    SiCapsuleButton,
    SiLongPressButtonRefactor,
    SiPushButtonRefactor
)
from siui.components.editbox import SiCapsuleLineEdit 




class TopArea:
    def __init__(self, main_window):
        self.mw=main_window
        self.connect_area=main_window.ui.connect_area
        self.mode_area=main_window.ui.mode_area
        self.status_area=main_window.ui.status_area
        self.config_area=main_window.ui.config_area


        # 统一四个角的圆角半径
        self.connect_area.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;  
        """)
        self.mode_area.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;  
        """)
        self.status_area.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;
        """)
        self.config_area.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;
        """)

    
        # 创建连接区域按钮 
        ###################################################################
        self.com_port=SiCapsuleComboBox()
        self.com_port.setTitle("端口")
        self.com_port.setMinimumHeight(36)
        self.com_port.setEditable(False)

        self.system_message=SiPushButtonRefactor()
        self.system_message.setSvgIcon(self.mw.icon.get("ic_fluent_memory_regular","#DFDFDF"))
        self.system_message.setToolTip("系统信息")
        self.system_message.adjustSize()
        

        self.connect_but=SiCapsuleButton()
        self.connect_but.setText("未连接")
        self.connect_but.setValue("连接")
        # 创建主垂直布局
        connect_H_layout = QVBoxLayout(self.connect_area)
        connect_H_layout.setContentsMargins(6, 6, 6, 6)  # 根据需要调整边距
        connect_H_layout.setSpacing(8)  # 行间距

        # 第一行：端口选择框
        connect_H_layout.addWidget(self.com_port)

        # 第二行：水平布局包含两个按钮
        connect_W_layout = QHBoxLayout()
        connect_W_layout.setSpacing(16)
        connect_W_layout.addWidget(self.system_message,1)
        connect_W_layout.addWidget(self.connect_but,3)

        connect_H_layout.addLayout(connect_W_layout)
        #############################################################################
        # 创建模式区域按钮

        title_W=50
        all_W=150

        self.sensormode_show=SiCapsuleLineEdit()
        self.sensormode_show.resize(all_W,40)
        self.sensormode_show.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.sensormode_show.setTitleFixedWidth(title_W) 
        self.sensormode_show.setAlignment(Qt.AlignCenter) 
        self.sensormode_show.setReadOnly(True)
        self.sensormode_show.setTitle("感应模式")

        self.runmode_show=SiCapsuleLineEdit()
        self.runmode_show.resize(all_W,40)
        self.runmode_show.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.runmode_show.setTitleFixedWidth(title_W)  
        self.runmode_show.setAlignment(Qt.AlignCenter) 
        self.runmode_show.setReadOnly(True)
        self.runmode_show.setTitle("运行模式")

        self.canid_show=SiCapsuleLineEdit()
        self.canid_show.resize(all_W,40)
        self.canid_show.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.canid_show.setTitleFixedWidth(title_W)  
        self.canid_show.setAlignment(Qt.AlignCenter) 
        self.canid_show.setReadOnly(True)
        self.canid_show.setTitle("CAN ID")
 
        self.canmode_show=SiCapsuleLineEdit()
        self.canmode_show.resize(all_W,40)
        self.canmode_show.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.canmode_show.setTitleFixedWidth(title_W) 
        self.canmode_show.setAlignment(Qt.AlignCenter) 
        self.canmode_show.setReadOnly(True)
        self.canmode_show.setTitle("CAN模式")

        #创建主水平布局
        mode_H_layout = QHBoxLayout(self.mode_area)
        mode_H_layout.setContentsMargins(12,6,12,6)  # 根据需要调整边距
        mode_H_layout.setSpacing(16)  
        #创建模式区域垂直布局
        mode_V_layout_l = QVBoxLayout()
        mode_V_layout_l.setContentsMargins(0, 0, 0,0)  # 根据需要调整边距
        mode_V_layout_l.setSpacing(8)  # 行间距
        mode_V_layout_l.addWidget(self.sensormode_show)
        mode_V_layout_l.addWidget(self.runmode_show)

        mode_V_layout_r = QVBoxLayout()
        mode_V_layout_r.setContentsMargins(0, 0, 0,0)  # 根据需要调整边距
        mode_V_layout_r.setSpacing(8)  # 行间距
        mode_V_layout_r.addWidget(self.canid_show)
        mode_V_layout_r.addWidget(self.canmode_show)
        
        mode_H_layout.addLayout(mode_V_layout_l,1)
        mode_H_layout.addLayout(mode_V_layout_r,1)

        #############################################################################
        # 状态区域显示控件

        self.focstate_show=SiCapsuleLineEdit()
        self.focstate_show.resize(all_W,40)
        self.focstate_show.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.focstate_show.setTitleFixedWidth(title_W)  
        self.focstate_show.setAlignment(Qt.AlignCenter) 
        self.focstate_show.setReadOnly(True)
        self.focstate_show.setTitle("FOC状态")
        self.focstate_show.setText("IDLE")

        self.systemstate_show=SiCapsuleLineEdit()
        self.systemstate_show.resize(all_W,40)
        self.systemstate_show.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.systemstate_show.setTitleFixedWidth(title_W)  
        self.systemstate_show.setAlignment(Qt.AlignCenter) 
        self.systemstate_show.setReadOnly(True)
        self.systemstate_show.setTitle("系统状态")
        self.systemstate_show.setText("IDLE")

        self.Vbus_show=SiCapsuleLineEdit()
        self.Vbus_show.resize(all_W,40)
        self.Vbus_show.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.Vbus_show.setTitleFixedWidth(title_W)  
        self.Vbus_show.setAlignment(Qt.AlignCenter) 
        self.Vbus_show.setReadOnly(True)
        self.Vbus_show.setTitle("电压")
        self.Vbus_show.setText("0.0V")

        self.temp_show=SiCapsuleLineEdit()
        self.temp_show.resize(all_W,40)
        self.temp_show.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.temp_show.setTitleFixedWidth(title_W)  
        self.temp_show.setAlignment(Qt.AlignCenter) 
        self.temp_show.setReadOnly(True)
        self.temp_show.setTitle("温度")
        self.temp_show.setText("0°C")

        self.fault_show=SiCapsuleLineEdit()
        self.fault_show.resize(all_W,40)
        self.fault_show.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.fault_show.setTitleFixedWidth(title_W)  
        self.fault_show.setAlignment(Qt.AlignCenter) 
        self.fault_show.setReadOnly(True)
        self.fault_show.setTitle("错误")
        self.fault_show.setText("无故障")

        self.warning_show=SiCapsuleLineEdit()
        self.warning_show.resize(all_W,40)
        self.warning_show.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.warning_show.setTitleFixedWidth(title_W)  
        self.warning_show.setAlignment(Qt.AlignCenter) 
        self.warning_show.setReadOnly(True)
        self.warning_show.setTitle("警告")
        self.warning_show.setText("无警告")


        #创建主垂直布局
        status_H_layout = QHBoxLayout(self.status_area)
        status_H_layout.setContentsMargins(12,6,12,6)  # 根据需要调整边距
        status_H_layout.setSpacing(16)  
        #创建状态区域水平布局
        status_W_layout_t = QVBoxLayout()
        status_W_layout_t.setContentsMargins(0, 0, 0,0)  # 根据需要调整边距
        status_W_layout_t.setSpacing(8)  # 行间距
        status_W_layout_t.addWidget(self.focstate_show)
        status_W_layout_t.addWidget(self.Vbus_show)
        status_W_layout_t.addWidget(self.fault_show)

        status_W_layout_b = QVBoxLayout()
        status_W_layout_b.setContentsMargins(0, 0, 0,0)  # 根据需要调整边距
        status_W_layout_b.setSpacing(8)  # 行间距
        status_W_layout_b.addWidget(self.systemstate_show)
        status_W_layout_b.addWidget(self.temp_show)
        status_W_layout_b.addWidget(self.warning_show)
        
        status_H_layout.addLayout(status_W_layout_t,1)
        status_H_layout.addLayout(status_W_layout_b,1)


        #############################################################################
        # 配置文件区域显示控件
        self.config_file=SiCapsuleComboBox()
        self.config_file.setTitle("配置")
        self.config_file.setMinimumHeight(30)
        self.config_file.setEditable(False)
        self.config_file.addItem("默认")

        self.load_config=SiPushButtonRefactor()
        self.load_config.setText("加载配置")
        self.load_config.setFixedHeight(30)
        self.load_config.adjustSize()

        self.save_config=SiPushButtonRefactor()
        self.save_config.setText("保存")
        self.save_config.setFixedHeight(30)
        self.save_config.adjustSize()

        self.remove_config=SiLongPressButtonRefactor()
        self.remove_config.setText("删除")
        self.remove_config.setFixedHeight(30)
        self.remove_config.setToolTip("长按删除配置")
        self.remove_config.adjustSize()

        #创建主垂直布局
        config_H_layout = QVBoxLayout(self.config_area)
        config_H_layout.setContentsMargins(12,6,12,6)  # 根据需要调整边距
        config_H_layout.setSpacing(8)  
        #创建配置区域水平布局
        config_W_layout_b = QHBoxLayout()
        config_W_layout_b.setContentsMargins(0, 0, 0,0)  # 根据需要调整边距
        config_W_layout_b.setSpacing(8) 
        config_W_layout_b.addWidget(self.save_config)
        config_W_layout_b.addWidget(self.remove_config)

        config_H_layout.addWidget(self.config_file,1)
        config_H_layout.addWidget(self.load_config,1)
        config_H_layout.addLayout(config_W_layout_b,1)








    