from PyQt5.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QColor
from qfluentwidgets import (
    FluentWindow, NavigationItemPosition, FluentIcon as FIF,
    setTheme, Theme, setThemeColor, FluentThemeColor,
)

from app.status_bar import StatusBar
from app.data_page import DataPage
from app.control_page import ControlPage
from app.iap_page import IAPPage
from app.data_ui_map import Data_UI_Map

from functions.com_port import ComPort
from functions.wave import Wave
from functions.data_show import DataShow
from functions.log import LogManager
from functions.parampeter import ParameterManager
from functions.config import Pconfig
from functions.quick_but import QuickBut
from functions.IAP_downloader import IAP_downloader
from functions.message_show import init_message_system
from protocol import Cidx, Fidx
from functions.message_show import send_titled_message, MSG_TYPE_SUCCESS, MSG_TYPE_ERROR
import struct


class MainWindow(FluentWindow):
    """上位机主窗口 —— 基于 FluentWindow"""

    def __init__(self):
        super().__init__()

        # 系统信息
        self.system_message = "无"

        # 创建页面
        self.data_page = DataPage()
        self.control_page = ControlPage()
        self.download_page = IAPPage()

        # 创建顶部状态栏（含快捷按钮）并嵌入到内容区域顶部
        self.top_area = StatusBar(self)
        self.mid_area = self.top_area   # 向后兼容
        self._embed_status_bar()

        # 功能初始化
        self.ui_map = Data_UI_Map(self)
        self.comport = ComPort(self)
        self.wave = Wave(self)
        self.data_show = DataShow(self)
        self.log = LogManager(self)
        self.param_manager = ParameterManager(self, self.comport)
        self.config = Pconfig(self)
        self.quick_but = QuickBut(self, self.comport)
        self.IAP = IAP_downloader(self)

        # 注册命令处理器
        self._register_data_handlers()

        # 初始化导航
        self._init_navigation()

        # 初始化消息系统
        init_message_system(self)

        # 默认主题
        setTheme(Theme.DARK)
        setThemeColor(QColor(FluentThemeColor.DEFAULT_BLUE.value), save=False)

        # 设置窗口图标和标题
        from PyQt5.QtGui import QIcon
        self.setWindowIcon(QIcon("app/images/logo.png"))
        self.setWindowTitle("XDr")
        self.resize(1600, 960)

    def _embed_status_bar(self):
        """将状态栏嵌入到 FluentWindow 的内容区域顶部"""
        # widgetLayout 是 FluentWindow 内部的右内容区布局
        # 包含 stackedWidget，顶部有 48px 标题栏空间
        # 我们将 stackedWidget 从 widgetLayout 取出，
        # 放入一个 vertical 容器（status_bar + stackedWidget）
        self.widgetLayout.removeWidget(self.stackedWidget)

        container = QWidget()
        container_layout = QVBoxLayout(container)
        container_layout.setContentsMargins(0, 0, 0, 0)
        container_layout.setSpacing(0)

        # 状态栏在上
        self.top_area.setFixedHeight(130)
        container_layout.addWidget(self.top_area)
        # 页面在下
        container_layout.addWidget(self.stackedWidget)

        self.widgetLayout.addWidget(container)

    def _init_navigation(self):
        """初始化导航栏"""
        self.addSubInterface(
            self.data_page,
            FIF.SETTING,
            "数据参数",
            position=NavigationItemPosition.TOP,
        )
        self.addSubInterface(
            self.control_page,
            FIF.ZOOM,
            "波形控制",
            position=NavigationItemPosition.TOP,
        )
        self.addSubInterface(
            self.download_page,
            FIF.DOWNLOAD,
            "固件升级",
            position=NavigationItemPosition.TOP,
        )

    def switch_to_page(self, page_name):
        """切换到指定页面"""
        route_map = {
            "data": self.data_page.objectName(),
            "control": self.control_page.objectName(),
            "iap": self.download_page.objectName(),
        }
        route = route_map.get(page_name)
        if route:
            self.navigationInterface.navigate(route)

    def _register_data_handlers(self):
        """注册串口命令处理器"""
        c = self.comport

        # 断连时清除设备信息
        c.connection_lost.connect(lambda _, __: self.download_page.set_system_info(""))

        STATUS_FIELD_COUNT = 4 + 2  # tune/foc/fault/warning + temp/vbus

        def _on_connect(data):
            c.update_status_time()
            if len(data) == 12:
                try:
                    temp_val = struct.unpack("<f", data[4:8])[0]
                    vbus_val = struct.unpack("<f", data[8:12])[0]
                    for i in range(STATUS_FIELD_COUNT):
                        if i < 4:
                            self.data_show.set_status(i, data[i])
                        elif i == 4:
                            self.data_show.set_status(i, temp_val)
                        else:
                            self.data_show.set_status(i, vbus_val)
                    self.data_show.show_status()
                except Exception:
                    pass
            elif len(data) > 0:
                try:
                    msg = "".join(ch for ch in data.decode(errors="ignore") if 32 <= ord(ch) <= 126)
                    if not msg:
                        return
                    parts = msg.split(",")
                    if len(parts) >= 2:
                        self.IAP.set_current_version(parts[0].strip() + " " + parts[1].strip())
                    labels = ["Device", "Version", "Author", "PWM", "CurLoop",
                              "SpdLoop", "PosLoop", "MaxCur", "Vin", "MaxTemp"]
                    units = ["", "", "", "Hz", "Hz", "Hz", "Hz", "A", "V", "\u00b0C"]
                    lines = [f"{l}: {p}{u}" for l, p, u in zip(labels, parts, units)]
                    sys_info = "\n".join(lines)
                    self.system_message = sys_info
                    # 更新到 IAP 页
                    self.download_page.set_system_info(sys_info)
                except Exception:
                    pass
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
