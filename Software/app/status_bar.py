from PyQt5.QtWidgets import QHBoxLayout, QVBoxLayout, QWidget, QFrame
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QColor, QPixmap
from qfluentwidgets import (
    ComboBox, PushButton, LineEdit,
    CardWidget, BodyLabel,
    setTheme, setThemeColor, Theme, FluentThemeColor,
    PrimaryPushButton, InfoBar, InfoBarPosition, ImageLabel,
)


class StatusBar(QWidget):
    """顶部状态栏 —— Logo + 连接/状态/配置 + 快捷按钮 + 主题切换"""

    def __init__(self, parent=None):
        super().__init__(parent)

        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(8, 2, 8, 2)
        main_layout.setSpacing(4)

        # ── 第一行：Logo + 连接/状态/复位/配置/主题 ──
        top_row = QHBoxLayout()
        top_row.setContentsMargins(0, 0, 0, 0)
        top_row.setSpacing(8)

        # Logo（已在标题栏显示，此处不再重复）
        self.logo_label = None

        # 1. 连接区域
        self.connect_card = CardWidget()
        connect_layout = QVBoxLayout(self.connect_card)
        connect_layout.setContentsMargins(8, 4, 8, 4)
        connect_layout.setSpacing(4)

        row1 = QHBoxLayout()
        row1.setContentsMargins(0, 0, 0, 0)
        row1.setSpacing(8)
        self.com_port = ComboBox()
        self.com_port.setMinimumWidth(140)
        row1.addWidget(self.com_port, 1)

        row2 = QHBoxLayout()
        row2.setContentsMargins(0, 0, 0, 0)
        row2.setSpacing(8)
        self.connect_but = PushButton()
        self.connect_but.setText("未连接")
        self.connect_but.setCheckable(True)
        row2.addWidget(self.connect_but, 1)

        connect_layout.addLayout(row1)
        connect_layout.addLayout(row2)

        # 2. 状态显示区域
        self.status_card = CardWidget()
        status_layout = QVBoxLayout(self.status_card)
        status_layout.setContentsMargins(8, 4, 8, 4)
        status_layout.setSpacing(12)

        self.sensormode_show = LineEdit()
        self.sensormode_show.setReadOnly(True)
        self.sensormode_show.setPlaceholderText("感应模式")
        self.sensormode_show.setMinimumWidth(100)
        self.runmode_show = LineEdit()
        self.runmode_show.setReadOnly(True)
        self.runmode_show.setPlaceholderText("运行模式")
        self.state_show = LineEdit()
        self.state_show.setReadOnly(True)
        self.state_show.setText("IDLE")
        self.Vbus_show = LineEdit()
        self.Vbus_show.setReadOnly(True)
        self.Vbus_show.setPlaceholderText("0.0 V")
        self.temp_show = LineEdit()
        self.temp_show.setReadOnly(True)
        self.temp_show.setPlaceholderText("0 °C")
        self.fault_warnning_show = LineEdit()
        self.fault_warnning_show.setReadOnly(True)
        self.fault_warnning_show.setText("None")


        status_top_row= QHBoxLayout()
        status_top_row.setContentsMargins(0, 0, 0, 0)
        status_top_row.setSpacing(8)

        status_bot_row = QHBoxLayout()
        status_bot_row.setContentsMargins(0, 0, 0, 0)
        status_bot_row.setSpacing(8)


        status_top_row.addWidget(self.sensormode_show,2)
        status_top_row.addWidget(self.Vbus_show,1)
        status_top_row.addWidget(self.state_show,3)

        status_bot_row.addWidget(self.runmode_show,2)
        status_bot_row.addWidget(self.temp_show,1)
        status_bot_row.addWidget(self.fault_warnning_show,3)



        status_layout.addLayout(status_top_row)
        status_layout.addLayout(status_bot_row)

        # 3. 复位按钮区域
        self.reset_card = CardWidget()
        reset_layout = QVBoxLayout(self.reset_card)
        reset_layout.setContentsMargins(8, 4, 8, 4)
        reset_layout.setSpacing(6)
        self.system_reset_button = PushButton()
        self.system_reset_button.setText("系统复位")
        self.reset_button = PushButton()
        self.reset_button.setText("状态复位")
        reset_layout.addWidget(self.system_reset_button)
        reset_layout.addWidget(self.reset_button)

        # 4. 配置管理区域
        self.config_card = CardWidget()
        config_layout = QVBoxLayout(self.config_card)
        config_layout.setContentsMargins(8, 4, 8, 4)
        config_layout.setSpacing(4)

        con_load_row = QHBoxLayout()
        con_load_row.setContentsMargins(0, 0, 0, 0)
        con_load_row.setSpacing(4)

        self.config_file = ComboBox()
        self.config_file.setMinimumWidth(100)
        self.config_file.addItem("默认")
        self.load_config = PushButton()
        self.load_config.setText("加载")

        con_load_row.addWidget(self.config_file,2)
        con_load_row.addWidget(self.load_config,1)

        config_btn_row = QHBoxLayout()
        config_btn_row.setContentsMargins(0, 0, 0, 0)
        config_btn_row.setSpacing(4)
        self.save_config = PushButton()
        self.save_config.setText("保存")
        self.remove_config = PushButton()
        self.remove_config.setText("删除")

        config_btn_row.addWidget(self.save_config)
        config_btn_row.addWidget(self.remove_config)

        config_layout.addLayout(con_load_row)
        config_layout.addLayout(config_btn_row)

        top_row.addWidget(self.connect_card, 2)
        top_row.addWidget(self.status_card, 4)
        top_row.addWidget(self.reset_card, 2)
        top_row.addWidget(self.config_card, 2)

        # 5. 主题配色区域
        self.theme_card = CardWidget()
        theme_layout = QVBoxLayout(self.theme_card)
        theme_layout.setContentsMargins(8, 4, 8, 4)
        theme_layout.setSpacing(4)

        self.theme_combo = ComboBox()
        self.theme_combo.setMinimumWidth(110)
        # 填充内置配色
        self._color_map = {}
        for name in dir(FluentThemeColor):
            if not name.startswith('_'):
                display = name.replace('_', ' ').title()
                self.theme_combo.addItem(display, userData=name)
                self._color_map[display] = name
        # 选中默认蓝色
        idx = self.theme_combo.findData('DEFAULT_BLUE')
        if idx >= 0:
            self.theme_combo.setCurrentIndex(idx)
        self.theme_combo.currentIndexChanged.connect(self._on_theme_color_changed)

        self.theme_toggle = PushButton()
        self.theme_toggle.setText("☀️ 亮色")
        self.theme_toggle.clicked.connect(self._on_theme_toggle)
        self._dark_mode = True

        # 颜色预览条
        self.color_preview = QFrame()
        self.color_preview.setFixedHeight(8)
        self.color_preview.setStyleSheet(
            f"background-color:{FluentThemeColor.DEFAULT_BLUE.value};border-radius:3px;"
        )

        theme_layout.addWidget(self.theme_combo)
        theme_layout.addWidget(self.color_preview)
        theme_layout.addWidget(self.theme_toggle)

        top_row.addWidget(self.theme_card, 1)
        main_layout.addLayout(top_row)

        # ── 第二行：快捷操作按钮（等宽居中） ──
        self.cmd_toolbar = CardWidget()
        cmd_layout = QHBoxLayout(self.cmd_toolbar)
        cmd_layout.setContentsMargins(8, 2, 8, 2)
        cmd_layout.setSpacing(24)

        self.ENable_button = PushButton()
        self.ENable_button.setText("使能")
        self.ENable_button.setFixedWidth(120)
        self.DEnable_button = PushButton()
        self.DEnable_button.setText("失能")
        self.DEnable_button.setFixedWidth(120)
        self.tunningstart_button = PushButton()
        self.tunningstart_button.setText("开始整定")
        self.tunningstart_button.setFixedWidth(120)
        self.brake_button = PushButton()
        self.brake_button.setText("制动")
        self.brake_button.setFixedWidth(120)
        self.pos_set_zero_button = PushButton()
        self.pos_set_zero_button.setText("设置零点")
        self.pos_set_zero_button.setFixedWidth(120)
        self.pos_set_limit_button = PushButton()
        self.pos_set_limit_button.setText("设置限位")
        self.pos_set_limit_button.setFixedWidth(120)

        # 两端弹簧撑开，按钮等距居中
        cmd_layout.addStretch(1)
        cmd_layout.addWidget(self.ENable_button)
        cmd_layout.addWidget(self.DEnable_button)
        cmd_layout.addWidget(self.tunningstart_button)
        cmd_layout.addWidget(self.brake_button)
        cmd_layout.addWidget(self.pos_set_zero_button)
        cmd_layout.addWidget(self.pos_set_limit_button)
        cmd_layout.addStretch(1)

        main_layout.addWidget(self.cmd_toolbar)

    # ── 主题配色事件 ──

    def _on_theme_color_changed(self, index):
        """主题色下拉变更"""
        name = self.theme_combo.itemData(index)
        if name and hasattr(FluentThemeColor, name):
            color = getattr(FluentThemeColor, name).value
            from PyQt5.QtGui import QColor
            from qfluentwidgets import qconfig, QConfig
            # 直接设置到 qconfig 并刷新
            qconfig.set(qconfig.themeColor, QColor(color), save=True)
            # 更新颜色预览条
            self.color_preview.setStyleSheet(
                f"background-color:{color};border-radius:3px;"
            )
            # 强制应用样式（lazy=False 确保立即刷新）
            from qfluentwidgets.common.style_sheet import updateStyleSheet
            updateStyleSheet(lazy=False)
            # 通知所有窗口主题已变更
            qconfig.themeChangedFinished.emit()
            # 强制重绘顶层窗口
            top = self.window()
            if top:
                top.style().unpolish(top)
                top.style().polish(top)
                top.update()

    def _on_theme_toggle(self):
        """明暗主题切换"""
        self._dark_mode = not self._dark_mode
        if self._dark_mode:
            setTheme(Theme.DARK)
            self.theme_toggle.setText("☀️ 亮色")
        else:
            setTheme(Theme.LIGHT)
            self.theme_toggle.setText("🌙 暗色")
        # 强制刷新所有控件样式
        from qfluentwidgets.common.style_sheet import updateStyleSheet
        updateStyleSheet(lazy=False)
        # 通知主题变更
        from qfluentwidgets import qconfig
        qconfig.themeChangedFinished.emit()
        # 强制重绘顶层窗口
        top = self.window()
        if top:
            top.style().unpolish(top)
            top.style().polish(top)
            top.update()
        # 切换后重选当前颜色确保正确应用
        idx = self.theme_combo.currentIndex()
        self._on_theme_color_changed(idx)
