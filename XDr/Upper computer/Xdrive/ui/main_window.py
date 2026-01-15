# ui/main_window.py
from siui.components.widgets import SiWidget
from siui.components.widgets import SiDenseVContainer
from siui.core import SiGlobal
from .components.header_bar import HeaderBar
from .components.tab_container import TabContainer

class MainWindowContent(SiWidget):
    def __init__(self, parent):
        super().__init__(parent)
        
        # 从 SiGlobal 颜色组获取背景色
        bg_color = SiGlobal.siui.colors["main_background"]
        self.setStyleSheet(f"background-color: {bg_color};")
        
        # 创建主垂直容器
        self.layout = SiDenseVContainer(self)
        self.layout.setSpacing(0)
        self.layout.setAdjustWidgetsSize(True)
        
        # 顶部控制栏（自适应高度）
        self.header = HeaderBar(self)
        
        # 底部Tab容器（占用剩余空间）
        self.tab_container = TabContainer(self)
        
        # 关键：header放在top，tab_container放在bottom
        # 这样header占用实际所需高度，tab_container占用剩余空间
        self.layout.addWidget(self.header, side="top")
        self.layout.addWidget(self.tab_container, side="bottom")
        
        # 设置容器几何尺寸
        self.layout.setGeometry(0, 0, parent.width(), parent.height())

    def resizeEvent(self, event):
        super().resizeEvent(event)
        self.layout.setGeometry(0, 0, self.width(), self.height())