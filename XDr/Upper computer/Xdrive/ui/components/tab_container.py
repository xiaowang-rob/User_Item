# components/tab_container.py
from siui.components.widgets import SiWidget
from siui.components.widgets import SiDenseVContainer
from siui.core import SiGlobal

from ..tabs.params_tab import ParamsTab
from ..tabs.logs_tab import LogsTab
from ..tabs.control_tab import ControlTab

class TabContainer(SiWidget):
    def __init__(self, parent):
        super().__init__(parent)
        self.parent = parent
        
        # 从 SiGlobal 颜色组获取背景色
        bg_color = SiGlobal.siui.colors["tab_background"]
        self.setStyleSheet(f"background-color: {bg_color};")
        
        # 创建垂直容器
        self.container = SiDenseVContainer(self)
        self.container.setSpacing(0)
        
        # Tab栏（假设你有标签页导航栏）
        # Tab页面
        self.params_tab = ParamsTab(self)
        self.logs_tab = LogsTab(self)
        self.control_tab = ControlTab(self)
        
        # 显示第一个tab
        self.params_tab.show()
        self.logs_tab.hide()
        self.control_tab.hide()
    
    def resizeEvent(self, event):
        super().resizeEvent(event)
        
        # 调整容器大小
        self.container.setGeometry(0, 0, self.width(), self.height())
        
        # 调整Tab内容大小（占用全部空间）
        self.params_tab.resize(self.width(), self.height())
        self.logs_tab.resize(self.width(), self.height())
        self.control_tab.resize(self.width(), self.height())