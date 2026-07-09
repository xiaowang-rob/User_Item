from PyQt5.QtWidgets import QWidget, QHBoxLayout, QVBoxLayout
from PyQt5.QtCore import Qt
from siui.components.button import (
    SiCapsuleButton,
    SiFlatButton,
    SiToggleButtonRefactor,
)
from siui.components.combobox_ import SiCapsuleComboBox
from protocol import Midx

from siui.components.editbox import SiLabeledLineEdit
from siui.components.slider import SiSliderH
from PyQt5.QtGui import QColor


class ControlPage:
    def __init__(self, main_window):
        self.mw = main_window
        self.widget = main_window.ui.control_page

        main_layout = QVBoxLayout(self.widget)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(12)

        top_layout = QHBoxLayout()
        top_layout.setContentsMargins(0, 0, 0, 0)
        top_layout.setSpacing(12)

        bottom_layout = QHBoxLayout()
        bottom_layout.setContentsMargins(0, 0, 0, 0)
        bottom_layout.setSpacing(12)

        self.button_area = QWidget()
        self.button_area.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;  
        """)

        button_layout = QVBoxLayout(self.button_area)
        button_layout.setContentsMargins(6, 24, 6, 0)
        button_layout.setSpacing(12)

        self.start_wave_button = SiCapsuleButton(self.button_area)
        self.start_wave_button.setText("示波")
        self.start_wave_button.setValue("开始")

        self.wave_ch1 = SiCapsuleComboBox()
        self.wave_ch1.setTitle("CH1")
        self.wave_ch1.setTitleColor(QColor("#58E6D9"))
        self.wave_ch1.setEditable(False)
        for i in Midx.data_select:
            self.wave_ch1.addItem(i)

        self.wave_ch2 = SiCapsuleComboBox()
        self.wave_ch2.setTitle("CH2")
        self.wave_ch2.setTitleColor(QColor("#FFAA33"))
        self.wave_ch2.setEditable(False)
        for i in Midx.data_select:
            self.wave_ch2.addItem(i)

        self.wave_ch3 = SiCapsuleComboBox()
        self.wave_ch3.setTitle("CH3")
        self.wave_ch3.setTitleColor(QColor("#FF6B9D"))
        self.wave_ch3.setEditable(False)
        for i in Midx.data_select:
            self.wave_ch3.addItem(i)

        self.wave_ch4 = SiCapsuleComboBox()
        self.wave_ch4.setTitle("CH4")
        self.wave_ch4.setTitleColor(QColor("#88CC66"))
        self.wave_ch4.setEditable(False)
        for i in Midx.data_select:
            self.wave_ch4.addItem(i)

        self.wave_ch5 = SiCapsuleComboBox()
        self.wave_ch5.setTitle("CH5")
        self.wave_ch5.setTitleColor(QColor("#B886FF"))
        self.wave_ch5.setEditable(False)
        for i in Midx.data_select:
            self.wave_ch5.addItem(i)

        self.auto_x_switch = SiToggleButtonRefactor()
        self.auto_x_switch.setText("自动x轴")
        self.auto_x_switch.adjustSize()

        self.auto_y_switch = SiToggleButtonRefactor()
        self.auto_y_switch.setText("自动y轴")
        self.auto_y_switch.adjustSize()

        self.clear_wave_button = SiFlatButton()
        self.clear_wave_button.setText("清除波形")
        self.clear_wave_button.adjustSize()

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

        top_layout.addWidget(self.button_area, 1)
        top_layout.addWidget(self.wave_area, 10)

        self.control_area = QWidget()
        self.control_area.setStyleSheet("""
            background-color: #332E38;
            border-radius: 12px;  
        """)

        control_layout = QVBoxLayout(self.control_area)
        control_layout.setContentsMargins(0, 0, 0, 0)
        control_layout.setSpacing(12)


        
        self.control_target_show_1 = SiLabeledLineEdit()
        self.control_target_show_1.setTitle("控制目标")
        self.control_target_show_1.setEnabled(False)
        self.control_target_show_1.adjustSize()

        self.MIN_value_1 = SiLabeledLineEdit()
        self.MIN_value_1.setTitle("min")
        self.MIN_value_1.setReadOnly(True)
        self.MIN_value_1.setText(str(0))
        self.MIN_value_1.setAlignment(Qt.AlignCenter)
        self.MIN_value_1.adjustSize()

        self.MAX_value_1 = SiLabeledLineEdit()
        self.MAX_value_1.setTitle("max")
        self.MAX_value_1.setText(str(10))
        self.MAX_value_1.setAlignment(Qt.AlignCenter)
        self.MAX_value_1.adjustSize()

        self.value_slider_1 = SiSliderH()
        self.value_slider_1.setMinimumWidth(100)
        self.value_slider_1.setValueColor("#66D4CB", "#D16F51")
        self.value_slider_1.setMinimum(0)
        self.value_slider_1.setMaximum(1000)
        self.value_slider_1.setValue(0, move_to=False)
        self.value_slider_1.setText(str(0))

        self.target_value_1 = SiLabeledLineEdit()
        self.target_value_1.setTitle("目标值")
        self.target_value_1.setAlignment(Qt.AlignCenter)
        self.target_value_1.adjustSize()

        self.write_value_button_1 = SiFlatButton()
        self.write_value_button_1.setSvgIcon(
            self.mw.icon.get("ic_fluent_triangle_right_filled", "#DFDFDF")
        )
        self.write_value_button_1.adjustSize()


        control_layout_1 = QHBoxLayout(self.control_area)
        control_layout_1.setContentsMargins(0, 0, 0, 12)
        control_layout_1.setSpacing(12)

        control_layout_1.addWidget(self.control_target_show_1, 2)
        control_layout_1.addWidget(self.MIN_value_1, 1)
        control_layout_1.addWidget(self.value_slider_1, 10)
        control_layout_1.addWidget(self.MAX_value_1, 1)
        control_layout_1.addWidget(self.target_value_1, 2)
        control_layout_1.addWidget(self.write_value_button_1, 1)

        self.control_target_show_2 = SiLabeledLineEdit()
        self.control_target_show_2.setTitle("控制目标")
        self.control_target_show_2.setEnabled(False)
        self.control_target_show_2.adjustSize()

        self.MIN_value_2 = SiLabeledLineEdit()
        self.MIN_value_2.setTitle("min")
        self.MIN_value_2.setReadOnly(True)
        self.MIN_value_2.setText(str(0))
        self.MIN_value_2.setAlignment(Qt.AlignCenter)
        self.MIN_value_2.adjustSize()

        self.MAX_value_2 = SiLabeledLineEdit()
        self.MAX_value_2.setTitle("max")
        self.MAX_value_2.setText(str(10))
        self.MAX_value_2.setAlignment(Qt.AlignCenter)
        self.MAX_value_2.adjustSize()

        self.value_slider_2 = SiSliderH()
        self.value_slider_2.setMinimumWidth(100)
        self.value_slider_2.setValueColor("#66D4CB", "#D16F51")
        self.value_slider_2.setMinimum(0)
        self.value_slider_2.setMaximum(1000)
        self.value_slider_2.setValue(0, move_to=False)
        self.value_slider_2.setText(str(0))

        self.target_value_2 = SiLabeledLineEdit()
        self.target_value_2.setTitle("目标值")
        self.target_value_2.setAlignment(Qt.AlignCenter)
        self.target_value_2.adjustSize()

        self.write_value_button_2 = SiFlatButton()
        self.write_value_button_2.setSvgIcon(
            self.mw.icon.get("ic_fluent_triangle_right_filled", "#DFDFDF")
        )
        self.write_value_button_2.adjustSize()

        control_layout_2 = QHBoxLayout(self.control_area)
        control_layout_2.setContentsMargins(0, 0, 0, 12)
        control_layout_2.setSpacing(12)

        control_layout_2.addWidget(self.control_target_show_2, 2)
        control_layout_2.addWidget(self.MIN_value_2, 1)
        control_layout_2.addWidget(self.value_slider_2, 10)
        control_layout_2.addWidget(self.MAX_value_2, 1)
        control_layout_2.addWidget(self.target_value_2, 2)
        control_layout_2.addWidget(self.write_value_button_2, 1)

        control_layout.addLayout(control_layout_1)
        control_layout.addLayout(control_layout_2)

        main_layout.addLayout(top_layout, 8)
        main_layout.addLayout(control_layout, 1)
