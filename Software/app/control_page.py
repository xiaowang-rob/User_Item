from PyQt5.QtWidgets import QWidget, QHBoxLayout, QVBoxLayout
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QColor
from qfluentwidgets import (
    HeaderCardWidget, CardWidget, PushButton, ComboBox, LineEdit, Slider,
    ToggleButton, FluentIcon as FIF, BodyLabel
)
from protocol import Midx


class ControlPage(QWidget):
    """控制页面 —— 波形示波 + 目标值设定"""

    def __init__(self):
        super().__init__()
        self.setObjectName("control_page")

        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(12, 8, 12, 8)
        main_layout.setSpacing(8)

        # ── 上半部分 ──
        top_layout = QHBoxLayout()
        top_layout.setContentsMargins(0, 0, 0, 0)
        top_layout.setSpacing(12)

        # 左侧通道配置（无标题）
        self.channel_card = CardWidget()
        ch_container = QWidget()
        channel_layout = QVBoxLayout(ch_container)
        channel_layout.setContentsMargins(8, 4, 8, 8)
        channel_layout.setSpacing(32)

        self.start_wave_button = ToggleButton("开始示波")
        self.start_wave_button.setChecked(False)
        channel_layout.addWidget(self.start_wave_button)

        ch_colors = ["#58E6D9", "#FFAA33", "#FF6B9D", "#88CC66", "#B886FF"]
        ch_labels = ["CH1", "CH2", "CH3", "CH4", "CH5"]
        self.wave_ch = []
        for i in range(5):
            combo = ComboBox()
            combo.setMinimumWidth(80)
            combo.addItem("NONE")
            for item in Midx.data_select:
                combo.addItem(item)
            row = QHBoxLayout()
            row.setContentsMargins(0, 0, 0, 0)
            row.setSpacing(6)
            # 颜色指示条
            from PyQt5.QtWidgets import QFrame
            color_bar = QFrame()
            color_bar.setFixedSize(4, 20)
            color_bar.setStyleSheet(
                f"background:{ch_colors[i]}; border-radius:2px;"
            )
            row.addWidget(color_bar)
            lbl = BodyLabel(ch_labels[i])
            row.addWidget(lbl)
            row.addWidget(combo, 1)
            channel_layout.addLayout(row)
            self.wave_ch.append(combo)

        self.auto_x_switch = ToggleButton("自动X轴")
        self.auto_x_switch.setChecked(True)

        self.auto_y_switch = ToggleButton("自动Y轴")
        self.auto_y_switch.setChecked(True)

        self.clear_wave_button = PushButton()
        self.clear_wave_button.setText("清除波形")

        channel_layout.addWidget(self.auto_x_switch)
        channel_layout.addWidget(self.auto_y_switch)
        channel_layout.addWidget(self.clear_wave_button)
        channel_layout.addStretch()
        # 直接设置 CardWidget 的布局
        card_layout = QVBoxLayout(self.channel_card)
        card_layout.setContentsMargins(0, 0, 0, 0)
        card_layout.addWidget(ch_container)

        # 波形区域（直接放置波形控件，无标题）
        self.wave_area = QWidget()
        wave_area_layout = QVBoxLayout(self.wave_area)
        wave_area_layout.setContentsMargins(0, 0, 0, 0)

        top_layout.addWidget(self.channel_card, 1)

        # 波形区域用 CardWidget 包裹，无标题
        from qfluentwidgets import CardWidget as CW
        self.wave_card = CW()
        wave_inner = QVBoxLayout(self.wave_card)
        wave_inner.setContentsMargins(0, 0, 0, 0)
        wave_inner.addWidget(self.wave_area)
        top_layout.addWidget(self.wave_card, 6)

        # ── 下半部分：目标值控制 ──
        self.control_card = CardWidget()
        control_layout = QVBoxLayout(self.control_card)
        control_layout.setContentsMargins(8, 8, 8, 8)
        control_layout.setSpacing(8)

        # 控制目标 1
        ctrl_row1 = QHBoxLayout()
        ctrl_row1.setContentsMargins(0, 0, 0, 0)
        ctrl_row1.setSpacing(8)

        self.control_target_show_1 = LineEdit()
        self.control_target_show_1.setPlaceholderText("控制目标")
        self.control_target_show_1.setEnabled(False)
        self.control_target_show_1.setMaximumWidth(100)

        self.MIN_value_1 = LineEdit()
        self.MIN_value_1.setPlaceholderText("min")
        self.MIN_value_1.setText("0")
        self.MIN_value_1.setReadOnly(True)
        self.MIN_value_1.setFixedWidth(60)

        self.value_slider_1 = Slider(Qt.Horizontal)
        self.value_slider_1.setRange(0, 1000)
        self.value_slider_1.setValue(0)
        self.value_slider_1.setFixedHeight(24)

        self.MAX_value_1 = LineEdit()
        self.MAX_value_1.setPlaceholderText("max")
        self.MAX_value_1.setText("10")
        self.MAX_value_1.setFixedWidth(60)

        self.target_value_1 = LineEdit()
        self.target_value_1.setPlaceholderText("目标值")
        self.target_value_1.setText("0")
        self.target_value_1.setFixedWidth(80)

        self.write_value_button_1 = PushButton()
        self.write_value_button_1.setText("▶")
        self.write_value_button_1.setFixedWidth(36)

        ctrl_row1.addWidget(self.control_target_show_1, 1)
        ctrl_row1.addWidget(self.MIN_value_1,1)
        ctrl_row1.addWidget(self.value_slider_1, 4)
        ctrl_row1.addWidget(self.MAX_value_1,1)
        ctrl_row1.addWidget(self.target_value_1,2)
        ctrl_row1.addWidget(self.write_value_button_1,1)

        # 控制目标 2
        ctrl_row2 = QHBoxLayout()
        ctrl_row2.setContentsMargins(0, 0, 0, 0)
        ctrl_row2.setSpacing(8)

        self.control_target_show_2 = LineEdit()
        self.control_target_show_2.setPlaceholderText("控制目标")
        self.control_target_show_2.setEnabled(False)
        self.control_target_show_2.setMaximumWidth(100)


        self.MIN_value_2 = LineEdit()
        self.MIN_value_2.setPlaceholderText("min")
        self.MIN_value_2.setText("0")
        self.MIN_value_2.setReadOnly(True)
        self.MIN_value_2.setFixedWidth(60)

        self.value_slider_2 = Slider(Qt.Horizontal)
        self.value_slider_2.setRange(0, 1000)
        self.value_slider_2.setValue(0)
        self.value_slider_2.setFixedHeight(24)

        self.MAX_value_2 = LineEdit()
        self.MAX_value_2.setPlaceholderText("max")
        self.MAX_value_2.setText("10")
        self.MAX_value_2.setFixedWidth(60)

        self.target_value_2 = LineEdit()
        self.target_value_2.setPlaceholderText("目标值")
        self.target_value_2.setText("0")
        self.target_value_2.setFixedWidth(80)

        self.write_value_button_2 = PushButton()
        self.write_value_button_2.setText("▶")
        self.write_value_button_2.setFixedWidth(36)

        ctrl_row2.addWidget(self.control_target_show_2, 1)
        ctrl_row2.addWidget(self.MIN_value_2,1)
        ctrl_row2.addWidget(self.value_slider_2, 4)
        ctrl_row2.addWidget(self.MAX_value_2,1)
        ctrl_row2.addWidget(self.target_value_2)
        ctrl_row2.addWidget(self.write_value_button_2,1)

        control_layout.addLayout(ctrl_row1)
        control_layout.addLayout(ctrl_row2)

        # ── 组装 ──
        main_layout.addLayout(top_layout, 4)
        main_layout.addWidget(self.control_card, 1)

        self.wave_ch1 = self.wave_ch[0]
        self.wave_ch2 = self.wave_ch[1]
        self.wave_ch3 = self.wave_ch[2]
        self.wave_ch4 = self.wave_ch[3]
        self.wave_ch5 = self.wave_ch[4]
