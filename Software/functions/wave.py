import pyqtgraph as pg
from PyQt5.QtWidgets import QWidget, QVBoxLayout
from PyQt5.QtCore import Qt, QTimer, pyqtSignal, QEvent
from PyQt5.QtGui import QFont
import numpy as np
import logging
from protocol import Cidx

# ---------- 日志配置 ----------
logger = logging.getLogger("Wave")
logger.setLevel(logging.INFO)
if not logger.handlers:
    _handler = logging.StreamHandler()
    _handler.setFormatter(logging.Formatter("[%(levelname)s] %(name)s: %(message)s"))
    logger.addHandler(_handler)

class WaveformWidget(QWidget):
    """波形显示控件 - 支持5通道实时波形绘制"""

    auto_x_state_changed = pyqtSignal(bool)
    auto_y_state_changed = pyqtSignal(bool)

    def __init__(self, parent=None):
        super().__init__(parent)

        # 核心配置
        self.max_points = 4000              # 每通道最大缓存点数
        self.min_view_points = 1000          # X轴最小显示点数
        self.max_view_points = 3000          # X轴最大显示点数
        self.colors = [
            (88, 230, 217),
            (255, 170, 51),
            (255, 107, 157),
            (136, 204, 102),
            (184, 134, 255),
        ]
        self.wave_data = [[] for _ in range(5)]
        self.is_running = True
        self.auto_x = True
        self.auto_y = True

        # UI
        self.init_ui()

        # 曲线
        self.curves = []
        for i in range(5):
            curve = self.plot_widget.plot(pen=pg.mkPen(color=self.colors[i], width=2))
            self.curves.append(curve)

        # 末端值标签 - 共用字体
        label_font = QFont()
        label_font.setPointSize(11)
        self.value_labels = []
        for i in range(5):
            label = pg.TextItem(
                text="N/A",
                color=self.colors[i],
                border=pg.mkPen(80, 80, 80, 200),
                fill=pg.mkBrush(35, 35, 38, 220),
                anchor=(0.5, 0.5),
            )
            label.setFont(label_font)
            label.setZValue(10)
            self.plot_widget.addItem(label)
            self.value_labels.append(label)

        # 定时器
        self.auto_scale_timer = QTimer()
        self.auto_scale_timer.timeout.connect(self.auto_scale_axes)
        self.auto_scale_timer.start(100)

        # 交互
        self.plot_widget.setMouseEnabled(x=True, y=True)
        self.plot_widget.getViewBox().setMouseMode(pg.ViewBox.PanMode)
        self.plot_widget.scene().sigMouseClicked.connect(self._on_mouse_clicked)

        # 滚轮事件拦截（viewport级）
        self.plot_widget.viewport().installEventFilter(self)

        # 悬停标签
        self.hover_label = pg.TextItem(
            text="",
            color="#ffffff",
            border=pg.mkPen(80, 80, 80, 200),
            fill=pg.mkBrush(35, 35, 38, 220),
            anchor=(0.5, 1.5),
        )
        self.hover_label.setFont(QFont("Arial", 10, QFont.Bold))
        self.plot_widget.addItem(self.hover_label)
        self.hover_label.hide()

        self._mouse_proxy = pg.SignalProxy(
            self.plot_widget.scene().sigMouseMoved,
            rateLimit=60,
            slot=self._on_mouse_hover,
        )

        # 手动拖动时同步关闭自动缩放
        self.plot_widget.getViewBox().sigRangeChangedManually.connect(
            self._on_manual_range_change
        )

    # ---------- UI ----------
    def init_ui(self):
        layout = QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)

        self.plot_widget = pg.PlotWidget(background="#39373F")
        self.plot_widget.setLabel("left", "Y轴", color="#ffffff", size="10pt")
        self.plot_widget.setLabel("bottom", "X轴", color="#ffffff", size="10pt")
        self.plot_widget.getAxis("left").setTextPen("#aaaaaa")
        self.plot_widget.getAxis("bottom").setTextPen("#aaaaaa")
        self.plot_widget.getAxis("left").setPen("#777777")
        self.plot_widget.getAxis("bottom").setPen("#777777")
        self.plot_widget.showGrid(x=True, y=True, alpha=0.3)

        layout.addWidget(self.plot_widget)
        self.setLayout(layout)

    # ---------- 事件处理 ----------
    def eventFilter(self, obj, event):
        if event.type() == QEvent.Wheel:
            if obj is self.plot_widget.viewport():
                self.handle_wheel_event(event)
                return True
        return super().eventFilter(obj, event)

    def handle_wheel_event(self, event):
        if event.modifiers() & Qt.ShiftModifier:
            scale = 0.9 if event.angleDelta().y() > 0 else 1.1
            self.plot_widget.getViewBox().scaleBy(x=scale)
            self.set_auto_x_scale(False)
        else:
            scale = 0.9 if event.angleDelta().y() > 0 else 1.1
            self.plot_widget.getViewBox().scaleBy(y=scale)
            self.set_auto_y_scale(False)
        event.accept()

    def _on_manual_range_change(self, _):
        """手动平移/缩放时，同时关闭XY自动缩放"""
        self.set_auto_x_scale(False)
        self.set_auto_y_scale(False)

    def _on_mouse_clicked(self, event):
        if event.double():
            self.plot_widget.enableAutoRange()
            self.set_auto_x_scale(True)
            self.set_auto_y_scale(True)

    # ---------- 自动缩放逻辑 ----------
    def auto_scale_axes(self):
        if self.auto_y:
            self.auto_scale_y_axis()
        if self.auto_x:
            self.auto_scale_x_axis()

    def auto_scale_y_axis(self):
        all_data = []
        for wave in self.wave_data:
            if wave:
                valid = [y for y in wave if np.isfinite(y)]
                if valid:
                    all_data.extend(valid)

        if not all_data:
            return

        min_val = min(all_data)
        max_val = max(all_data)
        if max_val == min_val:
            margin = 1.0
        else:
            margin = (max_val - min_val) * 0.1
        self.plot_widget.setYRange(min_val - margin, max_val + margin)

    def auto_scale_x_axis(self):
        """X轴自动缩放：先固定最小范围，超出后动态视野"""
        total_points = max((len(wave) for wave in self.wave_data), default=0)
        if total_points <= 0:
            return

        # 阶段1：未超出最小视野 → 固定显示 [0, min_view_points]
        if total_points <= self.min_view_points:
            self.plot_widget.setXRange(0, self.min_view_points)
            return

        # 阶段2：已超出 → 基于密度动态调整视野
        all_points = []
        for wave in self.wave_data:
            if wave:
                all_points.extend([y for y in wave if np.isfinite(y)])
        if not all_points:
            return

        # 密度指标（标准差/极差）
        if len(all_points) > 1:
            global_std = np.std(all_points)
            range_val = max(all_points) - min(all_points)
            density = global_std / range_val if range_val > 0 else 0
        else:
            density = 0

        view_width = int(self.min_view_points + density * (self.max_view_points - self.min_view_points))
        view_width = max(self.min_view_points, min(view_width, self.max_view_points))

        x_max = total_points                     # 右边界紧贴最新点
        x_min = max(0, x_max - view_width)       # 左边界动态调整

        self.plot_widget.setXRange(x_min, x_max)
    def add_waveform_data(self, channel, data):
        if not (0 <= channel < 5) or not self.is_running:
            return

        if isinstance(data, (list, tuple, np.ndarray)):
            self.wave_data[channel].extend(data)
        else:
            self.wave_data[channel].append(data)

        # 限制最大缓存
        if len(self.wave_data[channel]) > self.max_points:
            self.wave_data[channel] = self.wave_data[channel][-self.max_points:]

        self.update_plot()

    def update_plot(self):
        if not self.is_running:
            return

        for i in range(5):
            wave = self.wave_data[i]
            if wave:
                y_data = np.array(wave)
                x_data = np.arange(len(y_data))
                self.curves[i].setData(x_data, y_data)

                # 末端标签
                last_x, last_y = x_data[-1], y_data[-1]
                self.value_labels[i].setText(f"{last_y:.3f}")
                self.value_labels[i].setPos(last_x + 8, last_y)
                self.value_labels[i].setVisible(True)
            else:
                self.curves[i].setData([], [])
                self.value_labels[i].setVisible(False)

    def clear_waveforms(self):
        self.wave_data = [[] for _ in range(5)]
        for i in range(5):
            self.curves[i].setData([], [])
            self.value_labels[i].setVisible(False)
        logger.debug("波形已清除")

    def set_auto_x_scale(self, enable: bool):
        if self.auto_x != enable:
            self.auto_x = enable
            self.auto_x_state_changed.emit(enable)
            logger.debug(f"X自动缩放 -> {enable}")

    def set_auto_y_scale(self, enable: bool):
        if self.auto_y != enable:
            self.auto_y = enable
            self.auto_y_state_changed.emit(enable)
            logger.debug(f"Y自动缩放 -> {enable}")

    def start(self):
        self.is_running = True
        logger.debug("波形启动")

    def pause(self):
        self.is_running = False
        logger.debug("波形暂停")

    # ---------- 悬停显示 ----------
    def _on_mouse_hover(self, evt):
        pos = evt[0] if isinstance(evt, tuple) else evt
        view_box = self.plot_widget.getViewBox()

        if not self.plot_widget.sceneBoundingRect().contains(pos):
            self.hover_label.hide()
            return

        mouse_view = view_box.mapSceneToView(pos)
        view_range = view_box.viewRange()
        x_range = view_range[0]
        y_range = view_range[1]

        min_dist = float("inf")
        best_y = None

        for wave in self.wave_data:
            if not wave:
                continue
            x_data = np.arange(len(wave))
            mask = (x_data >= x_range[0]) & (x_data <= x_range[1])
            if not np.any(mask):
                continue
            y_arr = np.array(wave)[mask]
            x_sub = x_data[mask]

            dx = (x_sub - mouse_view.x()) / (x_range[1] - x_range[0] + 1e-6)
            dy = (y_arr - mouse_view.y()) / (y_range[1] - y_range[0] + 1e-6)
            dist = np.sqrt(dx**2 + dy**2)

            idx = np.argmin(dist)
            if dist[idx] < min_dist:
                min_dist = dist[idx]
                best_y = y_arr[idx]

        if min_dist < 0.05 and best_y is not None and np.isfinite(best_y):
            self.hover_label.setHtml(
                f'<span style="color:#ffffff; font-weight:bold;">{best_y:.3f}</span>'
            )
            self.hover_label.setPos(mouse_view.x(), best_y)
            self.hover_label.show()
        else:
            self.hover_label.hide()

    def leaveEvent(self, event):
        super().leaveEvent(event)
        self.hover_label.hide()


class Wave:
    """波形控制模块 - 连接UI与波形控件"""

    def __init__(self, main_window):
        self.mw = main_window
        self.pw = self.mw.control_page
        self.wave_area = self.pw.wave_area
        self.com = self.mw.comport

        self.showindex = []
        self.channel_index = []

        self.combo_boxes = [
            self.pw.wave_ch1,
            self.pw.wave_ch2,
            self.pw.wave_ch3,
            self.pw.wave_ch4,
            self.pw.wave_ch5,
        ]

        self.waveform_widget = WaveformWidget()

        self.start_wave_but = self.pw.start_wave_button
        self.auto_x_but = self.pw.auto_x_switch
        self.auto_x_but.setChecked(True)
        self.auto_y_but = self.pw.auto_y_switch
        self.auto_y_but.setChecked(True)
        self.clear_wave_but = self.pw.clear_wave_button

        self.start_wave_but.toggled.connect(self.handle_start)
        self.auto_x_but.toggled.connect(self.waveform_widget.set_auto_x_scale)
        self.auto_y_but.toggled.connect(self.waveform_widget.set_auto_y_scale)
        self.clear_wave_but.clicked.connect(self.clear)

        # 嵌入波形控件
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

        # 双向同步（auto_x/auto_y 的 toggled 信号已在上方连接）
        self.waveform_widget.auto_x_state_changed.connect(self._sync_auto_x_button)
        self.waveform_widget.auto_y_state_changed.connect(self._sync_auto_y_button)

    def _sync_auto_x_button(self, state: bool):
        self.auto_x_but.blockSignals(True)
        self.auto_x_but.setChecked(state)
        self.auto_x_but._onButtonToggled(state)
        self.auto_x_but.blockSignals(False)
        self.auto_x_but.update()

    def _sync_auto_y_button(self, state: bool):
        self.auto_y_but.blockSignals(True)
        self.auto_y_but.setChecked(state)
        self.auto_y_but._onButtonToggled(state)
        self.auto_y_but.blockSignals(False)
        self.auto_y_but.update()

    def clear(self):
        self.waveform_widget.clear_waveforms()

    def handle_start(self, enable: bool):
        if enable:
            self.waveform_widget.start()
            self.start_wave_but.setValue("Stop")
            self.showindex.clear()
            self.channel_index.clear()
            for i, combo in enumerate(self.combo_boxes):
                if combo.currentText() != "NONE":
                    self.showindex.append(combo.currentIndex()-1)
                    self.channel_index.append(i)
            self.com.send_packet(Cidx.CMD_STREAM_SET, bytes(self.showindex))
            logger.info("波形流已启动，通道映射: %s", self.channel_index)
        else:
            self.waveform_widget.pause()
            self.start_wave_but.setValue("Start")
            self.showindex.clear()
            self.com.send_packet(Cidx.CMD_STREAM_SET, bytes())
            logger.info("波形流已停止")

    def add_data(self, channel: int, data):
        self.waveform_widget.add_waveform_data(channel, data)

    def handle_stream_data(self, data):
        import struct
        count = len(data) // 4
        for i in range(count):
            val = struct.unpack("<f", data[i*4:(i+1)*4])[0]
            self.add_data_by_index(i, val)

    def add_data_by_index(self, index: int, data):
        id_index = index % len(self.channel_index)
        if id_index < len(self.channel_index):
            self.add_data(self.channel_index[id_index], data)