from shared_constants import Midx, Sidx, Didx


class DataShow:
    def __init__(self, main_window):
        self.mw = main_window
        self.status_map = self.mw.ui_map.status_map
        self.com = self.mw.comport

        self.status=[0.0] * 6
        self.data = [0.0]
        self.showindex = []
        self.channel_index = []

    def set_status(self, index, data):

        match index:
            case Sidx.TUNE_STATE:
                self.status[Sidx.TUNE_STATE] = Midx.tune_state[int(data)]
                return
            case Sidx.FOC_STATE:
                self.status[Sidx.FOC_STATE] = Midx.foc_state[int(data)]
                return
            case Sidx.FAULT:
                self.status[Sidx.FAULT] = Midx.fault_state[int(data)]
                return
            case Sidx.WARNING:
                self.status[Sidx.WARNING] = Midx.warning_state[int(data)]
                return
            case Sidx.TEMPERATURE | Sidx.VBUS:
                self.status[index] = data
                return

    def set_data(self, index, data):
        if index >= len(self.data):
            self.data.extend([0.0] * (index - len(self.data) + 1))
        self.data[index + 1] = data

    def get_data(self, index):
        return self.data[index]

    def show_status(self):
        # 状态显示
        if self.status[Sidx.FOC_STATE] == "TUNE":
            self.status_map[Sidx.TUNE_STATE].setText(self.status[Sidx.TUNE_STATE])
        else:
            self.status_map[Sidx.FOC_STATE].setText(self.status[Sidx.FOC_STATE])
        if self.status[Sidx.FAULT] == "NONE":
            self.status_map[Sidx.WARNING].setTitle("警告")
            self.status_map[Sidx.WARNING].setText(self.status[Sidx.WARNING])
        else:
            self.status_map[Sidx.FAULT].setTitle("错误")
            self.status_map[Sidx.FAULT].setText(self.status[Sidx.FAULT])
        self.status_map[Sidx.TEMPERATURE].setText(
            f"{self.status[Sidx.TEMPERATURE]:.2f} °C"
        )
        self.status_map[Sidx.VBUS].setText(f"{self.status[Sidx.VBUS]:.2f} V")

