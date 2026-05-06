from shared_constants import Midx, Sidx


class DataShow:
    def __init__(self, main_window):
        self.mw = main_window
        self.status_map = self.mw.ui_map.status_map
        self.com = self.mw.comport

        self.data = [0.0] * 26
        self.showindex = []
        self.channel_index = []

    def set_status(self, index, data):
        if index == 0:
            self.data[index] = Midx.sys_state[int(data)]
        elif index == 1:
            self.data[index] = Midx.foc_state[int(data)]
        elif index == 2:
            self.data[index] = Midx.fault_state[int(data)]
        elif index == 3:
            self.data[index] = Midx.warning_state[int(data)]
        else:
            self.data[index] = data

    def set_data(self, index, data):
        self.data[self.showindex[index] + 3] = data
        print(self.showindex[index] + 3, data)
        print(self.data[self.showindex[index] + 3])

    def get_data(self, index):
        return self.data[index]

    def show_status(self):
        # 状态显示
        self.status_map[Sidx.SYSTEM_state].setText(self.data[Sidx.SYSTEM_state])
        self.status_map[Sidx.FOC_state].setText(self.data[Sidx.FOC_state])
        self.status_map[Sidx.FAULT].setText(self.data[Sidx.FAULT])
        self.status_map[Sidx.WARNING].setText(self.data[Sidx.WARNING])
        self.status_map[Sidx.TEMPERATURE].setText(
            f"{self.data[Sidx.TEMPERATURE]:.2f} °C"
        )
        self.status_map[Sidx.VBUS].setText(f"{self.data[Sidx.VBUS]:.2f} V")

    def show_data(self):
        # 自定义波形显示

        for i in range(len(self.showindex)):
            # 注意：确保 self.data 索引不越界
            data_index = self.showindex[i] + 3
            self.mw.Wave.add_data(self.channel_index[i], self.data[data_index])
