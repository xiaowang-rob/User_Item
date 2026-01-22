from .parameter import Cmd


class status:
    sys_state=["INIT","RUN","ERROR"]
    foc_state=["IDLE","TUNE","RESET","ENABLE","DISABLE","RUNNING","SHUTDOWN","FAULT","WARNING"]
    fault_state = [
    "无故障",
    "闪存离线",
    "整定超时",
    "极对数不匹配",
    "电机参数异常",
    "过压",
    "低电压",
    "过流",
    "CAN初始化失败",
    "CAN通信失败"
]
    warning_state = [
    "无警告",
    "过温",
    "超速",
    "位置超限",
    "编码器离线",
    "编码器通信错误"
]
class DataIndex:
    SYSTEM_state = 0
    FOC_state = 1
    FAULT = 2
    WARNING = 3
    TEMPERATURE = 4 
    VBUS = 5
    VOLTAGE_U = 6
    VOLTAGE_V = 7
    VOLTAGE_W = 8
    VOLTAGE_q = 9
    VOLTAGE_d = 10
    CURRENT_U = 11
    CURRENT_V = 12
    CURRENT_W = 13
    CURRENT_q = 14
    CURRENT_d = 15
    CURRENT_q_ref = 16
    CURRENT_d_ref = 17
    SPEED = 18
    SPEED_con = 19
    SPEED_ref = 20
    THETA_elec = 21
    THETA_mech = 22
    POSITION = 23
    POSITION_con = 24
    POSITION_ref = 25

class Data:
    def __init__(self, main_window):
        self.mw = main_window
        self.data=[0.0]*26
        self.showindex=[]
        self.channal_index=[]
        options = ["NONE"] + [
        "温度",
        "Vbus",
        "VOL_U",
        "VOL_V",
        "VOL_W",
        "VOL_q",
        "VOL_d",
        "CUR_U",
        "CUR_V",
        "CUR_W",
        "CUR_q",
        "CUR_d",
        "CUR_q_ref",
        "CUR_d_ref",
        "SPE",
        "SPE_con",
        "SPE_ref",
        "THE_elec",
        "THE_mech",
        "POS",
        "POS_con",
        "POS_ref"
    ]

        # 给 combo_ch1 ~ combo_ch5 设置相同选项
        for combo in [
            self.mw.ui.combo_ch1,
            self.mw.ui.combo_ch2,
            self.mw.ui.combo_ch3,
            self.mw.ui.combo_ch4,
            self.mw.ui.combo_ch5,
        ]:
            combo.clear()
            combo.addItems(options)

    def set_status(self, index, data):
        if index==0:
            self.data[index] = status.sys_state[int(data)]
        elif index==1:
            self.data[index] = status.foc_state[int(data)]
        elif index==2:
            self.data[index] = status.fault_state[int(data)]
        elif index==3:
            self.data[index] = status.warning_state[int(data)]
        else:
            self.data[index] = data       
    def set_data(self,index, data):
        self.data[self.showindex[index]+3]=data
        print(self.showindex[index]+3,data)
        print(self.data[self.showindex[index]+3])
    def get_data(self, index):
        return self.data[index]
    
    def show_status(self):
        # 状态显示
        self.mw.ui.systemstateshow.setText(self.data[0])
        self.mw.ui.FOCstateshow.setText(self.data[1])
        self.mw.ui.erroeshow.setText(self.data[2])
        self.mw.ui.warnningshow.setText(self.data[3])
        self.mw.ui.tempshow.setText(f"{self.data[4]:.2f}")
        self.mw.ui.voltageshow.setText(f"{self.data[5]:.2f}")        

    def show_data(self):
        # 自定义波形显示
        
        for i in range(len(self.showindex)):
            # 注意：确保 self.data 索引不越界
            data_index = self.showindex[i] + 3
            self.mw.waveform_widget.add_waveform_data(self.channal_index[i], self.data[data_index])


    def send_stream_id(self):
        combo_boxes = [
            self.mw.ui.combo_ch1,
            self.mw.ui.combo_ch2,
            self.mw.ui.combo_ch3,
            self.mw.ui.combo_ch4,
            self.mw.ui.combo_ch5,
        ]
        self.showindex.clear()
        self.channal_index.clear()
        for  i,combo in enumerate(combo_boxes):
            current_text = combo.currentText()
            if current_text != "NONE":
                self.showindex.append(combo.currentIndex()) 
                self.channal_index.append(i)
        print(self.channal_index)
        self.mw.com_port.send_packet(Cmd.CMD_STREAM_SET, bytes(self.showindex))

    def send_none_stream(self):
        self.showindex.clear()
        self.mw.com_port.send_packet(Cmd.CMD_STREAM_SET, bytes())
        