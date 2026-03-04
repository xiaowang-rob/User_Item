from PyQt5.QtCore import Qt
from PyQt5.QtGui import QColor
from PyQt5.QtWidgets import QWidget, QHBoxLayout

from siui.components import SiDenseHContainer, SiLabel
from siui.components.widgets.abstracts.navigation_bar import ABCSiNavigationBar
from siui.core import SiColor
from siui.components.button import SiFlatButton


class SiNavigationBarH(ABCSiNavigationBar):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.item_dict = {}
        self.is_no_indicator = False

        # === 方案2：创建居中布局容器 ===
        self.center_container = QWidget(self)
        self.center_layout = QHBoxLayout(self.center_container)
        self.center_layout.setContentsMargins(0, 0, 0, 0)
        self.center_layout.setSpacing(2)
        self.center_layout.addStretch()  # 左侧弹簧

        # 按钮容器（嵌入居中布局）
        self.item_container = SiDenseHContainer(self.center_container)
        self.item_container.setFixedHeight(32)
        self.item_container.setSpacing(16)
        self.center_layout.addWidget(self.item_container)
        self.center_layout.addStretch()  # 右侧弹簧

        # 指示器区域（保持底部）
        self.indicator_frame = QWidget(self)
        self.indicator_frame.setFixedHeight(6)
        self.indicator_frame.setStyleSheet("background: transparent;")

        self.indicator_track = SiLabel(self.indicator_frame)
        self.indicator_track.setFixedHeight(2)
        self.indicator_track.move(0, 2)
        self.indicator_track.setFixedStyleSheet("border-radius: 1px")

        self.indicator = SiLabel(self.indicator_frame)
        self.indicator.setFixedHeight(6)
        self.indicator.resize(32, 6)
        self.indicator.setFixedStyleSheet("border-radius: 3px")

        # 启用样式背景绘制（关键！）
        self.indicator.setAttribute(Qt.WA_StyledBackground, True)
        self.indicator_track.setAttribute(Qt.WA_StyledBackground, True)

        self.indexChanged.connect(self._on_index_changed)
        self.setMinimumHeight(38)  # 32(按钮) + 6(指示器)

    def container(self):
        return self.item_container

    def indicatorFrame(self):
        return self.indicator_frame

    def setNoIndicator(self, state):
        self.is_no_indicator = state
        self.indicatorFrame().setVisible(not state)

    def adjustSize(self):
        min_height = 32 + 6 * int(not self.is_no_indicator)  # 38px
        self.resize(self.container().width() + 40, max(self.height(), min_height))  # +40 为左右弹簧留空间

    def addItem(self, name, side="left"):
        new_index = self.maximumIndex() + 1

        def on_clicked():
            self._on_button_clicked(new_index)

        button = SiFlatButton(self)
        button.setText(name)
        button.style_data.text_color = QColor("#C58BC2")      # 未选中色
        button.style_data.button_color = QColor(0, 0, 0, 0)   # 透明背景
        button.adjustSize()
        button.clicked.connect(on_clicked)

        self.item_container.addWidget(button, side)
        self.item_dict[str(new_index)] = button
        self.setMaximumIndex(new_index)

    def _on_button_clicked(self, index):
        self.setCurrentIndex(index)

    def _on_index_changed(self, index):
        # 1. 重置所有按钮为未选中状态
        for btn in self.item_dict.values():
            btn.style_data.text_color = QColor("#C58BC2")  # 未选中：#C58BC2
            btn.update()

        # 2. 激活当前选中按钮
        button = self.item_dict[str(index)]
        button.style_data.text_color = QColor("#FFFFFF")   # 选中：白色
        button.update()

        # 3. 计算指示器宽度（按钮宽度的 61.8%）
        width = int(button.width() * 0.618)

        # 4. 关键：计算按钮在导航栏中的绝对 x 位置
        #    - button.x()         : 相对于 item_container 的 x
        #    - item_container.x() : 相对于 center_container 的 x（通常为0）
        #    - center_container.x(): 相对于导航栏的 x（居中偏移量）
        button_abs_x = (button.x() + 
                        self.item_container.x() + 
                        self.center_container.x())

        # 5. 指示器居中于按钮下方
        indicator_x = button_abs_x + (button.width() - width) // 2

        # 6. 定位指示器（y=0 因为 indicator_frame 已在底部）
        self.indicator.move(indicator_x, 0)
        self.indicator.resize(width, 6)

        # 7. 强制设置背景色（绕过颜色系统问题）
        self.indicator.setStyleSheet("background-color: #C58BC2; border-radius: 3px;")
        self.indicator_track.setStyleSheet("background-color: #4A4450; border-radius: 1px;")

    def reloadStyleSheet(self):
        super().reloadStyleSheet()
        # 硬编码颜色（避免 QGradient 问题）
        self.indicator.setStyleSheet("background-color: #C58BC2; border-radius: 3px;")
        self.indicator_track.setStyleSheet("background-color: #4A4450; border-radius: 1px;")

    def resizeEvent(self, event):
        super().resizeEvent(event)
        
        # 1. 按钮区域：高度32，顶部对齐，宽度=导航栏宽度
        self.center_container.setGeometry(0, 0, self.width(), 32)
        
        # 2. 指示器区域：高度6，底部对齐
        self.indicator_frame.setGeometry(0, self.height() - 6, self.width(), 6)
        self.indicator_track.resize(self.width(), 2)
        
        # 3. 更新指示器位置
        if self.item_dict:
            self._on_index_changed(self.currentIndex())

    def showEvent(self, a0):
        super().showEvent(a0)
        if self.item_dict:
            button = self.item_dict[str(self.currentIndex())]
            button.style_data.text_color = QColor("#FFFFFF")
            button.update()
            self._on_index_changed(self.currentIndex())