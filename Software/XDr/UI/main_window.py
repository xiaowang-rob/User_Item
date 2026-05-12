from PyQt5.QtWidgets import QMainWindow
from UI.Ui_mainwindow import Ui_XDr
from siui.gui.icons.parser import GlobalIconPack
from siui.components.widgets.abstracts import SiWidget
from siui.templates.application.components.layer.layer_child_page.layer_child_page import (
    LayerChildPage,
)
from .top_area import TopArea
from siui.core import SiGlobal
from siui.components.tooltip import ToolTipWindow
from functons.message_show import init_message_system
from functons.com_port import ComPort
from .middle_area import MiddleArea
from .data_page import DataPage
from .control_page import ControlPage
from .IAP_widget import DownloadPage

from functons.wave import Wave
from .data_ui_map import Data_UI_Map
from functons.data_show import DataShow
from functons.log import LogManager
from functons.parampeter import ParameterManager
from functons.config import Pconfig
from functons.quick_but import QuickBut
from functons.IAP_downloader import IAP_downloader


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        # 全局变量初始化
        self.system_message = "无"

        # 全局变量初始化

        # 图标
        self.icon = GlobalIconPack()
        # 注册自己为 MAIN_WINDOW
        SiGlobal.siui.windows["MAIN_WINDOW"] = self
        tool_tip = ToolTipWindow()
        tool_tip.reloadStyleSheet()  # 应用样式（颜色等）
        SiGlobal.siui.windows["TOOL_TIP"] = tool_tip
        tool_tip.show()  # 调用 QWidget.show()
        tool_tip.setOpacity(0)  # 初始透明（等待 hover 时 fade in）

        init_message_system(self)  # 初始化消息系统

        # 主要布局
        self.ui = Ui_XDr()
        self.ui.setupUi(self)

        # 区域UI初始化
        self.top_area = TopArea(self)
        self.mid_area = MiddleArea(self)
        self.data_page = DataPage(self)
        self.control_page = ControlPage(self)
        self.download_page = DownloadPage(self)

        # 构建二级消息子页面界面
        self.layer_child_page = LayerChildPage(self)
        self.layer_child_page.raise_()  # 置顶

        # 控件映射表
        self.ui_map = Data_UI_Map(self)

        # 功能初始化
        self.comport = ComPort(self)
        self.wave = Wave(self)
        self.data_show = DataShow(self)
        self.log = LogManager(self)
        self.param_manager = ParameterManager(self, self.comport)
        self.config = Pconfig(self)
        self.quick_but = QuickBut(self, self.comport)
        self.IAP = IAP_downloader(self)

    def showChildPage(self):
        self.layer_child_page.setChildPage(self.download_page)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        # 当窗口大小改变时，调整层的大小以覆盖整个窗口
        self.layer_child_page.resize(event.size())
