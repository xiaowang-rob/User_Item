import pyqtgraph as pg
from PyQt5.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QLabel, QSpinBox, QPushButton, QCheckBox
from PyQt5.QtCore import Qt, QTimer
from PyQt5.QtGui import QFont, QColor, QPalette, QBrush
import numpy as np

class WaveformWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        
        # 设置深色主题
        self.setup_dark_theme()
        
        # 定义属性
        self.max_points = 1000
        self.colors = [
            (255, 0, 0),      # 红色
            (0, 255, 0),      # 绿色
            (0, 0, 255),      # 蓝色
            (255, 255, 0),    # 黄色
            (255, 0, 255)     # 紫色
        ]
        self.wave_data = [[] for _ in range(5)]
        self.x_min = 0
        self.x_max = 100
        self.is_running = True
        
        # 初始化UI
        self.init_ui()
        
        # 创建曲线
        self.curves = []
        for i in range(5):
            curve = self.plot_widget.plot(pen=pg.mkPen(color=self.colors[i], width=2))
            self.curves.append(curve)
        
        # 创建当前值标签 (每个波形末端一个标签)
        self.value_labels = []
        font = QFont()
        font.setPointSize(9)
        
        for i in range(5):
            # 创建跟随波形的标签 - 白色文字，深灰背景
            label = pg.TextItem(
                text=f"Ch{i+1}: N/A",
                color=(255, 255, 255),  # 白色文字
                border=pg.mkPen(80, 80, 80, 200),  # 深灰边框
                fill=pg.mkBrush(35, 35, 38, 220),  # #39373F背景，半透明
                anchor=(0.5, 0.5)  # 中心锚点
            )
            label.setFont(font)
            label.setZValue(10)  # 确保标签在波形上方
            self.plot_widget.addItem(label)
            self.value_labels.append(label)
        
        # 自动调节计时器
        self.auto_scale_timer = QTimer()
        self.auto_scale_timer.timeout.connect(self.auto_scale_axes)
        self.auto_scale_timer.start(100)
    
    def setup_dark_theme(self):
        """设置深色主题样式表"""
        dark_palette = QPalette()
        dark_palette.setColor(QPalette.Window, QColor(53, 53, 53))
        dark_palette.setColor(QPalette.WindowText, Qt.white)
        dark_palette.setColor(QPalette.Base, QColor(35, 35, 38))
        dark_palette.setColor(QPalette.AlternateBase, QColor(53, 53, 53))
        dark_palette.setColor(QPalette.ToolTipBase, Qt.white)
        dark_palette.setColor(QPalette.ToolTipText, Qt.white)
        dark_palette.setColor(QPalette.Text, Qt.white)
        dark_palette.setColor(QPalette.Button, QColor(35, 35, 38))
        dark_palette.setColor(QPalette.ButtonText, Qt.white)
        dark_palette.setColor(QPalette.BrightText, Qt.red)
        dark_palette.setColor(QPalette.Link, QColor(42, 130, 218))
        dark_palette.setColor(QPalette.Highlight, QColor(42, 130, 218))
        dark_palette.setColor(QPalette.HighlightedText, Qt.black)
        
        self.setPalette(dark_palette)
        self.setStyleSheet("""
            QWidget {
                background-color: #39373F;
                color: #ffffff;
                font-family: 'Segoe UI', Arial, sans-serif;
            }
            QSpinBox {
                background-color: #39373F;
                border: 1px solid #555555;
                color: #ffffff;
                padding: 2px;
                selection-background-color: #2a82da;
            }
            QSpinBox::up-button, QSpinBox::down-button {
                subcontrol-origin: border;
                width: 12px;
                border-left: 1px solid #555555;
                background-color: #2a2a2e;
            }
            QCheckBox {
                spacing: 5px;
            }
            QCheckBox::indicator {
                width: 13px;
                height: 13px;
                border: 1px solid #555555;
                background-color: #39373F;
            }
            QCheckBox::indicator:checked {
                background-color: #2a82da;
                border: 1px solid #2a82da;
            }
            QPushButton {
                background-color: #2a2a2e;
                border: 1px solid #555555;
                color: #ffffff;
                padding: 3px 8px;
                min-width: 60px;
                border-radius: 3px;
            }
            QPushButton:hover {
                background-color: #3a3a3e;
                border: 1px solid #777777;
            }
            QPushButton:pressed {
                background-color: #1a1a1e;
                border: 1px solid #333333;
            }
            QLabel {
                color: #ffffff;
            }
        """)
    
    def init_ui(self):
        main_layout = QVBoxLayout()
        main_layout.setContentsMargins(8, 8, 8, 8)
        main_layout.setSpacing(8)
        
        # 绘图区域 - 深色背景
        self.plot_widget = pg.PlotWidget(background='#39373F')
        self.plot_widget.setLabel('left', 'Y轴', color='#ffffff', size='10pt')
        self.plot_widget.setLabel('bottom', 'X轴', color='#ffffff', size='10pt')
        
        # 设置坐标轴样式
        styles = {'color': '#aaaaaa', 'font-size': '9pt'}
        self.plot_widget.getAxis('left').setTextPen('#aaaaaa')
        self.plot_widget.getAxis('bottom').setTextPen('#aaaaaa')
        self.plot_widget.getAxis('left').setPen('#777777')
        self.plot_widget.getAxis('bottom').setPen('#777777')
        self.plot_widget.getAxis('left').setStyle(tickLength=-10)
        self.plot_widget.getAxis('bottom').setStyle(tickLength=-10)
        self.plot_widget.showGrid(x=True, y=True, alpha=0.3)
        
        # 控制面板 (所有控制元素放在同一行)
        control_layout = QHBoxLayout()
        control_layout.setContentsMargins(0, 4, 0, 0)
        control_layout.setSpacing(10)
        
        # Y轴范围控制
        y_range_layout = QHBoxLayout()
        y_range_layout.setContentsMargins(0, 0, 0, 0)
        y_range_layout.setSpacing(4)
        y_range_label = QLabel("Y轴:")
        y_range_label.setStyleSheet("color: #aaaaaa; font-weight: bold;")
        
        self.y_min_spin = QSpinBox()
        self.y_min_spin.setRange(-10000, 10000)
        self.y_min_spin.setValue(-10)
        self.y_min_spin.setFixedWidth(60)
        self.y_min_spin.valueChanged.connect(self.manual_y_range_changed)
        
        self.y_max_spin = QSpinBox()
        self.y_max_spin.setRange(-10000, 10000)
        self.y_max_spin.setValue(10)
        self.y_max_spin.setFixedWidth(60)
        self.y_max_spin.valueChanged.connect(self.manual_y_range_changed)
        
        self.auto_y_check = QCheckBox("自动")
        self.auto_y_check.setChecked(True)
        self.auto_y_check.setStyleSheet("color: #cccccc;")
        
        y_range_layout.addWidget(y_range_label)
        y_range_layout.addWidget(self.y_min_spin)
        y_range_layout.addWidget(QLabel("-"))
        y_range_layout.addWidget(self.y_max_spin)
        y_range_layout.addWidget(self.auto_y_check)
        
        # X轴范围控制
        x_range_layout = QHBoxLayout()
        x_range_layout.setContentsMargins(0, 0, 0, 0)
        x_range_layout.setSpacing(4)
        x_range_label = QLabel("X轴:")
        x_range_label.setStyleSheet("color: #aaaaaa; font-weight: bold;")
        
        self.x_min_spin = QSpinBox()
        self.x_min_spin.setRange(0, 10000)
        self.x_min_spin.setValue(0)
        self.x_min_spin.setFixedWidth(60)
        self.x_min_spin.valueChanged.connect(self.manual_x_range_changed)
        
        self.x_max_spin = QSpinBox()
        self.x_max_spin.setRange(0, 10000)
        self.x_max_spin.setValue(100)
        self.x_max_spin.setFixedWidth(60)
        self.x_max_spin.valueChanged.connect(self.manual_x_range_changed)
        
        self.auto_x_check = QCheckBox("自动")
        self.auto_x_check.setChecked(True)
        self.auto_x_check.setStyleSheet("color: #cccccc;")
        
        x_range_layout.addWidget(x_range_label)
        x_range_layout.addWidget(self.x_min_spin)
        x_range_layout.addWidget(QLabel("-"))
        x_range_layout.addWidget(self.x_max_spin)
        x_range_layout.addWidget(self.auto_x_check)
        
        # 清除按钮
        clear_btn = QPushButton("清除")
        clear_btn.setStyleSheet("""
            QPushButton {
                background-color: #d32f2f;
                border: 1px solid #b71c1c;
                color: white;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #f44336;
                border: 1px solid #e53935;
            }
            QPushButton:pressed {
                background-color: #b71c1c;
            }
        """)
        clear_btn.setFixedWidth(70)
        clear_btn.clicked.connect(self.clear_waveforms)
        
        # 添加所有元素到控制行
        control_layout.addLayout(y_range_layout)
        control_layout.addLayout(x_range_layout)
        control_layout.addWidget(clear_btn)
        control_layout.addStretch()  # 右侧填充
        
        # 添加到主布局
        main_layout.addWidget(self.plot_widget, 1)
        main_layout.addLayout(control_layout, 0)
        
        self.setLayout(main_layout)
    
    def manual_y_range_changed(self):
        if self.auto_y_check.isChecked():
            self.auto_y_check.setChecked(False)
        self.update_y_range()
    
    def manual_x_range_changed(self):
        if self.auto_x_check.isChecked():
            self.auto_x_check.setChecked(False)
        self.update_x_range()
    
    def update_y_range(self):
        if not self.auto_y_check.isChecked():
            y_min = self.y_min_spin.value()
            y_max = self.y_max_spin.value()
            if y_max > y_min:
                self.plot_widget.setYRange(y_min, y_max)
    
    def update_x_range(self):
        if not self.auto_x_check.isChecked():
            x_min = self.x_min_spin.value()
            x_max = self.x_max_spin.value()
            if x_max > x_min:
                self.x_min = x_min
                self.x_max = x_max
                self.plot_widget.setXRange(x_min, x_max)
    
    def auto_scale_axes(self):
        if not self.is_running:
            return
            
        if self.auto_y_check.isChecked():
            self.auto_scale_y_axis()
        
        if self.auto_x_check.isChecked():
            self.auto_scale_x_axis()
    
    def auto_scale_y_axis(self):
        all_data = []
        for wave in self.wave_data:
            if len(wave) > 0:
                all_data.extend(wave)
        
        if len(all_data) == 0:
            return
        
        min_val = min(all_data)
        max_val = max(all_data)
        
        margin = (max_val - min_val) * 0.1 if max_val != min_val else 1
        new_min = min_val - margin
        new_max = max_val + margin
        
        if new_max - new_min < 0.1:
            new_min = min_val - 0.05
            new_max = max_val + 0.05
        
        self.plot_widget.setYRange(new_min, new_max)
        self.y_min_spin.blockSignals(True)
        self.y_max_spin.blockSignals(True)
        self.y_min_spin.setValue(int(new_min))
        self.y_max_spin.setValue(int(new_max))
        self.y_min_spin.blockSignals(False)
        self.y_max_spin.blockSignals(False)
    
    def auto_scale_x_axis(self):
        max_length = 0
        for wave in self.wave_data:
            if len(wave) > max_length:
                max_length = len(wave)
        
        if max_length == 0:
            return
        
        target_width = 200
        current_width = self.x_max - self.x_min
        visible_points = min(max_length, self.x_max) - max(0, self.x_min)
        
        if visible_points > target_width * 1.5:
            new_width = max(100, current_width * 0.8)
        elif visible_points < target_width * 0.5:
            new_width = min(2000, current_width * 1.2)
        else:
            new_width = current_width
        
        new_width = max(100, new_width)
        new_max = max_length + 10
        new_min = new_max - new_width
        
        self.x_min = max(0, new_min)
        self.x_max = new_max
        
        self.plot_widget.setXRange(self.x_min, self.x_max)
        
        self.x_min_spin.blockSignals(True)
        self.x_max_spin.blockSignals(True)
        self.x_min_spin.setValue(int(self.x_min))
        self.x_max_spin.setValue(int(self.x_max))
        self.x_min_spin.blockSignals(False)
        self.x_max_spin.blockSignals(False)
    
    def add_waveform_data(self, channel, data):
        if 0 <= channel < 5 and self.is_running:
            if isinstance(data, (list, tuple, np.ndarray)):
                self.wave_data[channel].extend(data)
            else:
                self.wave_data[channel].append(data)
            
            if len(self.wave_data[channel]) > self.max_points:
                self.wave_data[channel] = self.wave_data[channel][-self.max_points:]
            
            self.update_plot()
    
    def update_plot(self):
        if not self.is_running:
            return
            
        # 更新波形
        for i in range(5):
            if len(self.wave_data[i]) > 0:
                y_data = np.array(self.wave_data[i])
                x_data = np.arange(len(y_data))
                
                # 更新曲线
                self.curves[i].setData(x_data, y_data)
                
                # 计算标签位置 - 放在波形最末端
                last_x = x_data[-1]
                last_y = y_data[-1]
                
                # 添加一些偏移，避免标签直接贴在点上
                offset_x = 8  # 右侧偏移
                offset_y = 0  # 无垂直偏移
                
                # 更新值标签
                current_value = y_data[-1]
                self.value_labels[i].setText(f"Ch{i+1}: {current_value:.3f}")
                self.value_labels[i].setPos(last_x + offset_x, last_y + offset_y)
                self.value_labels[i].setVisible(True)
            else:
                # 无数据时隐藏标签
                self.value_labels[i].setVisible(False)
    
    def clear_waveforms(self):
        if self.is_running:
            self.wave_data = [[] for _ in range(5)]
            for i, curve in enumerate(self.curves):
                curve.setData([], [])
                self.value_labels[i].setVisible(False)
    
    def set_channel_visibility(self, channel, visible):
        if 0 <= channel < 5:
            self.curves[channel].setVisible(visible)
            self.value_labels[channel].setVisible(visible and len(self.wave_data[channel]) > 0)
    
    def get_channel_count(self):
        count = 0
        for wave in self.wave_data:
            if len(wave) > 0:
                count += 1
        return count
    
    def is_oscilloscope_running(self):
        return self.is_running