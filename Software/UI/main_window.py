from PyQt5.QtWidgets import QMainWindow
from UI.Ui_mainwindow import Ui_XDr
from siui.gui.icons.parser import GlobalIconPack
from siui.templates.application.components.layer.layer_child_page.layer_child_page import (
    LayerChildPage,
)
from .top_area import TopArea
from siui.core import SiGlobal
from siui.components.tooltip import ToolTipWindow
from functions.message_show import init_message_system
from functions.com_port import ComPort
from .middle_area import MiddleArea
from .data_page import DataPage
from .control_page import ControlPage
from .IAP_widget import DownloadPage

from functions.wave import Wave
from .data_ui_map import Data_UI_Map
from functions.data_show import DataShow
from functions.log import LogManager
from functions.parampeter import ParameterManager
from functions.config import Pconfig
from functions.quick_but import QuickBut
from functions.IAP_downloader import IAP_downloader


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

        # Register command handlers (replaces giant match-case in com_port)
        self._register_data_handlers()

    def _register_data_handlers(self):
        from protocol import Cidx, Fidx
        from functions.message_show import send_titled_message, MSG_TYPE_SUCCESS, MSG_TYPE_ERROR
        import struct

        c = self.comport

        # UC_CONNECT: status or system info
        # 状态包字段数量：前4字节(int) + 2个float = 6个字段
        STATUS_FIELD_COUNT = 4 + 2  # tune/foc/fault/warning + temp/vbus

        def _on_connect(data):
            c.update_status_time()
            # 状态包固定 12 字节 (4 raw + 2 float)，其余为系统信息字符串
            if len(data) == 12:
                for i in range(STATUS_FIELD_COUNT):
                    if i < 4:
                        self.data_show.set_status(i, data[i])
                    else:
                        val = struct.unpack("<f", data[(i-3)*4:(i-2)*4])[0]
                        self.data_show.set_status(i, val)
                self.data_show.show_status()
            else:
                msg = "".join(ch for ch in data.decode(errors="ignore") if 32 <= ord(ch) <= 126)
                parts = msg.split(",")
                self.IAP.set_current_version(parts[0].strip() + " " + parts[1].strip())
                labels = ["Device", "Version", "Author", "PWM", "CurLoop",
                          "SpdLoop", "PosLoop", "MaxCur", "Vin", "MaxTemp"]
                units = ["", "", "", "Hz", "Hz", "Hz", "Hz", "A", "V", "\u00b0C"]
                lines = [f"{l}: {p}{u}" for l, p, u in zip(labels, parts, units)]
                self.system_message = "\n".join(lines)
        c.register_handler(Cidx.UC_CONNECT, _on_connect)

        c.register_handler(Cidx.LOG_GET, self.log.add_log)
        c.register_handler(Cidx.PARAM_READ, self.param_manager.add_param)

        def _feedback(ok_msg, fail_msg):
            def h(data):
                ok = data[0] == Fidx.FEEDBACK_EXECUTE
                send_titled_message(MSG_TYPE_SUCCESS if ok else MSG_TYPE_ERROR,
                    "OK" if ok else "ERR", ok_msg if ok else fail_msg, True, 1000)
            return h
        c.register_handler(Cidx.LOG_ERASE, _feedback("Logs cleared", "Log clear failed"))
        c.register_handler(Cidx.PARAM_ERASE, _feedback("Params cleared", "Param clear failed"))
        c.register_handler(Cidx.PARAM_SAVE, _feedback("Params saved", "Param save failed"))

        c.register_handler(Cidx.CMD_STREAM_SET, self.wave.handle_stream_data)


    def showChildPage(self):
        self.layer_child_page.setChildPage(self.download_page)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        # 当窗口大小改变时，调整层的大小以覆盖整个窗口
        self.layer_child_page.resize(event.size())
