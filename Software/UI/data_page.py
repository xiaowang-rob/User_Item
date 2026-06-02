from PyQt5.QtWidgets import QWidget, QHBoxLayout, QVBoxLayout,QScrollArea
from PyQt5.QtCore import Qt
from siui.components.button import (
    SiLongPressButtonRefactor,
    SiPushButtonRefactor,
)
from siui.components.editbox import SiCapsuleLineEdit
from siui.components.widgets.label import SiLabel
from siui.core import GlobalFont
from siui.core import Si
from siui.gui import SiFont
from siui.components.combobox_ import SiCapsuleComboBox

from protocol import Midx


title_W = 80
all_W = 240


class DataPage:
    def __init__(self, main_window):
        self.mw = main_window
        self.widget = main_window.ui.data_page

        main_layout = QHBoxLayout(self.widget)
        main_layout.setContentsMargins(24, 12, 24, 0)
        main_layout.setSpacing(36)

        self.control_group = QWidget(self.widget)
        self.control_group.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;  
        """)
        self.mode_group = QWidget(self.widget)
        self.mode_group.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;  
        """)
        self.motor_group = QWidget(self.widget)
        self.motor_group.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;  
        """)
        self.log_group = QWidget(self.widget)
        self.log_group.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;  
        """)

        # 控制参数##################################################################
        control_layout = QVBoxLayout(self.control_group)
        control_layout.setContentsMargins(24, 12, 24, 12)
        control_layout.setSpacing(12)

        Title_control = SiLabel()
        Title_control.setStyleSheet("color:#E5E5E5;")
        Title_control.setText("控制参数")
        Title_control.setFont(SiFont.tokenized(GlobalFont.M_BOLD))
        Title_control.setFixedHeight(26)
        Title_control.setAlignment(Qt.AlignBottom)
        Title_control.setSiliconWidgetFlag(Si.AdjustSizeOnTextChanged)

        button_layout1 = QHBoxLayout()
        button_layout1.setContentsMargins(0, 0, 0, 0)
        button_layout1.setSpacing(12)

        self.all_read_button = SiPushButtonRefactor()
        self.all_read_button.setText("一键读取")
        self.all_read_button.adjustSize()
        self.all_read_button.setFixedHeight(30)
        button_layout1.addWidget(self.all_read_button)

        self.all_write_button = SiPushButtonRefactor()
        self.all_write_button.setText("一键写入")
        self.all_write_button.adjustSize()
        self.all_write_button.setFixedHeight(30)
        button_layout1.addWidget(self.all_write_button)

        button_layout2 = QHBoxLayout()
        button_layout2.setContentsMargins(0, 0, 0, 0)
        button_layout2.setSpacing(12)

        self.all_save_button = SiPushButtonRefactor()
        self.all_save_button.setText("一键保存")
        self.all_save_button.adjustSize()
        self.all_save_button.setFixedHeight(30)
        button_layout2.addWidget(self.all_save_button)

        self.all_erase_button = SiLongPressButtonRefactor()
        self.all_erase_button.setText("清除参数")
        self.all_erase_button.setToolTip("长按清除所有参数")
        self.all_erase_button.adjustSize()
        self.all_erase_button.setFixedHeight(30)
        button_layout2.addWidget(self.all_erase_button)


        control_scroll = QScrollArea()
        control_scroll.setWidgetResizable(True)
        control_scroll.setStyleSheet("""
            QScrollBar:vertical {
                background: #2b2b2b;          /* 轨道背景 */
                width: 8px;                   /* 滚动条宽度 */
                margin: 0px;
                border-radius: 4px;
            }
            QScrollBar::handle:vertical {
                background: #666666;          /* 滑块颜色 */
                min-height: 30px;
                border-radius: 4px;
            }
            QScrollBar::handle:vertical:hover {
                background: #888888;          /* 鼠标悬停 */
            }
            QScrollBar::handle:vertical:pressed {
                background: #aaaaaa;          /* 滑块按下 */
            }
            QScrollBar::sub-line:vertical,
            QScrollBar::add-line:vertical {
                height: 0px;                  /* 隐藏上下箭头 */
            }
            QScrollBar::add-page:vertical,
            QScrollBar::sub-page:vertical {
                background: none;
            }
        """)
        control_container = QWidget()
        control_layout_scroll = QVBoxLayout(control_container)
        control_layout_scroll.setContentsMargins(0, 0, 0, 0)
        control_layout_scroll.setAlignment(Qt.AlignTop) 
        control_layout_scroll.setSpacing(12)

        control_scroll.setWidget(control_container)

        control_layout.addWidget(Title_control)
        control_layout.addLayout(button_layout1)
        control_layout.addLayout(button_layout2)
        control_layout.addWidget(control_scroll)
    
        # CAN ID
        self.CAN_ID_input = SiCapsuleLineEdit()
        self.CAN_ID_input.resize(all_W, 40)
        self.CAN_ID_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.CAN_ID_input.setTitleFixedWidth(title_W)
        self.CAN_ID_input.setAlignment(Qt.AlignCenter)
        self.CAN_ID_input.setTitle("CAN ID")
        self.CAN_ID_input.setText("0")

        # 速度环 P
        self.speed_loop_P_input = SiCapsuleLineEdit()
        self.speed_loop_P_input.resize(all_W, 40)
        self.speed_loop_P_input.setTitleWidthMode(
            SiCapsuleLineEdit.TitleWidthMode.Fixed
        )
        self.speed_loop_P_input.setTitleFixedWidth(title_W)
        self.speed_loop_P_input.setAlignment(Qt.AlignCenter)
        self.speed_loop_P_input.setTitle("速度环P")
        self.speed_loop_P_input.setText("0")

        # 速度环 I
        self.speed_loop_I_input = SiCapsuleLineEdit()
        self.speed_loop_I_input.resize(all_W, 40)
        self.speed_loop_I_input.setTitleWidthMode(
            SiCapsuleLineEdit.TitleWidthMode.Fixed
        )
        self.speed_loop_I_input.setTitleFixedWidth(title_W)
        self.speed_loop_I_input.setAlignment(Qt.AlignCenter)
        self.speed_loop_I_input.setTitle("速度环I")
        self.speed_loop_I_input.setText("0")

        # 位置环 P
        self.position_loop_P_input = SiCapsuleLineEdit()
        self.position_loop_P_input.resize(all_W, 40)
        self.position_loop_P_input.setTitleWidthMode(
            SiCapsuleLineEdit.TitleWidthMode.Fixed
        )
        self.position_loop_P_input.setTitleFixedWidth(title_W)
        self.position_loop_P_input.setAlignment(Qt.AlignCenter)
        self.position_loop_P_input.setTitle("位置环P")
        self.position_loop_P_input.setText("0")

        # 位置环 I
        self.position_loop_I_input = SiCapsuleLineEdit()
        self.position_loop_I_input.resize(all_W, 40)
        self.position_loop_I_input.setTitleWidthMode(
            SiCapsuleLineEdit.TitleWidthMode.Fixed
        )
        self.position_loop_I_input.setTitleFixedWidth(title_W)
        self.position_loop_I_input.setAlignment(Qt.AlignCenter)
        self.position_loop_I_input.setTitle("位置环I")
        self.position_loop_I_input.setText("0")

        # 位置环 D
        self.position_loop_D_input = SiCapsuleLineEdit()
        self.position_loop_D_input.resize(all_W, 40)
        self.position_loop_D_input.setTitleWidthMode(
            SiCapsuleLineEdit.TitleWidthMode.Fixed
        )
        self.position_loop_D_input.setTitleFixedWidth(title_W)
        self.position_loop_D_input.setAlignment(Qt.AlignCenter)
        self.position_loop_D_input.setTitle("位置环D")
        self.position_loop_D_input.setText("0")

        self.traj_max_rate = SiCapsuleLineEdit()
        self.traj_max_rate.resize(all_W, 40)
        self.traj_max_rate.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.traj_max_rate.setTitleFixedWidth(title_W)
        self.traj_max_rate.setAlignment(Qt.AlignCenter)
        self.traj_max_rate.setTitle("最大变化率")
        self.traj_max_rate.setText("100")

        self.traj_max_acc = SiCapsuleLineEdit()
        self.traj_max_acc.resize(all_W, 40)
        self.traj_max_acc.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.traj_max_acc.setTitleFixedWidth(title_W)
        self.traj_max_acc.setAlignment(Qt.AlignCenter)
        self.traj_max_acc.setTitle("最大加速度")
        self.traj_max_acc.setText("50")

        self.traj_max_jerk = SiCapsuleLineEdit()
        self.traj_max_jerk.resize(all_W, 40)
        self.traj_max_jerk.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.traj_max_jerk.setTitleFixedWidth(title_W)
        self.traj_max_jerk.setAlignment(Qt.AlignCenter)
        self.traj_max_jerk.setTitle("最大加加速度")
        self.traj_max_jerk.setText("200")

        self.traj_tolerance = SiCapsuleLineEdit()
        self.traj_tolerance.resize(all_W, 40)
        self.traj_tolerance.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.traj_tolerance.setTitleFixedWidth(title_W)
        self.traj_tolerance.setAlignment(Qt.AlignCenter)
        self.traj_tolerance.setTitle("容差")
        self.traj_tolerance.setText("0.01")

        self.tune_current = SiCapsuleLineEdit()
        self.tune_current.resize(all_W, 40)
        self.tune_current.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.tune_current.setTitleFixedWidth(title_W)
        self.tune_current.setAlignment(Qt.AlignCenter)
        self.tune_current.setTitle("校准电流/A")
        self.tune_current.setText("50")

        self.limit_current = SiCapsuleLineEdit()
        self.limit_current.resize(all_W, 40)
        self.limit_current.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.limit_current.setTitleFixedWidth(title_W)
        self.limit_current.setAlignment(Qt.AlignCenter)
        self.limit_current.setTitle("电流限幅/A")
        self.limit_current.setText("50")

        self.limit_speed = SiCapsuleLineEdit()
        self.limit_speed.resize(all_W, 40)
        self.limit_speed.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.limit_speed.setTitleFixedWidth(title_W)
        self.limit_speed.setAlignment(Qt.AlignCenter)
        self.limit_speed.setTitle("速度限幅/rpm")
        self.limit_speed.setText("1000")

        self.min_position = SiCapsuleLineEdit()
        self.min_position.resize(all_W, 40)
        self.min_position.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.min_position.setTitleFixedWidth(title_W)
        self.min_position.setAlignment(Qt.AlignCenter)
        self.min_position.setTitle("最小位置/°")
        self.min_position.setText("-10000")

        self.max_position = SiCapsuleLineEdit()
        self.max_position.resize(all_W, 40)
        self.max_position.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.max_position.setTitleFixedWidth(title_W)
        self.max_position.setAlignment(Qt.AlignCenter)
        self.max_position.setTitle("最大位置/°")
        self.max_position.setText("10000")

        self.tolerance_time = SiCapsuleLineEdit()
        self.tolerance_time.resize(all_W, 40)
        self.tolerance_time.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.tolerance_time.setTitleFixedWidth(title_W)
        self.tolerance_time.setAlignment(Qt.AlignCenter)
        self.tolerance_time.setTitle("容忍时间/ms")
        self.tolerance_time.setText("1")

        self.tolerance_limit = SiCapsuleLineEdit()
        self.tolerance_limit.resize(all_W, 40)
        self.tolerance_limit.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.tolerance_limit.setTitleFixedWidth(title_W)
        self.tolerance_limit.setAlignment(Qt.AlignCenter)
        self.tolerance_limit.setTitle("容忍度")
        self.tolerance_limit.setText("1.0")

  

        control_layout_scroll.addWidget(self.CAN_ID_input)
        control_layout_scroll.addWidget(self.speed_loop_P_input)
        control_layout_scroll.addWidget(self.speed_loop_I_input)
        control_layout_scroll.addWidget(self.position_loop_P_input)
        control_layout_scroll.addWidget(self.position_loop_I_input)
        control_layout_scroll.addWidget(self.position_loop_D_input)
        control_layout_scroll.addWidget(self.traj_max_rate)
        control_layout_scroll.addWidget(self.traj_max_acc)
        control_layout_scroll.addWidget(self.traj_max_jerk)
        control_layout_scroll.addWidget(self.traj_tolerance)
        control_layout_scroll.addWidget(self.tune_current)
        control_layout_scroll.addWidget(self.limit_current)
        control_layout_scroll.addWidget(self.limit_speed)
        control_layout_scroll.addWidget(self.min_position)
        control_layout_scroll.addWidget(self.max_position)
        control_layout_scroll.addWidget(self.tolerance_time)
        control_layout_scroll.addWidget(self.tolerance_limit)


        # 模式参数##############################################
        mode_layout = QVBoxLayout(self.mode_group)
        mode_layout.setContentsMargins(24, 12, 24, 12)
        mode_layout.setSpacing(12)

        Title_mode = SiLabel()
        Title_mode.setStyleSheet("color:#E5E5E5;")
        Title_mode.setText("模式设置")
        Title_mode.setFont(SiFont.tokenized(GlobalFont.M_BOLD))
        Title_mode.setFixedHeight(26)
        Title_mode.setAlignment(Qt.AlignBottom)
        Title_mode.setSiliconWidgetFlag(Si.AdjustSizeOnTextChanged)

        self.encoder_input = SiCapsuleComboBox()
        self.encoder_input.setTitle("编码器")
        self.encoder_input.setEditable(False)
        self.encoder_input.setFixedHeight(36)
        self.encoder_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.encoder_input.setTitleFixedWidth(title_W)
        for i in Midx.encoder_chip:
            if i !=  Midx.encoder_chip[len(Midx.encoder_chip)-1]:
                self.encoder_input.addItem(i)

        self.sensormode_input = SiCapsuleComboBox()
        self.sensormode_input.setTitle("感应模式")
        self.sensormode_input.setEditable(False)
        self.sensormode_input.setFixedHeight(36)
        self.sensormode_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.sensormode_input.setTitleFixedWidth(title_W)
        for i in Midx.sensor_mode:
            self.sensormode_input.addItem(i)

        self.runmode_input = SiCapsuleComboBox()
        self.runmode_input.setTitle("运行模式")
        self.runmode_input.setEditable(False)
        self.runmode_input.setFixedHeight(36)
        self.runmode_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.runmode_input.setTitleFixedWidth(title_W)
        for i in Midx.run_mode:
            self.runmode_input.addItem(i)

        self.can_mode_input = SiCapsuleComboBox()
        self.can_mode_input.setTitle("CAN模式")
        self.can_mode_input.setEditable(False)
        self.can_mode_input.setFixedHeight(36)
        self.can_mode_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.can_mode_input.setTitleFixedWidth(title_W)
        for i in Midx.can_mode:
            self.can_mode_input.addItem(i)

        self.vaguePID_input = SiCapsuleComboBox()
        self.vaguePID_input.setTitle("模糊PID")
        self.vaguePID_input.setFixedHeight(36)
        self.vaguePID_input.setEditable(False)
        self.vaguePID_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.vaguePID_input.setTitleFixedWidth(title_W)
        for i in Midx.vague_PID_mode:
            self.vaguePID_input.addItem(i)

        self.PVT_mode_input = SiCapsuleComboBox()
        self.PVT_mode_input.setTitle("PVT模式")
        self.PVT_mode_input.setEditable(False)
        self.PVT_mode_input.setFixedHeight(36)
        self.PVT_mode_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.PVT_mode_input.setTitleFixedWidth(title_W)
        for i in Midx.pvt_mode:
            self.PVT_mode_input.addItem(i)

        self.TRAJ_mode_input = SiCapsuleComboBox()
        self.TRAJ_mode_input.setTitle("轨迹模式")
        self.TRAJ_mode_input.setEditable(False)
        self.TRAJ_mode_input.setFixedHeight(36)
        self.TRAJ_mode_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.TRAJ_mode_input.setTitleFixedWidth(title_W)
        for i in Midx.traj_type:
            self.TRAJ_mode_input.addItem(i)

        mode_layout.addWidget(Title_mode)
        mode_layout.addWidget(self.encoder_input)
        mode_layout.addWidget(self.sensormode_input)
        mode_layout.addWidget(self.runmode_input)
        mode_layout.addWidget(self.can_mode_input)
        mode_layout.addWidget(self.vaguePID_input)
        mode_layout.addWidget(self.PVT_mode_input)
        mode_layout.addWidget(self.TRAJ_mode_input)
        mode_layout.setAlignment(Qt.AlignTop)

        # 电机参数#########################################################
        motor_layout = QVBoxLayout(self.motor_group)
        motor_layout.setContentsMargins(24, 12, 24, 12)
        motor_layout.setSpacing(12)

        Title_motor = SiLabel()
        Title_motor.setStyleSheet("color:#E5E5E5;")
        Title_motor.setText("电机参数")
        Title_motor.setFont(SiFont.tokenized(GlobalFont.M_BOLD))
        Title_motor.setFixedHeight(26)
        Title_motor.setAlignment(Qt.AlignBottom)
        Title_motor.setSiliconWidgetFlag(Si.AdjustSizeOnTextChanged)

        self.offsetangle_input = SiCapsuleLineEdit()
        self.offsetangle_input.resize(all_W, 40)
        self.offsetangle_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.offsetangle_input.setTitleFixedWidth(title_W)
        self.offsetangle_input.setAlignment(Qt.AlignCenter)
        self.offsetangle_input.setTitle("偏转角度/°")
        self.offsetangle_input.setText("0")

        self.motor_polepairs_input = SiCapsuleLineEdit()
        self.motor_polepairs_input.resize(all_W, 40)
        self.motor_polepairs_input.setTitleWidthMode(
            SiCapsuleLineEdit.TitleWidthMode.Fixed
        )
        self.motor_polepairs_input.setTitleFixedWidth(title_W)
        self.motor_polepairs_input.setAlignment(Qt.AlignCenter)
        self.motor_polepairs_input.setTitle("电机极对数")
        self.motor_polepairs_input.setText("7")

        self.motor_KV_input = SiCapsuleLineEdit()
        self.motor_KV_input.resize(all_W, 40)
        self.motor_KV_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.motor_KV_input.setTitleFixedWidth(title_W)
        self.motor_KV_input.setAlignment(Qt.AlignCenter)
        self.motor_KV_input.setTitle("电机KV")
        self.motor_KV_input.setText("0")

        self.motor_resistance_input = SiCapsuleLineEdit()
        self.motor_resistance_input.resize(all_W, 40)
        self.motor_resistance_input.setTitleWidthMode(
            SiCapsuleLineEdit.TitleWidthMode.Fixed
        )
        self.motor_resistance_input.setTitleFixedWidth(title_W)
        self.motor_resistance_input.setAlignment(Qt.AlignCenter)
        self.motor_resistance_input.setTitle("相电阻")
        self.motor_resistance_input.setText("0.001")

        self.motor_Ld_input = SiCapsuleLineEdit()
        self.motor_Ld_input.resize(all_W, 40)
        self.motor_Ld_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.motor_Ld_input.setTitleFixedWidth(title_W)
        self.motor_Ld_input.setAlignment(Qt.AlignCenter)
        self.motor_Ld_input.setTitle("d轴电感")
        self.motor_Ld_input.setText("0.001")

        self.motor_Lq_input = SiCapsuleLineEdit()
        self.motor_Lq_input.resize(all_W, 40)
        self.motor_Lq_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.motor_Lq_input.setTitleFixedWidth(title_W)
        self.motor_Lq_input.setAlignment(Qt.AlignCenter)
        self.motor_Lq_input.setTitle("q轴电感")
        self.motor_Lq_input.setText("0.001")

        self.motor_psif_input = SiCapsuleLineEdit()
        self.motor_psif_input.resize(all_W, 40)
        self.motor_psif_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.motor_psif_input.setTitleFixedWidth(title_W)
        self.motor_psif_input.setAlignment(Qt.AlignCenter)
        self.motor_psif_input.setTitle("磁链")
        self.motor_psif_input.setText("0.001")

        self.motor_Ke_input = SiCapsuleLineEdit()
        self.motor_Ke_input.resize(all_W, 40)
        self.motor_Ke_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.motor_Ke_input.setTitleFixedWidth(title_W)
        self.motor_Ke_input.setAlignment(Qt.AlignCenter)
        self.motor_Ke_input.setTitle("反电动势")
        self.motor_Ke_input.setText("0.001")

        self.motor_J_input = SiCapsuleLineEdit()
        self.motor_J_input.resize(all_W, 40)
        self.motor_J_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.motor_J_input.setTitleFixedWidth(title_W)
        self.motor_J_input.setAlignment(Qt.AlignCenter)
        self.motor_J_input.setTitle("转动惯量")
        self.motor_J_input.setText("0.001")

        self.motor_B_input = SiCapsuleLineEdit()
        self.motor_B_input.resize(all_W, 40)
        self.motor_B_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.motor_B_input.setTitleFixedWidth(title_W)
        self.motor_B_input.setAlignment(Qt.AlignCenter)
        self.motor_B_input.setTitle("摩擦系数")
        self.motor_B_input.setText("0.001")

        motor_layout.addWidget(Title_motor)
        motor_layout.addWidget(self.offsetangle_input)
        motor_layout.addWidget(self.motor_polepairs_input)
        motor_layout.addWidget(self.motor_KV_input)
        motor_layout.addWidget(self.motor_resistance_input)
        motor_layout.addWidget(self.motor_Ld_input)
        motor_layout.addWidget(self.motor_Lq_input)
        motor_layout.addWidget(self.motor_psif_input)
        motor_layout.addWidget(self.motor_Ke_input)
        motor_layout.addWidget(self.motor_J_input)
        motor_layout.addWidget(self.motor_B_input)
        motor_layout.setAlignment(Qt.AlignTop)


        #日志区域#########################################################
        Title_log = SiLabel()
        Title_log.setStyleSheet("color:#E5E5E5;")
        Title_log.setText("日志信息")
        Title_log.setFont(SiFont.tokenized(GlobalFont.M_BOLD))
        Title_log.setFixedHeight(26)
        Title_log.setAlignment(Qt.AlignBottom)
        Title_log.setSiliconWidgetFlag(Si.AdjustSizeOnTextChanged)

        log_layout = QVBoxLayout(self.log_group)
        log_layout.setContentsMargins(24, 12, 24, 12)
        log_layout.setSpacing(12)

        but_ton_layout = QHBoxLayout()
        but_ton_layout.setContentsMargins(0, 0, 0, 0)
        but_ton_layout.setSpacing(12)
        
        self.read_log_btn = SiPushButtonRefactor()
        self.read_log_btn.setText("读取日志")
        self.read_log_btn.adjustSize()
        self.read_log_btn.setFixedHeight(30)

        self.clear_log_btn = SiLongPressButtonRefactor()
        self.clear_log_btn.setText("清空日志")
        self.clear_log_btn.setToolTip("长按清空日志")
        self.clear_log_btn.adjustSize()
        self.clear_log_btn.setFixedHeight(30)

        but_ton_layout.addWidget(self.read_log_btn)
        but_ton_layout.addWidget(self.clear_log_btn)

        log_select_layout = QHBoxLayout()
        log_select_layout.setContentsMargins(0, 0, 0, 0)
        log_select_layout.setSpacing(12)

        self.log_num = SiCapsuleComboBox()
        self.log_num.setTitle("选择日志")
        self.log_num.setMinimumHeight(36)
        self.log_num.setEditable(False)

        self.show_log_btn = SiPushButtonRefactor()
        self.show_log_btn.setSvgIcon(
            self.mw.icon.get("ic_fluent_triangle_right_filled", "#DFDFDF")
        )
        self.show_log_btn.setToolTip("显示日志")
        self.show_log_btn.adjustSize()

        log_select_layout.addWidget(self.log_num,2)
        log_select_layout.addWidget(self.show_log_btn,1)

        log_scroll = QScrollArea()
        log_scroll.setWidgetResizable(True)
        log_scroll.setStyleSheet("""
            QScrollBar:vertical {
                background: #2b2b2b;          /* 轨道背景 */
                width: 8px;                   /* 滚动条宽度 */
                margin: 0px;
                border-radius: 4px;
            }
            QScrollBar::handle:vertical {
                background: #666666;          /* 滑块颜色 */
                min-height: 30px;
                border-radius: 4px;
            }
            QScrollBar::handle:vertical:hover {
                background: #888888;          /* 鼠标悬停 */
            }
            QScrollBar::handle:vertical:pressed {
                background: #aaaaaa;          /* 滑块按下 */
            }
            QScrollBar::sub-line:vertical,
            QScrollBar::add-line:vertical {
                height: 0px;                  /* 隐藏上下箭头 */
            }
            QScrollBar::add-page:vertical,
            QScrollBar::sub-page:vertical {
                background: none;
            }
        """)
        log_container = QWidget()
        log_layout_scroll = QVBoxLayout(log_container)
        log_layout_scroll.setContentsMargins(0, 0, 0, 0)
        log_layout_scroll.setAlignment(Qt.AlignTop) 
        log_layout_scroll.setSpacing(12)

        log_scroll.setWidget(log_container)

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


        self.can_status = SiCapsuleLineEdit()
        self.can_status.setReadOnly(True)
        self.can_status.resize(all_W, 40)
        self.can_status.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.can_status.setTitleFixedWidth(title_W)
        self.can_status.setAlignment(Qt.AlignCenter)
        self.can_status.setTitle("CAN状态")


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

        log_layout_scroll.addWidget(self.num)
        log_layout_scroll.addWidget(self.time)
        log_layout_scroll.addWidget(self.fault)
        log_layout_scroll.addWidget(self.warning)
        log_layout_scroll.addWidget(self.sensor_mode)
        log_layout_scroll.addWidget(self.run_mode)
        log_layout_scroll.addWidget(self.can_status)    
        log_layout_scroll.addWidget(self.encode_status)
        log_layout_scroll.addWidget(self.voltage)
        log_layout_scroll.addWidget(self.temperature)
        log_layout_scroll.addWidget(self.iu)
        log_layout_scroll.addWidget(self.iv)
        log_layout_scroll.addWidget(self.iw)
        log_layout_scroll.addWidget(self.id)
        log_layout_scroll.addWidget(self.id_ref)
        log_layout_scroll.addWidget(self.iq)
        log_layout_scroll.addWidget(self.iq_ref)
        log_layout_scroll.addWidget(self.speed)
        log_layout_scroll.addWidget(self.target_speed)
        log_layout_scroll.addWidget(self.position)
        log_layout_scroll.addWidget(self.target_position)   
        
        log_layout.addWidget(Title_log)
        log_layout.addLayout(but_ton_layout)
        log_layout.addLayout(log_select_layout)
        log_layout.addWidget(log_scroll)
        ##########################################################
        main_layout.addWidget(self.control_group,1)
        main_layout.addWidget(self.mode_group,1)
        main_layout.addWidget(self.motor_group,1)
        main_layout.addWidget(self.log_group,1)
