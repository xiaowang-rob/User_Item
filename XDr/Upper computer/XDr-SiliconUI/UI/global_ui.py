from PyQt5.QtWidgets import QMainWindow
from UI.Ui_mainwindow import Ui_MainWindow
from siui.gui.icons.parser import GlobalIconPack
from .top_area import TopArea

from siui.core import SiGlobal
from siui.components.tooltip import ToolTipWindow
from functons.message_show import init_message_system
from functons.com_port import ComPort
from .middle_area import MiddleArea
from .parameter_page import ParameterPage

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        #全局变量初始化
        self.system_message = "无"


        #图标
        self.icon=GlobalIconPack()
        #全局显示初始化
        
        # 注册自己为 MAIN_WINDOW
        SiGlobal.siui.windows["MAIN_WINDOW"] = self
        tool_tip = ToolTipWindow()
        tool_tip.reloadStyleSheet()  # 应用样式（颜色等）
        SiGlobal.siui.windows["TOOL_TIP"] = tool_tip
        tool_tip.show()          # 调用 QWidget.show()
        tool_tip.setOpacity(0)   # 初始透明（等待 hover 时 fade in）

        init_message_system(self)  # 初始化消息系统


        #主要布局
        self.ui=Ui_MainWindow()
        self.ui.setupUi(self)

        #区域初始化
        self.top_area=TopArea(self)
        self.comport=ComPort(self.top_area)
        self.mid_area=MiddleArea(self)

        self.parameter_page=ParameterPage(self)


