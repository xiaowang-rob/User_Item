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
from .log_page import LogPage
from .control_page import ControlPage
from functons.wave import Wave
from .data_ui_map import Data_UI_Map
from functons.data_show import DataShow
from functons.log import LogManager
from functons.parampeter import ParameterManager
from functons.config import Pconfig
from functons.data_process import DataProcess
from functons.quick_but import QuickBut


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        #全局变量初始化
        self.system_message = "无"

        #全局变量初始化

        #图标
        self.icon=GlobalIconPack()
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

        #区域UI初始化
        self.top_area=TopArea(self)
        self.mid_area=MiddleArea(self)
        self.parameter_page=ParameterPage(self)
        self.log_page=LogPage(self)
        self.control_page=ControlPage(self)

        # 控件映射表
        self.ui_map=Data_UI_Map(self)

        #功能初始化
        self.comport=ComPort(self.top_area)
        self.data_process=DataProcess(self)
        self.wave = Wave(self) 
        self.data_show=DataShow(self)
        self.log=LogManager(self)
        self.param_manager=ParameterManager(self,self.comport)
        self.config=Pconfig(self)
        self.quick_but=QuickBut(self,self.comport)

        # 连接信号
        self.comport.packet_valid.connect(self.data_process.handle_received_data)

