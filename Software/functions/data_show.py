import logging
from protocol import Midx, Sidx

# ---------- 日志配置 ----------
logger = logging.getLogger("DataShow")
logger.setLevel(logging.DEBUG)
if not logger.handlers:
    _handler = logging.StreamHandler()
    _handler.setFormatter(logging.Formatter("[%(levelname)s] %(name)s: %(message)s"))
    logger.addHandler(_handler)


class DataShow:
    """实时状态与数据通道显示"""

    def __init__(self, main_window):
        self.mw = main_window
        self.status_map = self.mw.ui_map.status_map
        self.com = self.mw.comport

        # 状态缓存 [调谐, FOC, 故障, 警告, 温度, VBUS]
        self.status = [0.0] * 6
        self.data = [0.0]          # 波形通道数据（动态扩展）
        self.showindex = []         # 当前显示的索引
        self.channel_index = []     # 通道选择

    def set_status(self, index, data):
        """更新指定索引的状态值（枚举/数值）"""
        match index:
            case Sidx.TUNE_STATE:
                self.status[Sidx.TUNE_STATE] = Midx.tune_state[int(data)]
            case Sidx.FOC_STATE:
                self.status[Sidx.FOC_STATE] = Midx.foc_state[int(data)]
            case Sidx.FAULT:
                self.status[Sidx.FAULT] = Midx.fault_state[int(data)]
            case Sidx.WARNING:
                self.status[Sidx.WARNING] = Midx.warning_state[int(data)]
            case Sidx.TEMPERATURE | Sidx.VBUS:
                self.status[index] = data
        logger.debug(f"状态更新 idx={index} val={data}")

    def set_data(self, index, data):
        """设置波形通道数据（列表自动扩展）"""
        if index >= len(self.data):
            self.data.extend([0.0] * (index - len(self.data) + 1))
        self.data[index + 1] = data

    def get_data(self, index):
        """读取波形通道数据"""
        return self.data[index]

    def show_status(self):
        """刷新状态显示到 UI"""
        # FOC 状态（调谐中 / 运行中）
        if self.status[Sidx.FOC_STATE] == "TUNE":
            self.status_map[Sidx.TUNE_STATE].setText(self.status[Sidx.TUNE_STATE])
        else:
            self.status_map[Sidx.FOC_STATE].setText(self.status[Sidx.FOC_STATE])

        # 故障/警告
        if self.status[Sidx.FAULT] == "NONE":
            self.status_map[Sidx.WARNING].setTitle("警告")
            self.status_map[Sidx.WARNING].setText(self.status[Sidx.WARNING])
        else:
            self.status_map[Sidx.FAULT].setTitle("错误")
            self.status_map[Sidx.FAULT].setText(self.status[Sidx.FAULT])

        # 温度 / 电压
        self.status_map[Sidx.TEMPERATURE].setText(
            f"{self.status[Sidx.TEMPERATURE]:.2f} °C"
        )
        self.status_map[Sidx.VBUS].setText(f"{self.status[Sidx.VBUS]:.2f} V")
