from PyQt5.QtWidgets import QWidget, QHBoxLayout, QVBoxLayout
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

from .data_ui_map import Midx


title_W=80
drive_title_W=100
all_W=240


class ParameterPage():
    def __init__(self, main_window):
        self.mw = main_window
        self.widget=main_window.ui.parameter_page

        main_layout=QHBoxLayout(self.widget)
        main_layout.setContentsMargins(24,12,24,0)
        main_layout.setSpacing(36)

        self.control_group=QWidget(self.widget)
        self.control_group.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;  
        """)
        self.mode_group=QWidget(self.widget)
        self.mode_group.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;  
        """)
        self.motor_group=QWidget(self.widget)
        self.motor_group.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;  
        """)
        self.drive_group=QWidget(self.widget)
        self.drive_group.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;  
        """)

        #控制参数##################################################################
        control_layout=QVBoxLayout(self.control_group)
        control_layout.setContentsMargins(24,12,24,12)
        control_layout.setSpacing(12)

        Title_control=SiLabel()
        Title_control.setStyleSheet("color:#E5E5E5;")
        Title_control.setText("控制参数")
        Title_control.setFont(SiFont.tokenized(GlobalFont.M_BOLD))
        Title_control.setFixedHeight(26)
        Title_control.setAlignment(Qt.AlignBottom)
        Title_control.setSiliconWidgetFlag(Si.AdjustSizeOnTextChanged)        

        button_layout1=QHBoxLayout()
        button_layout1.setContentsMargins(0,0,0,0)
        button_layout1.setSpacing(12)

        self.all_read_button=SiPushButtonRefactor()
        self.all_read_button.setText("一键读取")
        self.all_read_button.adjustSize()
        self.all_read_button.setFixedHeight(30)
        button_layout1.addWidget(self.all_read_button)

        self.all_write_button=SiPushButtonRefactor()
        self.all_write_button.setText("一键写入")
        self.all_write_button.adjustSize()
        self.all_write_button.setFixedHeight(30)
        button_layout1.addWidget(self.all_write_button)

 
        button_layout2=QHBoxLayout()
        button_layout2.setContentsMargins(0,0,0,0)
        button_layout2.setSpacing(12)

        self.all_save_button=SiPushButtonRefactor()
        self.all_save_button.setText("一键保存")
        self.all_save_button.adjustSize()
        self.all_save_button.setFixedHeight(30)
        button_layout2.addWidget(self.all_save_button)

        self.all_erase_button=SiLongPressButtonRefactor()
        self.all_erase_button.setText("清除参数")
        self.all_erase_button.setToolTip("长按清除所有参数")
        self.all_erase_button.adjustSize()
        self.all_erase_button.setFixedHeight(30)
        button_layout2.addWidget(self.all_erase_button)

        #输入区域
        self.CAN_ID_input=SiCapsuleLineEdit()
        self.CAN_ID_input.resize(all_W,40)
        self.CAN_ID_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.CAN_ID_input.setTitleFixedWidth(title_W) 
        self.CAN_ID_input.setAlignment(Qt.AlignCenter) 
        self.CAN_ID_input.setTitle("CAN ID")
        self.CAN_ID_input.setText("0")

        # 电流环 P
        self.current_loop_P_input = SiCapsuleLineEdit()
        self.current_loop_P_input.resize(all_W, 40)
        self.current_loop_P_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.current_loop_P_input.setTitleFixedWidth(title_W)
        self.current_loop_P_input.setAlignment(Qt.AlignCenter)
        self.current_loop_P_input.setTitle("电流环P")
        self.current_loop_P_input.setText("0")

        # 电流环 I
        self.current_loop_I_input = SiCapsuleLineEdit()
        self.current_loop_I_input.resize(all_W, 40)
        self.current_loop_I_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.current_loop_I_input.setTitleFixedWidth(title_W)
        self.current_loop_I_input.setAlignment(Qt.AlignCenter)
        self.current_loop_I_input.setTitle("电流环I")
        self.current_loop_I_input.setText("0")

        # 弱磁环 P
        self.flux_weakening_P_input = SiCapsuleLineEdit()
        self.flux_weakening_P_input.resize(all_W, 40)
        self.flux_weakening_P_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.flux_weakening_P_input.setTitleFixedWidth(title_W)
        self.flux_weakening_P_input.setAlignment(Qt.AlignCenter)
        self.flux_weakening_P_input.setTitle("弱磁环P")
        self.flux_weakening_P_input.setText("0")

        # 弱磁环 I
        self.flux_weakening_I_input = SiCapsuleLineEdit()
        self.flux_weakening_I_input.resize(all_W, 40)
        self.flux_weakening_I_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.flux_weakening_I_input.setTitleFixedWidth(title_W)
        self.flux_weakening_I_input.setAlignment(Qt.AlignCenter)
        self.flux_weakening_I_input.setTitle("弱磁环I")
        self.flux_weakening_I_input.setText("0")

        # 速度环 P
        self.speed_loop_P_input = SiCapsuleLineEdit()
        self.speed_loop_P_input.resize(all_W, 40)
        self.speed_loop_P_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.speed_loop_P_input.setTitleFixedWidth(title_W)
        self.speed_loop_P_input.setAlignment(Qt.AlignCenter)
        self.speed_loop_P_input.setTitle("速度环P")
        self.speed_loop_P_input.setText("0")

        # 速度环 I
        self.speed_loop_I_input = SiCapsuleLineEdit()
        self.speed_loop_I_input.resize(all_W, 40)
        self.speed_loop_I_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.speed_loop_I_input.setTitleFixedWidth(title_W)
        self.speed_loop_I_input.setAlignment(Qt.AlignCenter)
        self.speed_loop_I_input.setTitle("速度环I")
        self.speed_loop_I_input.setText("0")

        # 位置环 P
        self.position_loop_P_input = SiCapsuleLineEdit()
        self.position_loop_P_input.resize(all_W, 40)
        self.position_loop_P_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.position_loop_P_input.setTitleFixedWidth(title_W)
        self.position_loop_P_input.setAlignment(Qt.AlignCenter)
        self.position_loop_P_input.setTitle("位置环P")
        self.position_loop_P_input.setText("0")

        # 位置环 I
        self.position_loop_I_input = SiCapsuleLineEdit()
        self.position_loop_I_input.resize(all_W, 40)
        self.position_loop_I_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.position_loop_I_input.setTitleFixedWidth(title_W)
        self.position_loop_I_input.setAlignment(Qt.AlignCenter)
        self.position_loop_I_input.setTitle("位置环I")
        self.position_loop_I_input.setText("0")

        # 位置环 D
        self.position_loop_D_input = SiCapsuleLineEdit()
        self.position_loop_D_input.resize(all_W, 40)
        self.position_loop_D_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.position_loop_D_input.setTitleFixedWidth(title_W)
        self.position_loop_D_input.setAlignment(Qt.AlignCenter)
        self.position_loop_D_input.setTitle("位置环D")
        self.position_loop_D_input.setText("0")

        control_layout.addWidget(Title_control)
        control_layout.addLayout(button_layout1)
        control_layout.addLayout(button_layout2)
        control_layout.addWidget(self.CAN_ID_input)
        control_layout.addWidget(self.current_loop_P_input)
        control_layout.addWidget(self.current_loop_I_input)
        control_layout.addWidget(self.flux_weakening_P_input)
        control_layout.addWidget(self.flux_weakening_I_input)
        control_layout.addWidget(self.speed_loop_P_input)
        control_layout.addWidget(self.speed_loop_I_input)
        control_layout.addWidget(self.position_loop_P_input)
        control_layout.addWidget(self.position_loop_I_input)
        control_layout.addWidget(self.position_loop_D_input)
        control_layout.setAlignment(Qt.AlignTop)

    
        #模式参数##############################################
        mode_layout=QVBoxLayout(self.mode_group)
        mode_layout.setContentsMargins(24,12,24,12)
        mode_layout.setSpacing(12)

        Title_mode=SiLabel()
        Title_mode.setStyleSheet("color:#E5E5E5;")
        Title_mode.setText("模式设置")
        Title_mode.setFont(SiFont.tokenized(GlobalFont.M_BOLD))
        Title_mode.setFixedHeight(26)
        Title_mode.setAlignment(Qt.AlignBottom)
        Title_mode.setSiliconWidgetFlag(Si.AdjustSizeOnTextChanged)        

        self.sensormode_input=SiCapsuleComboBox()
        self.sensormode_input.setTitle("感应模式")
        self.sensormode_input.setEditable(False)
        self.sensormode_input.setFixedHeight(36)
        self.sensormode_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.sensormode_input.setTitleFixedWidth(title_W)
        for i in Midx.sensor_mode:
            self.sensormode_input.addItem(i)

        self.runmode_input=SiCapsuleComboBox()
        self.runmode_input.setTitle("运行模式")
        self.runmode_input.setEditable(False)
        self.runmode_input.setFixedHeight(36)
        self.runmode_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.runmode_input.setTitleFixedWidth(title_W)
        for i in Midx.run_mode:
            self.runmode_input.addItem(i)

        self.can_mode_input=SiCapsuleComboBox()
        self.can_mode_input.setTitle("CAN模式")
        self.can_mode_input.setEditable(False)
        self.can_mode_input.setFixedHeight(36)
        self.can_mode_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.can_mode_input.setTitleFixedWidth(title_W)
        for i in Midx.can_mode:
            self.can_mode_input.addItem(i)

        self.weakmag_mode_input=SiCapsuleComboBox()
        self.weakmag_mode_input.setTitle("弱磁控制")
        self.weakmag_mode_input.setEditable(False)
        self.weakmag_mode_input.setFixedHeight(36)
        self.weakmag_mode_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.weakmag_mode_input.setTitleFixedWidth(title_W)
        for i in Midx.weakmag_mode:
            self.weakmag_mode_input.addItem(i)

        self.vaguePID_input=SiCapsuleComboBox()
        self.vaguePID_input.setTitle("模糊PID")
        self.vaguePID_input.setFixedHeight(36)
        self.vaguePID_input.setEditable(False)
        self.vaguePID_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.vaguePID_input.setTitleFixedWidth(title_W)
        for i in Midx.vague_PID_mode:
            self.vaguePID_input.addItem(i)

        self.PVT_mode_input=SiCapsuleComboBox()
        self.PVT_mode_input.setTitle("PVT模式")
        self.PVT_mode_input.setEditable(False)
        self.PVT_mode_input.setFixedHeight(36)
        self.PVT_mode_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.PVT_mode_input.setTitleFixedWidth(title_W)
        for i in Midx.pvt_mode:
            self.PVT_mode_input.addItem(i)

        self.TRAJ_mode_input=SiCapsuleComboBox()
        self.TRAJ_mode_input.setTitle("轨迹模式")
        self.TRAJ_mode_input.setEditable(False)
        self.TRAJ_mode_input.setFixedHeight(36)
        self.TRAJ_mode_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.TRAJ_mode_input.setTitleFixedWidth(title_W)
        for i in Midx.traj_type:
            self.TRAJ_mode_input.addItem(i)



        mode_layout.addWidget(Title_mode)
        mode_layout.addWidget(self.sensormode_input)
        mode_layout.addWidget(self.runmode_input)
        mode_layout.addWidget(self.can_mode_input)
        mode_layout.addWidget(self.weakmag_mode_input)
        mode_layout.addWidget(self.vaguePID_input)
        mode_layout.addWidget(self.PVT_mode_input)
        mode_layout.addWidget(self.TRAJ_mode_input)
        mode_layout.setAlignment(Qt.AlignTop)

        #电机参数#########################################################
        motor_layout=QVBoxLayout(self.motor_group)
        motor_layout.setContentsMargins(24,12,24,12)
        motor_layout.setSpacing(12)

        Title_motor=SiLabel()
        Title_motor.setStyleSheet("color:#E5E5E5;")
        Title_motor.setText("电机参数")
        Title_motor.setFont(SiFont.tokenized(GlobalFont.M_BOLD))
        Title_motor.setFixedHeight(26)
        Title_motor.setAlignment(Qt.AlignBottom)
        Title_motor.setSiliconWidgetFlag(Si.AdjustSizeOnTextChanged)        


        self.offsetangle_input=SiCapsuleLineEdit()
        self.offsetangle_input.resize(all_W,40)
        self.offsetangle_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.offsetangle_input.setTitleFixedWidth(title_W) 
        self.offsetangle_input.setAlignment(Qt.AlignCenter) 
        self.offsetangle_input.setTitle("偏转角度/°")
        self.offsetangle_input.setText("0")

        self.motor_polepairs_input=SiCapsuleLineEdit()
        self.motor_polepairs_input.resize(all_W,40)
        self.motor_polepairs_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.motor_polepairs_input.setTitleFixedWidth(title_W) 
        self.motor_polepairs_input.setAlignment(Qt.AlignCenter) 
        self.motor_polepairs_input.setTitle("电机极数")
        self.motor_polepairs_input.setText("7")

        self.motor_KV_input=SiCapsuleLineEdit()
        self.motor_KV_input.resize(all_W,40)
        self.motor_KV_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.motor_KV_input.setTitleFixedWidth(title_W) 
        self.motor_KV_input.setAlignment(Qt.AlignCenter) 
        self.motor_KV_input.setTitle("电机KV")
        self.motor_KV_input.setText("0")

        self.motor_resistance_input=SiCapsuleLineEdit()
        self.motor_resistance_input.resize(all_W,40)
        self.motor_resistance_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.motor_resistance_input.setTitleFixedWidth(title_W) 
        self.motor_resistance_input.setAlignment(Qt.AlignCenter) 
        self.motor_resistance_input.setTitle("相电阻")
        self.motor_resistance_input.setText("0.001")

        self.motor_Ld_input=SiCapsuleLineEdit()
        self.motor_Ld_input.resize(all_W,40)
        self.motor_Ld_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.motor_Ld_input.setTitleFixedWidth(title_W) 
        self.motor_Ld_input.setAlignment(Qt.AlignCenter) 
        self.motor_Ld_input.setTitle("d轴电感")
        self.motor_Ld_input.setText("0.001")

        self.motor_Lq_input=SiCapsuleLineEdit()
        self.motor_Lq_input.resize(all_W,40)
        self.motor_Lq_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.motor_Lq_input.setTitleFixedWidth(title_W) 
        self.motor_Lq_input.setAlignment(Qt.AlignCenter) 
        self.motor_Lq_input.setTitle("q轴电感")
        self.motor_Lq_input.setText("0.001")



        self.motor_psif_input=SiCapsuleLineEdit()
        self.motor_psif_input.resize(all_W,40)
        self.motor_psif_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.motor_psif_input.setTitleFixedWidth(title_W) 
        self.motor_psif_input.setAlignment(Qt.AlignCenter) 
        self.motor_psif_input.setTitle("磁链")
        self.motor_psif_input.setText("0.001")

        self.motor_Ke_input=SiCapsuleLineEdit()
        self.motor_Ke_input.resize(all_W,40)
        self.motor_Ke_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.motor_Ke_input.setTitleFixedWidth(title_W) 
        self.motor_Ke_input.setAlignment(Qt.AlignCenter) 
        self.motor_Ke_input.setTitle("反电动势")
        self.motor_Ke_input.setText("0.001")

        self.motor_J_input=SiCapsuleLineEdit()
        self.motor_J_input.resize(all_W,40)
        self.motor_J_input.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.motor_J_input.setTitleFixedWidth(title_W) 
        self.motor_J_input.setAlignment(Qt.AlignCenter) 
        self.motor_J_input.setTitle("转动惯量")
        self.motor_J_input.setText("0.001")

        self.motor_B_input=SiCapsuleLineEdit()
        self.motor_B_input.resize(all_W,40)
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

        #驱动器参数################################################
        drive_layout=QVBoxLayout(self.drive_group)
        drive_layout.setContentsMargins(24,12,24,12)

        Title_drive=SiLabel()
        Title_drive.setStyleSheet("color:#E5E5E5;")
        Title_drive.setText("驱动器参数")
        Title_drive.setFont(SiFont.tokenized(GlobalFont.M_BOLD))
        Title_drive.setFixedHeight(26)
        Title_drive.setAlignment(Qt.AlignBottom)
        Title_drive.setSiliconWidgetFlag(Si.AdjustSizeOnTextChanged)        

        self.f_pwm=SiCapsuleLineEdit()
        self.f_pwm.setReadOnly(True)
        self.f_pwm.resize(all_W,40)
        self.f_pwm.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.f_pwm.setTitleFixedWidth(drive_title_W) 
        self.f_pwm.setAlignment(Qt.AlignCenter) 
        self.f_pwm.setTitle("PWM频率/Hz")
        self.f_pwm.setText("20000")

        self.f_current_loop=SiCapsuleLineEdit()
        self.f_current_loop.setReadOnly(True)
        self.f_current_loop.resize(all_W,40)
        self.f_current_loop.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.f_current_loop.setTitleFixedWidth(drive_title_W) 
        self.f_current_loop.setAlignment(Qt.AlignCenter) 
        self.f_current_loop.setTitle("电流环频率/Hz")
        self.f_current_loop.setText("20000")

        self.f_speed_loop=SiCapsuleLineEdit()
        self.f_speed_loop.setReadOnly(True)
        self.f_speed_loop.resize(all_W,40)
        self.f_speed_loop.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.f_speed_loop.setTitleFixedWidth(drive_title_W) 
        self.f_speed_loop.setAlignment(Qt.AlignCenter) 
        self.f_speed_loop.setTitle("速度环频率/Hz")
        self.f_speed_loop.setText("20000")

        self.f_position_loop=SiCapsuleLineEdit()
        self.f_position_loop.setReadOnly(True)
        self.f_position_loop.resize(all_W,40)
        self.f_position_loop.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.f_position_loop.setTitleFixedWidth(drive_title_W) 
        self.f_position_loop.setAlignment(Qt.AlignCenter) 
        self.f_position_loop.setTitle("位置环频率/Hz")
        self.f_position_loop.setText("20000")

        self.freq_current_loop=SiCapsuleLineEdit()
        self.freq_current_loop.resize(all_W,40)
        self.freq_current_loop.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.freq_current_loop.setTitleFixedWidth(drive_title_W) 
        self.freq_current_loop.setAlignment(Qt.AlignCenter) 
        self.freq_current_loop.setTitle("电流环分频系数")
        self.freq_current_loop.setText("1")

        self.freq_speed_loop=SiCapsuleLineEdit()
        self.freq_speed_loop.resize(all_W,40)
        self.freq_speed_loop.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.freq_speed_loop.setTitleFixedWidth(drive_title_W) 
        self.freq_speed_loop.setAlignment(Qt.AlignCenter) 
        self.freq_speed_loop.setTitle("速度环分频系数")
        self.freq_speed_loop.setText("1")

        self.freq_position_loop=SiCapsuleLineEdit()
        self.freq_position_loop.resize(all_W,40)
        self.freq_position_loop.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.freq_position_loop.setTitleFixedWidth(drive_title_W) 
        self.freq_position_loop.setAlignment(Qt.AlignCenter) 
        self.freq_position_loop.setTitle("位置环分频系数")
        self.freq_position_loop.setText("1")

        self.limit_current=SiCapsuleLineEdit()
        self.limit_current.resize(all_W,40)
        self.limit_current.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.limit_current.setTitleFixedWidth(drive_title_W) 
        self.limit_current.setAlignment(Qt.AlignCenter) 
        self.limit_current.setTitle("电流限幅/A")
        self.limit_current.setText("50")

        self.limit_speed=SiCapsuleLineEdit()
        self.limit_speed.resize(all_W,40)
        self.limit_speed.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.limit_speed.setTitleFixedWidth(drive_title_W) 
        self.limit_speed.setAlignment(Qt.AlignCenter) 
        self.limit_speed.setTitle("速度限幅/rpm")
        self.limit_speed.setText("1000")

        self.min_position=SiCapsuleLineEdit()
        self.min_position.resize(all_W,40)
        self.min_position.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.min_position.setTitleFixedWidth(drive_title_W) 
        self.min_position.setAlignment(Qt.AlignCenter) 
        self.min_position.setTitle("最小位置/°")
        self.min_position.setText("-10000")

        self.max_position=SiCapsuleLineEdit()
        self.max_position.resize(all_W,40)
        self.max_position.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.max_position.setTitleFixedWidth(drive_title_W) 
        self.max_position.setAlignment(Qt.AlignCenter) 
        self.max_position.setTitle("最大位置/°")
        self.max_position.setText("10000")

        self.tolerance_time=SiCapsuleLineEdit()
        self.tolerance_time.resize(all_W,40)
        self.tolerance_time.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.tolerance_time.setTitleFixedWidth(drive_title_W) 
        self.tolerance_time.setAlignment(Qt.AlignCenter) 
        self.tolerance_time.setTitle("容忍时间/ms")
        self.tolerance_time.setText("1")

        self.tolerance_voltage=SiCapsuleLineEdit()
        self.tolerance_voltage.resize(all_W,40)
        self.tolerance_voltage.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.tolerance_voltage.setTitleFixedWidth(drive_title_W) 
        self.tolerance_voltage.setAlignment(Qt.AlignCenter) 
        self.tolerance_voltage.setTitle("电压容忍度")
        self.tolerance_voltage.setText("1.0")

        self.tolerance_current=SiCapsuleLineEdit()
        self.tolerance_current.resize(all_W,40)
        self.tolerance_current.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.tolerance_current.setTitleFixedWidth(drive_title_W) 
        self.tolerance_current.setAlignment(Qt.AlignCenter) 
        self.tolerance_current.setTitle("电流容忍度")
        self.tolerance_current.setText("1.1")

        self.tolerance_speed=SiCapsuleLineEdit()
        self.tolerance_speed.resize(all_W,40)
        self.tolerance_speed.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.tolerance_speed.setTitleFixedWidth(drive_title_W) 
        self.tolerance_speed.setAlignment(Qt.AlignCenter) 
        self.tolerance_speed.setTitle("速度容忍度")
        self.tolerance_speed.setText("1.1")

        self.tolerance_position=SiCapsuleLineEdit()
        self.tolerance_position.resize(all_W,40)
        self.tolerance_position.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.tolerance_position.setTitleFixedWidth(drive_title_W) 
        self.tolerance_position.setAlignment(Qt.AlignCenter) 
        self.tolerance_position.setTitle("位置容忍度")
        self.tolerance_position.setText("1.1")

        self.traj_max_rate=SiCapsuleLineEdit()
        self.traj_max_rate.resize(all_W,40)
        self.traj_max_rate.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.traj_max_rate.setTitleFixedWidth(drive_title_W) 
        self.traj_max_rate.setAlignment(Qt.AlignCenter) 
        self.traj_max_rate.setTitle("最大变化率")
        self.traj_max_rate.setText("100")

        self.traj_max_acc=SiCapsuleLineEdit()
        self.traj_max_acc.resize(all_W,40)
        self.traj_max_acc.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.traj_max_acc.setTitleFixedWidth(drive_title_W) 
        self.traj_max_acc.setAlignment(Qt.AlignCenter) 
        self.traj_max_acc.setTitle("最大加速度")
        self.traj_max_acc.setText("50")

        self.traj_max_jerk=SiCapsuleLineEdit()
        self.traj_max_jerk.resize(all_W,40)
        self.traj_max_jerk.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.traj_max_jerk.setTitleFixedWidth(drive_title_W) 
        self.traj_max_jerk.setAlignment(Qt.AlignCenter) 
        self.traj_max_jerk.setTitle("最大加加速度")
        self.traj_max_jerk.setText("200")

        self.traj_tolerance=SiCapsuleLineEdit()
        self.traj_tolerance.resize(all_W,40)
        self.traj_tolerance.setTitleWidthMode(SiCapsuleLineEdit.TitleWidthMode.Fixed)
        self.traj_tolerance.setTitleFixedWidth(drive_title_W) 
        self.traj_tolerance.setAlignment(Qt.AlignCenter) 
        self.traj_tolerance.setTitle("容差")
        self.traj_tolerance.setText("0.01")

        drive_layout.addWidget(Title_drive)
        drive_layout.addWidget(self.f_pwm)
        drive_layout.addWidget(self.f_current_loop)
        drive_layout.addWidget(self.f_speed_loop)
        drive_layout.addWidget(self.f_position_loop)
        drive_layout.addWidget(self.freq_current_loop)
        drive_layout.addWidget(self.freq_speed_loop)
        drive_layout.addWidget(self.freq_position_loop)
        drive_layout.addWidget(self.limit_current)
        drive_layout.addWidget(self.limit_speed)
        drive_layout.addWidget(self.min_position)
        drive_layout.addWidget(self.max_position)
        drive_layout.addWidget(self.tolerance_time)
        drive_layout.addWidget(self.tolerance_voltage)
        drive_layout.addWidget(self.tolerance_current)
        drive_layout.addWidget(self.tolerance_speed)
        drive_layout.addWidget(self.tolerance_position)
        drive_layout.addWidget(self.traj_max_rate)
        drive_layout.addWidget(self.traj_max_acc)
        drive_layout.addWidget(self.traj_max_jerk)
        drive_layout.addWidget(self.traj_tolerance)
        
        drive_layout.setAlignment(Qt.AlignTop)

##########################################################
        main_layout.addWidget(self.control_group)
        main_layout.addWidget(self.mode_group)
        main_layout.addWidget(self.motor_group)
        main_layout.addWidget(self.drive_group)



