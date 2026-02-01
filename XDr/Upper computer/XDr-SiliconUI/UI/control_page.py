from PyQt5.QtWidgets import QWidget, QHBoxLayout, QVBoxLayout, QScrollArea,QLineEdit
from PyQt5.QtCore import Qt
from siui.components.button import (
    SiCapsuleButton,
    SiCheckBox,
    SiCheckBoxRefactor,
    SiFlatButton,
    SiFlatButtonWithIndicator,
    SiLongPressButtonRefactor,
    SiOptionButton,
    SiProgressPushButton,
    SiPushButtonRefactor,
    SiRadioButton,
    SiRadioButtonR,
    SiRadioButtonWithAvatar,
    SiRadioButtonWithDescription,
    SiSwitchRefactor,
    SiToggleButtonRefactor,
)
from siui.components.editbox import SiCapsuleLineEdit
from siui.components.combobox_ import SiCapsuleComboBox
from .data_map import Midx
from siui.components.widgets.label import SiLabel
from siui.core import Si
from siui.gui import SiFont
from siui.core import SiGlobal, GlobalFont
from siui.components.editbox import SiCapsuleLineEdit, SiDoubleSpinBox, SiLabeledLineEdit, SiSpinBox
from siui.components.slider import SiSliderH

class ControlPage():
    def __init__(self, main_window):
        self.mw = main_window
        self.widget=main_window.ui.control_page

        main_layout =QVBoxLayout(self.widget)
        main_layout.setContentsMargins(0,0,0,0)
        main_layout.setSpacing(12)

        top_layout = QHBoxLayout()
        top_layout.setContentsMargins(0,0,0,0)
        top_layout.setSpacing(12)

        bottom_layout = QHBoxLayout()
        bottom_layout.setContentsMargins(0,0,0,0)
        bottom_layout.setSpacing(12)

        self.button_area = QWidget()
        self.button_area.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;  
        """)

        button_layout = QVBoxLayout(self.button_area)
        button_layout.setContentsMargins(6,24,6,0)
        button_layout.setSpacing(12)

        self.start_wave_button = SiCapsuleButton(self.button_area)
        self.start_wave_button.setText("示波")
        self.start_wave_button.setValue("开始")
        self.start_wave_button.clicked.connect(self.start_wave)

        self.wave_ch1=SiCapsuleComboBox()
        self.wave_ch1.setTitle("CH1")
        self.wave_ch1.setEditable(False)
        for i in Midx.data_select:
            self.wave_ch1.addItem(i)
        
        self.wave_ch2=SiCapsuleComboBox()
        self.wave_ch2.setTitle("CH2")
        self.wave_ch2.setEditable(False)
        for i in Midx.data_select:
            self.wave_ch2.addItem(i)

        self.wave_ch3=SiCapsuleComboBox()
        self.wave_ch3.setTitle("CH3")
        self.wave_ch3.setEditable(False)
        for i in Midx.data_select:
            self.wave_ch3.addItem(i)

        self.wave_ch4=SiCapsuleComboBox()
        self.wave_ch4.setTitle("CH4")
        self.wave_ch4.setEditable(False)
        for i in Midx.data_select:
            self.wave_ch4.addItem(i)

        self.wave_ch5=SiCapsuleComboBox()
        self.wave_ch5.setTitle("CH5")
        self.wave_ch5.setEditable(False)
        for i in Midx.data_select:
            self.wave_ch5.addItem(i)

        self.auto_x_switch=SiToggleButtonRefactor()
        self.auto_x_switch.setText("自动x轴")
        self.auto_x_switch.adjustSize()
        self.auto_x_switch.clicked.connect(self.auto_x_switch_clicked)

        self.auto_y_switch=SiToggleButtonRefactor()
        self.auto_y_switch.setText("自动y轴")
        self.auto_y_switch.adjustSize()
        self.auto_y_switch.clicked.connect(self.auto_y_switch_clicked)


        self.clear_wave_button=SiFlatButton()
        self.clear_wave_button.setText("清除波形")
        self.clear_wave_button.adjustSize()
        self.clear_wave_button.clicked.connect(self.clear_wave)

        button_layout.addWidget(self.start_wave_button)
        button_layout.addWidget(self.wave_ch1)
        button_layout.addWidget(self.wave_ch2)
        button_layout.addWidget(self.wave_ch3)
        button_layout.addWidget(self.wave_ch4)
        button_layout.addWidget(self.wave_ch5)
        button_layout.addWidget(self.auto_x_switch)
        button_layout.addWidget(self.auto_y_switch)
        button_layout.addWidget(self.clear_wave_button)

        self.wave_area = QWidget(self.widget)
        self.wave_area.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;  
        """)

        top_layout.addWidget(self.button_area,1)
        top_layout.addWidget(self.wave_area,10)

        self.control_area = QWidget()
        self.control_area.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;  
        """)

        self.control_target_show=SiLabeledLineEdit()
        self.control_target_show.setTitle("控制目标")
        self.control_target_show.setEnabled(False)
        self.control_target_show.adjustSize()

        self.MIN_value=SiLabeledLineEdit()
        self.MIN_value.setTitle("min")
        self.MIN_value.setText(str(0))
        self.MIN_value.setAlignment(Qt.AlignCenter)
        self.MIN_value.adjustSize() 
        self.MIN_value.textChanged.connect(self.MIN_value_changed)

        self.MAX_value=SiLabeledLineEdit()
        self.MAX_value.setTitle("max")
        self.MAX_value.setText(str(10))
        self.MAX_value.setAlignment(Qt.AlignCenter)
        self.MAX_value.adjustSize()
        self.MAX_value.textChanged.connect(self.MAX_value_changed)

        self.value_slider=SiSliderH()
        self.value_slider.setMinimumWidth(100)
        self.value_slider.setValueColor("#66D4CB","#D16F51")
        self.value_slider.setMinimum(0)
        self.value_slider.setMaximum(1000)
        self.value_slider.setValue(0, move_to=False)
        self.value_slider.setText(str(0))
        self.value_slider.valueChanged.connect(self.value_slider_changed)

        self.target_value=SiLabeledLineEdit()
        self.target_value.setTitle("目标值")
        self.target_value.setAlignment(Qt.AlignCenter)
        self.target_value.adjustSize()

        self.write_value_button=SiFlatButton()
        self.write_value_button.setSvgIcon(self.mw.icon.get("ic_fluent_triangle_right_filled","#DFDFDF"))
        self.write_value_button.adjustSize()
        self.write_value_button.clicked.connect(self.write_value)

        control_layout = QHBoxLayout(self.control_area)
        control_layout.setContentsMargins(0,0,0,12)
        control_layout.setSpacing(6)

        control_layout.addWidget(self.control_target_show,2)
        control_layout.addWidget(self.MIN_value,2)
        control_layout.addWidget(self.value_slider,10)
        control_layout.addWidget(self.MAX_value,2)
        control_layout.addWidget(self.target_value,2)
        control_layout.addWidget(self.write_value_button,1)


        main_layout.addLayout(top_layout,10)
        main_layout.addWidget(self.control_area,1)




    def start_wave(self):
        if self.start_wave_button.value()=="开始":
            self.start_wave_button.setValue("暂停")
            self.mw.wave.start()
        else:
            self.start_wave_button.setValue("开始")
            self.mw.wave.pause()
  

    def auto_x_switch_clicked(self,):
        self.mw.wave._sync_auto_x_button(self.auto_x_switch.isChecked())

    def auto_y_switch_clicked(self):
        self.mw.wave._sync_auto_y_button(self.auto_y_switch.isChecked())

    def clear_wave(self):
        self.mw.wave.clear()

    def is_number(s: str) -> bool:
        try:
            float(s)
            return True
        except (ValueError, TypeError):
            return False
    
    def MIN_value_changed(self, text):
        try:
            val=float(text)
            if val>float(self.MAX_value.text()):
                self.MIN_value.setText(str(0))
                return
            self.value_slider.setValue(0)
            self.target_value.setText(str(f"{val:.3g}"))
            
        except (ValueError, TypeError):
            self.MIN_value.setText(str(0))
    def MAX_value_changed(self, text):
        try:
            val=float(text)
            if val<float(self.MIN_value.text()):
                self.MAX_value.setText(str(10))
                return
            self.value_slider.setValue(0)
            self.target_value.setText(str(f"{val:.3g}"))
        except (ValueError, TypeError):
            self.MAX_value.setText(str(10))

    def value_slider_changed(self, value):
        val=float(value/1000)*(float(self.MAX_value.text())-float(self.MIN_value.text()))+float(self.MIN_value.text())
        val=float(f"{val:.3g}")
        self.value_slider.setText(str(val))
        self.target_value.setText(str(val))
        #todo:发送
    def write_value(self):
        min_val=float(self.MIN_value.text())
        max_val=float(self.MAX_value.text())
        target_val=float(self.target_value.text())
        if target_val<min_val:
            target_val=min_val
        if target_val>max_val:
            target_val=max_val
        self.value_slider.setValue(int((target_val-min_val)/(max_val-min_val)*1000))
        self.target_value.setText(str(f"{target_val:.3g}"))
        #todo:发送