import pyqtgraph as pg
from PyQt5.QtWidgets import QWidget, QVBoxLayout
from PyQt5.QtCore import Qt, QTimer, pyqtSignal
from PyQt5.QtGui import QFont
import numpy as np
import time
from UI.data_ui_map import Cidx


class WaveformWidget(QWidget):
    # 新增信号：当自动缩放状态变化时发出
    auto_x_state_changed = pyqtSignal(bool)
    auto_y_state_changed = pyqtSignal(bool)
    
    def __init__(self, parent=None):
        super().__init__(parent)
        
        # 核心属性
        self.max_points = 1000
        self.colors = [
            (88, 230, 217),   # #58E6D9
            (255, 170, 51),   # #FFAA33
            (255, 107, 157),  # #FF6B9D
            (136, 204, 102),  # #88CC66
            (184, 134, 255),  # #B886FF
        ]
        self.wave_data = [[] for _ in range(5)]
        self.is_running = True
        self.auto_x = True   # X轴自动缩放标志
        self.auto_y = True   # Y轴自动缩放标志
        self.manual_mode_until = 0  # 手动模式截止时间戳(毫秒)
        
        # 初始化UI（仅绘图区域）
        self.init_ui()
        
        # 创建曲线
        self.curves = []
        for i in range(5):
            curve = self.plot_widget.plot(pen=pg.mkPen(color=self.colors[i], width=2))
            self.curves.append(curve)
        
        # 创建值标签
        self.value_labels = []
        font = QFont()
        font.setPointSize(11)  # 字号稍大

        for i in range(5):
            label = pg.TextItem(
                text="N/A",
                color=self.colors[i],  # ← 关键：使用对应曲线颜色
                border=pg.mkPen(80, 80, 80, 200),
                fill=pg.mkBrush(35, 35, 38, 220),
                anchor=(0.5, 0.5)
            )
            label.setFont(font)
            label.setZValue(10)
            self.plot_widget.addItem(label)
            self.value_labels.append(label)
        
        # 定时器（自动缩放）
        self.auto_scale_timer = QTimer()
        self.auto_scale_timer.timeout.connect(self.auto_scale_axes)
        self.auto_scale_timer.start(100)
        
        # 启用鼠标交互
        self.plot_widget.setMouseEnabled(x=True, y=True)
        self.plot_widget.getViewBox().setMouseMode(pg.ViewBox.PanMode)
        self.plot_widget.scene().sigMouseClicked.connect(self._on_mouse_clicked)
    
    def init_ui(self):
        layout = QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        
        # 深色背景绘图区域
        self.plot_widget = pg.PlotWidget(background='#39373F')
        self.plot_widget.setLabel('left', 'Y轴', color='#ffffff', size='10pt')
        self.plot_widget.setLabel('bottom', 'X轴', color='#ffffff', size='10pt')
        
        # 坐标轴样式
        self.plot_widget.getAxis('left').setTextPen('#aaaaaa')
        self.plot_widget.getAxis('bottom').setTextPen('#aaaaaa')
        self.plot_widget.getAxis('left').setPen('#777777')
        self.plot_widget.getAxis('bottom').setPen('#777777')
        self.plot_widget.showGrid(x=True, y=True, alpha=0.3)
        
        layout.addWidget(self.plot_widget)
        self.setLayout(layout)
    
    def wheelEvent(self, event):
        """鼠标滚轮控制：Y轴缩放 / Shift+滚轮X轴缩放"""
        if event.modifiers() & Qt.ShiftModifier:
            # X轴缩放 → 关闭X轴自动缩放
            scale = 0.9 if event.angleDelta().y() > 0 else 1.1
            self.plot_widget.getViewBox().scaleBy(x=scale)
            self.set_auto_x_scale(False)
            print(f"X轴缩放：{scale}")
        else:
            # Y轴缩放 → 关闭Y轴自动缩放
            scale = 0.9 if event.angleDelta().y() > 0 else 1.1
            self.plot_widget.getViewBox().scaleBy(y=scale)
            self.set_auto_y_scale(False)
            print(f"Y轴缩放：{scale}")
        
        # 进入手动模式（3秒内禁用自动缩放）
        self.manual_mode_until = time.time() * 1000 + 3000
        event.accept()
    
    def _on_mouse_clicked(self, event):
        """双击恢复自动范围 → 同时开启X/Y轴自动缩放"""
        if event.double():
            self.plot_widget.enableAutoRange()
            self.manual_mode_until = 0
            self.set_auto_x_scale(True) 
            self.set_auto_y_scale(True)  
    
    def auto_scale_axes(self):
        """自动缩放坐标轴（受manual_mode和auto_x/auto_y控制）"""
        current_time = time.time() * 1000
        if current_time < self.manual_mode_until:
            return  # 手动模式期间跳过自动缩放
        
        if self.auto_y:
            self.auto_scale_y_axis()
        if self.auto_x:
            self.auto_scale_x_axis()
    
    def auto_scale_y_axis(self):
        all_data = []
        for wave in self.wave_data:
            if len(wave) > 0:
                all_data.extend(wave)
        
        if not all_data:
            return
        
        min_val = min(all_data)
        max_val = max(all_data)
        margin = (max_val - min_val) * 0.1 if max_val != min_val else 1
        
        self.plot_widget.setYRange(min_val - margin, max_val + margin)
    
    def auto_scale_x_axis(self):
        max_length = max((len(wave) for wave in self.wave_data), default=0)
        if max_length == 0:
            return
        
        # 保持最近200个点可见
        view_width = 200
        x_max = max_length + 10
        x_min = max(0, x_max - view_width)
        
        self.plot_widget.setXRange(x_min, x_max)
    
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
            
        for i in range(5):
            if self.wave_data[i]:
                y_data = np.array(self.wave_data[i])
                x_data = np.arange(len(y_data))
                
                self.curves[i].setData(x_data, y_data)
                
                # 更新末端值标签
                if self.wave_data[i]:
                    last_x, last_y = x_data[-1], y_data[-1] 
                    self.value_labels[i].setText(f"{last_y:.3f}") 
                    self.value_labels[i].setPos(last_x + 8, last_y)
                    self.value_labels[i].setVisible(True)
                else:
                    self.value_labels[i].setVisible(False)
    
    # ===== 核心接口（带状态同步）=====
    
    def clear_waveforms(self):
        """清除波形数据接口"""
        self.wave_data = [[] for _ in range(5)]
        for i in range(5):
            self.curves[i].setData([], [])
            self.value_labels[i].setVisible(False)
    
    def set_auto_x_scale(self, enable: bool):
        """X轴自动缩放开关接口（带信号发射）"""
        if self.auto_x != enable:
            self.auto_x = enable
            self.auto_x_state_changed.emit(enable)  # ← 关键：发出状态变化信号
    
    def set_auto_y_scale(self, enable: bool):
        """Y轴自动缩放开关接口（带信号发射）"""
        if self.auto_y != enable:
            self.auto_y = enable
            self.auto_y_state_changed.emit(enable)  # ← 关键：发出状态变化信号
    
    def start(self):
        """启动波形更新接口"""
        self.is_running = True
    
    def pause(self):
        """暂停波形更新接口"""
        self.is_running = False


class Wave:
    def __init__(self, main_window):
        self.mw = main_window
        self.pw = self.mw.control_page
        self.wave_area = self.pw.wave_area  # self.wave_area 就是示波区域
        self.com=self.mw.comport    


        self.showindex=[]
        self.channel_index=[]

        self.combo_boxes = [
            self.pw.wave_ch1,
            self.pw.wave_ch2,
            self.pw.wave_ch3,
            self.pw.wave_ch4,
            self.pw.wave_ch5,
        ]
        # 创建波形控件并嵌入到 wave_area
        self.waveform_widget = WaveformWidget()
        #按钮

        self.start_wave_but = self.pw.start_wave_button  # 启动按钮
        self.auto_x_but = self.pw.auto_x_switch  # 双态按钮
        self.auto_x_but.setChecked(True)
        self.auto_y_but = self.pw.auto_y_switch  # 双态按钮
        self.auto_y_but.setChecked(True)
        self.clear_wave_but = self.pw.clear_wave_button  # 清除按钮
        self.start_wave_but.toggled.connect(self.handle_start)  # 启动按钮槽函数
        self.auto_x_but.toggled.connect(self.waveform_widget.set_auto_x_scale)  # 双态按钮槽函数
        self.auto_y_but.toggled.connect(self.waveform_widget.set_auto_y_scale)  # 双态按钮槽函数
        self.clear_wave_but.clicked.connect(self.clear)  # 清除按钮槽函数        
        # 清理原有布局并嵌入
        layout = self.wave_area.layout()
        if layout:
            while layout.count():
                child = layout.takeAt(0)
                if child.widget():
                    child.widget().deleteLater()
        else:
            layout = QVBoxLayout(self.wave_area)
            layout.setContentsMargins(0, 0, 0, 0)
        
        layout.addWidget(self.waveform_widget)
        
        # ===== 直接操作双态按钮（双向同步）=====
        # 1. 按钮状态变化 → 更新波形控件
        self.auto_x_but.toggled.connect(self.waveform_widget.set_auto_x_scale)
        self.auto_y_but.toggled.connect(self.waveform_widget.set_auto_y_scale)
        
        # 2. 波形控件状态变化 → 更新按钮（关键：避免循环）
        self.waveform_widget.auto_x_state_changed.connect(self._sync_auto_x_button)
        self.waveform_widget.auto_y_state_changed.connect(self._sync_auto_y_button)
        # ===== 按钮操作结束 =====
    
    def _sync_auto_x_button(self, state: bool):
        """同步X轴自动缩放按钮状态（避免信号循环）"""
        self.auto_x_but.blockSignals(True)
        self.auto_x_but.setChecked(state)
        self.auto_x_but.blockSignals(False)
    
    def _sync_auto_y_button(self, state: bool):
        """同步Y轴自动缩放按钮状态（避免信号循环）"""
        self.auto_y_but.blockSignals(True)
        self.auto_y_but.setChecked(state)
        self.auto_y_but.blockSignals(False)
    
    # ===== 以下3个方法是其他按钮的槽函数接口 =====
    
    def clear(self):
        """清除波形按钮槽函数调用此方法"""
        self.waveform_widget.clear_waveforms()
    
    def handle_start(self,enable:bool):
        """启动按钮槽函数调用此方法"""
        if enable:
            self.waveform_widget.start()
            self.start_wave_but.setValue("Stop")
            self.showindex.clear()
            self.channel_index.clear()
            for  i,combo in enumerate(self.combo_boxes):
                current_text = combo.currentText()
                if current_text != "NONE":
                    self.showindex.append(combo.currentIndex()) 
                    self.channel_index.append(i)
            self.com.send_packet(Cidx.CMD_STREAM_SET, bytes(self.showindex))
        else:
            self.waveform_widget.pause()
            self.start_wave_but.setValue("Start")
            self.showindex.clear()
            self.com.send_packet(Cidx.CMD_STREAM_SET, bytes())
    
    # ===== 数据添加接口 =====
    def add_data(self, channel: int, data):
        """添加波形数据（channel: 0-4,  float或数值列表）"""
        self.waveform_widget.add_waveform_data(channel, data)

    def add_data_by_index(self, index: int, data):
        """添加波形数据（index: 0-4,  float或数值列表）"""
        self.add_data(self.channel_index[index], data)