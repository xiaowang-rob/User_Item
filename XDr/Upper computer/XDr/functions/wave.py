import pyqtgraph as pg
from PyQt5.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QLabel, QSpinBox, QPushButton, QCheckBox
from PyQt5.QtCore import Qt, QTimer
from PyQt5.QtGui import QColor
import numpy as np

class WaveformWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.init_ui()
        
        # 波形数据存储 (最多5条)
        self.wave_data = [[] for _ in range(5)]
        self.max_points = 1000  # 每条波形最大点数
        
        # 颜色配置
        self.colors = [
            (255, 0, 0),      # 红色
            (0, 255, 0),      # 绿色
            (0, 0, 255),      # 蓝色
            (255, 255, 0),    # 黄色
            (255, 0, 255)     # 紫色
        ]
        
        # 曲线对象
        self.curves = []
        
        # 创建曲线
        for i in range(5):
            curve = self.plot_widget.plot(pen=pg.mkPen(color=self.colors[i], width=2))
            self.curves.append(curve)
        
        # 自动调节计时器
        self.auto_scale_timer = QTimer()
        self.auto_scale_timer.timeout.connect(self.auto_scale_y_axis)
        self.auto_scale_timer.start(1000)  # 每秒自动调节一次
        
        # X轴范围
        self.x_min = 0
        self.x_max = 100
        
        # 示波状态
        self.is_running = True  # 默认开始状态
        
    def init_ui(self):
        layout = QVBoxLayout()
        
        # 创建pyqtgraph绘图窗口
        self.plot_widget = pg.PlotWidget()
        self.plot_widget.setBackground('white')
        self.plot_widget.setLabel('left', 'Y轴')
        self.plot_widget.setLabel('bottom', 'X轴')
        
        # 添加控制面板
        control_layout = QHBoxLayout()
        
        # 开始/停止按钮
        self.start_stop_btn = QPushButton("停止示波")
        self.start_stop_btn.clicked.connect(self.toggle_oscilloscope)
        
        # Y轴范围控制
        y_range_label = QLabel("Y轴范围:")
        self.y_min_spin = QSpinBox()
        self.y_min_spin.setRange(-10000, 10000)
        self.y_min_spin.setValue(-10)
        self.y_min_spin.valueChanged.connect(self.update_y_range)
        
        self.y_max_spin = QSpinBox()
        self.y_max_spin.setRange(-10000, 10000)
        self.y_max_spin.setValue(10)
        self.y_max_spin.valueChanged.connect(self.update_y_range)
        
        # 自动调节Y轴复选框
        self.auto_y_check = QCheckBox("自动Y轴")
        self.auto_y_check.setChecked(True)
        self.auto_y_check.stateChanged.connect(self.toggle_auto_scale)
        
        # X轴范围控制
        x_range_label = QLabel("X轴范围:")
        self.x_min_spin = QSpinBox()
        self.x_min_spin.setRange(0, 10000)
        self.x_min_spin.setValue(0)
        self.x_min_spin.valueChanged.connect(self.update_x_range)
        
        self.x_max_spin = QSpinBox()
        self.x_max_spin.setRange(0, 10000)
        self.x_max_spin.setValue(100)
        self.x_max_spin.valueChanged.connect(self.update_x_range)
        
        # 清除按钮
        clear_btn = QPushButton("清除波形")
        clear_btn.clicked.connect(self.clear_waveforms)
        
        # 添加到控制面板
        control_layout.addWidget(self.start_stop_btn)
        control_layout.addWidget(y_range_label)
        control_layout.addWidget(self.y_min_spin)
        control_layout.addWidget(self.y_max_spin)
        control_layout.addWidget(self.auto_y_check)
        control_layout.addWidget(x_range_label)
        control_layout.addWidget(self.x_min_spin)
        control_layout.addWidget(self.x_max_spin)
        control_layout.addWidget(clear_btn)
        control_layout.addStretch()
        
        layout.addWidget(self.plot_widget)
        layout.addLayout(control_layout)
        self.setLayout(layout)
    
    def start_oscilloscope(self):
        """开始示波"""
        if not self.is_running:
            self.is_running = True
            self.start_stop_btn.setText("停止示波")
            # 恢复自动缩放
            if self.auto_y_check.isChecked():
                self.auto_scale_timer.start(1000)
    
    def stop_oscilloscope(self):
        """停止示波"""
        if self.is_running:
            self.is_running = False
            self.start_stop_btn.setText("开始示波")
            # 停止自动缩放
            self.auto_scale_timer.stop()
    
    def toggle_oscilloscope(self):
        """切换示波状态"""
        if self.is_running:
            self.stop_oscilloscope()
        else:
            self.start_oscilloscope()
    
    def add_waveform_data(self, channel, data):
        """
        添加波形数据
        channel: 通道号 (0-4)
        data: 列表数组或单个数值
        """
        if 0 <= channel < 5 and self.is_running:  # 只有在运行状态下才添加数据
            if isinstance(data, (list, tuple, np.ndarray)):
                self.wave_data[channel].extend(data)
            else:
                self.wave_data[channel].append(data)
            
            # 限制数据长度
            if len(self.wave_data[channel]) > self.max_points:
                self.wave_data[channel] = self.wave_data[channel][-self.max_points:]
            
            self.update_plot()
    
    def update_plot(self):
        """更新波形显示"""
        if self.is_running:
            for i in range(5):
                if len(self.wave_data[i]) > 0:
                    y_data = np.array(self.wave_data[i])
                    x_data = np.arange(len(y_data)) + self.x_min
                    
                    # 更新对应曲线
                    self.curves[i].setData(x_data, y_data)
    
    def auto_scale_y_axis(self):
        """自动调节Y轴范围"""
        if not self.auto_y_check.isChecked() or not self.is_running:
            return
            
        all_data = []
        for wave in self.wave_data:
            if len(wave) > 0:
                all_data.extend(wave)
        
        if len(all_data) > 0:
            min_val = min(all_data)
            max_val = max(all_data)
            
            # 添加一些边距
            margin = (max_val - min_val) * 0.1 if max_val != min_val else 1
            new_min = min_val - margin
            new_max = max_val + margin
            
            self.plot_widget.setYRange(new_min, new_max)
            self.y_min_spin.setValue(int(new_min))
            self.y_max_spin.setValue(int(new_max))
    
    def update_y_range(self):
        """手动更新Y轴范围"""
        if not self.auto_y_check.isChecked():
            y_min = self.y_min_spin.value()
            y_max = self.y_max_spin.value()
            if y_max > y_min:
                self.plot_widget.setYRange(y_min, y_max)
    
    def update_x_range(self):
        """更新X轴范围"""
        x_min = self.x_min_spin.value()
        x_max = self.x_max_spin.value()
        if x_max > x_min:
            self.x_min = x_min
            self.x_max = x_max
            self.plot_widget.setXRange(x_min, x_max)
    
    def toggle_auto_scale(self, state):
        """切换自动调节状态"""
        if state == Qt.Checked and self.is_running:
            self.auto_scale_timer.start(1000)
        elif not self.is_running:
            self.auto_scale_timer.stop()
    
    def clear_waveforms(self):
        """清除所有波形数据"""
        if self.is_running:
            self.wave_data = [[] for _ in range(5)]
            for curve in self.curves:
                curve.setData([], [])
    
    def set_channel_visibility(self, channel, visible):
        """设置某个通道是否可见"""
        if 0 <= channel < 5:
            self.curves[channel].setVisible(visible)
    
    def get_channel_count(self):
        """获取当前有效通道数"""
        count = 0
        for i, wave in enumerate(self.wave_data):
            if len(wave) > 0:
                count += 1
        return count
    
    def is_oscilloscope_running(self):
        """返回示波器是否正在运行"""
        return self.is_running