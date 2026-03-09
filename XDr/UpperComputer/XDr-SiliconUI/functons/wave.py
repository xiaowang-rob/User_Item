import pyqtgraph as pg
from PyQt5.QtWidgets import QWidget, QVBoxLayout
from PyQt5.QtCore import Qt, QTimer, pyqtSignal, QEvent
from PyQt5.QtGui import QFont
import numpy as np
from UI.data_ui_map import (
    Cidx,Didx,
    rad_per_sec_to_rpm,
    rpm_to_rad_per_sec,
    rad_to_deg,
    deg_to_rad,
    )

class WaveformWidget(QWidget):
    """波形显示控件 - 支持5通道实时波形绘制"""
    
    # 自动缩放状态变化信号
    auto_x_state_changed = pyqtSignal(bool)
    auto_y_state_changed = pyqtSignal(bool)
    
    def __init__(self, parent=None):
        super().__init__(parent)
        
        # ========== 核心配置参数 ==========
        self.max_points = 1000                      # 每通道最大数据点数
        self.colors = [                             # 5通道颜色配置
            (88, 230, 217),   # 青色
            (255, 170, 51),   # 橙色
            (255, 107, 157),  # 粉色
            (136, 204, 102),  # 绿色
            (184, 134, 255),  # 紫色
        ]
        self.wave_data = [[] for _ in range(5)]     # 5通道波形数据缓存
        self.is_running = True                      # 波形更新运行标志
        self.auto_x = True                          # X轴自动缩放标志
        self.auto_y = True                          # Y轴自动缩放标志
        
        # ========== UI初始化 ==========
        self.init_ui()
        
        # ========== 曲线对象创建 ==========
        self.curves = []
        for i in range(5):
            curve = self.plot_widget.plot(pen=pg.mkPen(color=self.colors[i], width=2))
            self.curves.append(curve)
        
        # ========== 末端值标签创建 ==========
        self.value_labels = []
        font = QFont()
        font.setPointSize(11)  # 设置标签字体大小
        
        for i in range(5):
            label = pg.TextItem(
                text="N/A",
                color=self.colors[i],               # 使用对应通道颜色
                border=pg.mkPen(80, 80, 80, 200),   # 边框颜色
                fill=pg.mkBrush(35, 35, 38, 220),   # 背景填充
                anchor=(0.5, 0.5)                   # 锚点居中
            )
            label.setFont(font)
            label.setZValue(10)                     # 置于顶层
            self.plot_widget.addItem(label)
            self.value_labels.append(label)
        
        # ========== 定时器配置 ==========
        self.auto_scale_timer = QTimer()
        self.auto_scale_timer.timeout.connect(self.auto_scale_axes)
        self.auto_scale_timer.start(100)  # 100ms自动缩放检查
        
        # ========== 交互配置 ==========
        self.plot_widget.setMouseEnabled(x=True, y=True)  # 启用鼠标交互
        self.plot_widget.getViewBox().setMouseMode(pg.ViewBox.PanMode)  # 平移模式
        self.plot_widget.scene().sigMouseClicked.connect(self._on_mouse_clicked)  # 双击事件
        
        # ========== 关键修复：为PlotWidget的viewport安装事件过滤器 ==========
        # PyQtGraph中滚轮事件实际由PlotWidget.viewport()接收，需在此层级拦截
        self.plot_widget.viewport().installEventFilter(self)

        # ========== 悬停显示功能初始化 ==========
        self.hover_label = pg.TextItem(
            text="",
            color='#ffffff',
            border=pg.mkPen(80, 80, 80, 200),
            fill=pg.mkBrush(35, 35, 38, 220),
            anchor=(0.5, 1.5)  # 标签位于点正上方
        )
        self.hover_label.setFont(QFont("Arial", 10, QFont.Bold))
        self.plot_widget.addItem(self.hover_label)
        self.hover_label.hide()

        # 限流鼠标事件（60Hz）
        self._mouse_proxy = pg.SignalProxy(
            self.plot_widget.scene().sigMouseMoved,
            rateLimit=60,
            slot=self._on_mouse_hover
        )
    
    def set_auto_x_scale(self, state):
        """设置X轴自动缩放状态"""                                                                                   
    
    def init_ui(self):
        """初始化UI布局和样式"""
        layout = QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        
        # 创建深色背景的绘图区域
        self.plot_widget = pg.PlotWidget(background='#39373F')
        self.plot_widget.setLabel('left', 'Y轴', color='#ffffff', size='10pt')
        self.plot_widget.setLabel('bottom', 'X轴', color='#ffffff', size='10pt')
        
        # 配置坐标轴样式
        self.plot_widget.getAxis('left').setTextPen('#aaaaaa')
        self.plot_widget.getAxis('bottom').setTextPen('#aaaaaa')
        self.plot_widget.getAxis('left').setPen('#777777')
        self.plot_widget.getAxis('bottom').setPen('#777777')
        self.plot_widget.showGrid(x=True, y=True, alpha=0.3)  # 显示网格
        
        layout.addWidget(self.plot_widget)
        self.setLayout(layout)
    
    def eventFilter(self, obj, event):
        """
        事件过滤器 - 拦截PlotWidget viewport的滚轮事件
        
        注意：PyQtGraph中滚轮事件实际由PlotWidget.viewport()接收，而非ViewBox。
        必须为PlotWidget的viewport安装事件过滤器才能可靠捕获滚轮事件。
        """
        if event.type() == QEvent.Wheel:
            # 检查事件来源是否为PlotWidget的viewport
            if obj is self.plot_widget.viewport():
                self.handle_wheel_event(event)
                return True  # 标记事件已处理，阻止默认缩放行为
        return super().eventFilter(obj, event)
    
    def handle_wheel_event(self, event):
        """
        处理滚轮事件逻辑
        
        - 普通滚轮：Y轴缩放，关闭Y轴自动缩放
        - Shift+滚轮：X轴缩放，关闭X轴自动缩放
        - 不再设置手动模式计时器，保持手动状态直到用户主动恢复
        """

        if event.modifiers() & Qt.ShiftModifier:
            # X轴缩放
            scale = 0.9 if event.angleDelta().y() > 0 else 1.1
            self.plot_widget.getViewBox().scaleBy(x=scale)
            self.set_auto_x_scale(False)
        else:
            # Y轴缩放
            scale = 0.9 if event.angleDelta().y() > 0 else 1.1
            self.plot_widget.getViewBox().scaleBy(y=scale)
            self.set_auto_y_scale(False)
        
        event.accept()
    
    def wheelEvent(self, event):
        """
        父控件滚轮事件（备用处理）
        
        虽然主要通过事件过滤器处理，但保留此方法以兼容可能传递到父控件的事件。
        """
        self.handle_wheel_event(event)
    
    def _on_mouse_clicked(self, event):
        """鼠标点击事件处理 - 双击恢复自动范围并重新启用自动缩放"""
        if event.double():
            self.plot_widget.enableAutoRange()  # 恢复自动范围
            self.set_auto_x_scale(True)         # 重新启用X轴自动缩放
            self.set_auto_y_scale(True)         # 重新启用Y轴自动缩放
    
    def auto_scale_axes(self):
        """自动缩放坐标轴（仅当auto_x/auto_y为True时执行）"""
        if self.auto_y:
            self.auto_scale_y_axis()
        if self.auto_x:
            self.auto_scale_x_axis()
    
    def auto_scale_y_axis(self):
        """Y轴自动缩放 - 过滤NaN/Inf避免ViewBox崩溃"""
        all_data = []
        for wave in self.wave_data:
            if wave:
                # 关键修复：过滤非有限值
                valid_data = [y for y in wave if np.isfinite(y)]
                if valid_data:
                    all_data.extend(valid_data)
        
        if not all_data:
            return
        
        min_val = min(all_data)
        max_val = max(all_data)
        margin = (max_val - min_val) * 0.1 if max_val != min_val else 1
        self.plot_widget.setYRange(min_val - margin, max_val + margin)
    
    def auto_scale_x_axis(self):
        """X轴自动缩放 - 保持最近200个点可见"""
        max_length = max((len(wave) for wave in self.wave_data), default=0)
        if max_length == 0:
            return
        
        view_width = 200
        x_max = max_length + 10
        x_min = max(0, x_max - view_width)
        
        self.plot_widget.setXRange(x_min, x_max)
    
    def add_waveform_data(self, channel, data):
        """
        添加波形数据
        
        Args:
            channel: 通道索引(0-4)
            data: 单个数值或数值列表
        """
        if 0 <= channel < 5 and self.is_running:
            if isinstance(data, (list, tuple, np.ndarray)):
                self.wave_data[channel].extend(data)
            else:
                self.wave_data[channel].append(data)
            
            # 限制最大数据点数
            if len(self.wave_data[channel]) > self.max_points:
                self.wave_data[channel] = self.wave_data[channel][-self.max_points:]
            
            self.update_plot()
    
    def update_plot(self):
        """更新波形显示"""
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
                    self.value_labels[i].setPos(last_x + 8, last_y)  # 标签偏移避免遮挡曲线
                    self.value_labels[i].setVisible(True)
                else:
                    self.value_labels[i].setVisible(False)
    
    def clear_waveforms(self):
        """清除所有波形数据"""
        self.wave_data = [[] for _ in range(5)]
        for i in range(5):
            self.curves[i].setData([], [])
            self.value_labels[i].setVisible(False)
    
    def set_auto_x_scale(self, enable: bool):
        """设置X轴自动缩放状态"""
        if self.auto_x != enable:
            self.auto_x = enable
            self.auto_x_state_changed.emit(enable)
    
    def set_auto_y_scale(self, enable: bool):
        """设置Y轴自动缩放状态"""
        if self.auto_y != enable:
            self.auto_y = enable
            self.auto_y_state_changed.emit(enable)
    
    def _on_mouse_hover(self, evt):
        """鼠标悬停显示最近点的Y值"""
        pos = evt[0] if isinstance(evt, tuple) else evt
        view_box = self.plot_widget.getViewBox()
        
        # 检查鼠标是否在绘图区域内
        if not self.plot_widget.sceneBoundingRect().contains(pos):
            self.hover_label.hide()
            return
        
        mouse_view = view_box.mapSceneToView(pos)
        view_range = view_box.viewRange()
        x_visible = (view_range[0][0], view_range[0][1])
        y_visible = (view_range[1][0], view_range[1][1])
        
        # 查找最近的有效数据点
        min_dist = float('inf')
        best_y = None
        
        for wave in self.wave_data:
            if not wave:
                continue
                
            # 仅处理可视区域内的数据（性能优化）
            x_data = np.arange(len(wave))
            mask = (x_data >= x_visible[0]) & (x_data <= x_visible[1])
            if not np.any(mask):
                continue
                
            y_data = np.array(wave)[mask]
            x_sub = x_data[mask]
            
            # 计算归一化距离（避免坐标轴比例影响）
            dx = (x_sub - mouse_view.x()) / (x_visible[1] - x_visible[0] + 1e-6)
            dy = (y_data - mouse_view.y()) / (y_visible[1] - y_visible[0] + 1e-6)
            dist = np.sqrt(dx*dx + dy*dy)
            
            idx = np.argmin(dist)
            if dist[idx] < min_dist:
                min_dist = dist[idx]
                best_y = y_data[idx]
        
        # 距离阈值：归一化距离 < 0.05 时显示（约15像素内）
        if min_dist < 0.05 and best_y is not None and np.isfinite(best_y):
            self.hover_label.setHtml(f'<span style="color:#ffffff; font-weight:bold;">{best_y:.3f}</span>')
            self.hover_label.setPos(mouse_view.x(), best_y)  # 标签定位在Y值位置
            self.hover_label.show()
        else:
            self.hover_label.hide()
    
    def leaveEvent(self, event):
        super().leaveEvent(event)
        self.hover_label.hide()

    def start(self):
        """启动波形更新"""
        self.is_running = True
    
    def pause(self):
        """暂停波形更新"""
        self.is_running = False


class Wave:
    """波形控制模块 - 连接UI与波形控件"""
    
    def __init__(self, main_window):
        self.mw = main_window
        self.pw = self.mw.control_page
        self.wave_area = self.pw.wave_area  # 波形显示区域
        self.com = self.mw.comport          # 串口通信对象
        
        self.showindex = []                 # 当前显示的数据索引
        self.channel_index = []             # 通道映射关系
        
        # 5个通道选择下拉框
        self.combo_boxes = [
            self.pw.wave_ch1,
            self.pw.wave_ch2,
            self.pw.wave_ch3,
            self.pw.wave_ch4,
            self.pw.wave_ch5,
        ]
        
        # 创建波形控件
        self.waveform_widget = WaveformWidget()
        
        # ========== 按钮初始化 ==========
        self.start_wave_but = self.pw.start_wave_button  # 启动/停止按钮
        self.auto_x_but = self.pw.auto_x_switch          # X轴自动缩放开关
        self.auto_x_but.setChecked(True)
        self.auto_y_but = self.pw.auto_y_switch          # Y轴自动缩放开关
        self.auto_y_but.setChecked(True)
        self.clear_wave_but = self.pw.clear_wave_button  # 清除按钮
        
        # ========== 信号连接 ==========
        self.start_wave_but.toggled.connect(self.handle_start)
        self.auto_x_but.toggled.connect(self.waveform_widget.set_auto_x_scale)
        self.auto_y_but.toggled.connect(self.waveform_widget.set_auto_y_scale)
        self.clear_wave_but.clicked.connect(self.clear)
        
        # ========== 嵌入波形控件到UI区域 ==========
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
        
        # ========== 双向状态同步 ==========
        # 按钮状态变化 → 波形控件
        self.auto_x_but.toggled.connect(self.waveform_widget.set_auto_x_scale)
        self.auto_y_but.toggled.connect(self.waveform_widget.set_auto_y_scale)
        
        # 波形控件状态变化 → 按钮（避免信号循环）
        self.waveform_widget.auto_x_state_changed.connect(self._sync_auto_x_button)
        self.waveform_widget.auto_y_state_changed.connect(self._sync_auto_y_button)
    
    def _sync_auto_x_button(self, state: bool):
        """同步X轴自动缩放按钮状态"""
        # 关键修复：直接调用按钮的_toggled处理方法，而非仅设置checked状态
        self.auto_x_but.blockSignals(True)
        self.auto_x_but.setChecked(state)
        self.auto_x_but._onButtonToggled(state)  # ← 强制触发颜色动画
        self.auto_x_but.blockSignals(False)
        self.auto_x_but.update()  # ← 确保立即重绘

    def _sync_auto_y_button(self, state: bool):
        """同步Y轴自动缩放按钮状态"""
        self.auto_y_but.blockSignals(True)
        self.auto_y_but.setChecked(state)
        self.auto_y_but._onButtonToggled(state)  # ← 强制触发颜色动画
        self.auto_y_but.blockSignals(False)
        self.auto_y_but.update()  # ← 确保立即重绘
    
    def clear(self):
        """清除波形"""
        self.waveform_widget.clear_waveforms()
    
    def handle_start(self, enable: bool):
        """
        启动/停止波形流
        
        Args:
            enable: True=启动, False=停止
        """
        if enable:
            self.waveform_widget.start()
            self.start_wave_but.setValue("Stop")
            self.showindex.clear()
            self.channel_index.clear()
            
            # 收集需要显示的通道
            for i, combo in enumerate(self.combo_boxes):
                current_text = combo.currentText()
                if current_text != "NONE":
                    self.showindex.append(combo.currentIndex()) 
                    self.channel_index.append(i)
            
            # 发送流配置命令
            self.com.send_packet(Cidx.CMD_STREAM_SET, bytes(self.showindex))
        else:
            self.waveform_widget.pause()
            self.start_wave_but.setValue("Start")
            self.showindex.clear()
            self.com.send_packet(Cidx.CMD_STREAM_SET, bytes())
    
    def add_data(self, channel: int, data):
        """
        添加波形数据到指定通道
        
        Args:
            channel: 通道索引(0-4)
            data: 数值或数值列表
        """
        self.waveform_widget.add_waveform_data(channel, data)

    def add_data_by_index(self, index: int, data):
        """
        通过显示索引添加数据（映射到实际通道）
        
        Args:
            index: 显示索引(0-4)
            data: 数值或数值列表
        """
        id_index=index%len(self.channel_index)
        match (self.showindex[id_index]+3):
            case Didx.SPEED|Didx.SPEED_ref:
                val=rad_per_sec_to_rpm(data)
            case Didx.THETA_elec|Didx.THETA_mech|Didx.POSITION|Didx.POSITION_ref:
                val=rad_to_deg(data)
            case _:
                val=data
        if id_index < len(self.channel_index):
            self.add_data(self.channel_index[id_index], val)
        #print("index:",index,"id:",id_index,"数据id",self.showindex[id_index]+3,"通道",self.channel_index[id_index],"值",val)