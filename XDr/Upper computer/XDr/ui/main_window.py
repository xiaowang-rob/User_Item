from PyQt5.QtWidgets import QMainWindow
from ui.UI_XDr import Ui_XDr
from ui.slots import Slots
from functions.com_port import ComPort
from functions.parameter import ParameterManager
from functions.data_process import DataProcess
from functions.data_show import Data
from functions.wave import WaveformWidget
from PyQt5.QtWidgets import QVBoxLayout
from functions.log import LogManager
from functions.config import Pconfig

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        # 1. 载入UI控件
        self.ui = Ui_XDr()
        self.ui.setupUi(self)
        
        # 2. 实例化功能模块
        self.com_port = ComPort(self)
        self.data_processor = DataProcess(self)
        self.param_manager = ParameterManager(self, self.com_port)
        self.pconfig=Pconfig(self)
        self.data = Data(self)
        self.log=LogManager(self, self.com_port)

        # 3. 添加波形显示组件
        self.waveform_widget = WaveformWidget()
        # 将波形显示组件添加到你的空白QWidget容器中
        layout = QVBoxLayout(self.ui.waveformshow)  # 假设waveformshow是QFrame或QWidget
        layout.addWidget(self.waveform_widget)
        
        # 4. 连接信号
        self.com_port.packet_valid.connect(self.data_processor.handle_received_data)
        
        # 5. 初始化信号槽管理
        self.slots = Slots(self, self.param_manager)