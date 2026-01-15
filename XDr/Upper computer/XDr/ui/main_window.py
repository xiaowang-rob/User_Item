from PyQt5.QtWidgets import QMainWindow
from ui.UI_XDr import Ui_XDr
from ui.slots import Slots
from functions.com_port import ComPort
from functions.parameter import ParameterManager

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        # 1. 载入UI控件
        self.ui = Ui_XDr()
        self.ui.setupUi(self)
        
        # 2. 实例化功能模块
        # 将 self 传入以便功能模块访问 UI 控件
        self.com_port = ComPort(self)
        self.param_manager = ParameterManager(self, self.com_port)
        
        # 3. 初始化信号槽管理
        self.slots = Slots(self, self.param_manager)