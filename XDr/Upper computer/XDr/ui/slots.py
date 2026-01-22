from functions.parameter import Cmd

class Slots:
    def __init__(self, main_window, param_manager):
        self.mw = main_window
        self.pm = param_manager
        self._bind_slots()

    def _bind_slots(self):
        ui = self.mw.ui
        
        # 串口操作
        ui.connectbutton.clicked.connect(self._handle_conn)

        
        # 快捷动作
        ui.Enable.clicked.connect(lambda: self.mw.com_port.send_packet(Cmd.ENABLE, bytes()))
        ui.Disable.clicked.connect(lambda: self.mw.com_port.send_packet(Cmd.DISABLE, bytes()))
        ui.Tunningstart.clicked.connect(lambda: self.mw.com_port.send_packet(Cmd.START_TUNNING, bytes()))
        ui.FOCreset.clicked.connect(lambda: self.mw.com_port.send_packet(Cmd.FOC_NRST, bytes()))
        ui.Brake.clicked.connect(lambda: self.mw.com_port.send_packet(Cmd.BRAKE, bytes()))
        ui.Protectreset.clicked.connect(lambda: self.mw.com_port.send_packet(Cmd.PROTECT_RESET, bytes()))

        # 参数批量操作
        ui.all_write.clicked.connect(self.pm.write_all)
        ui.all_read.clicked.connect(self.pm.read_all)
        ui.all_save.clicked.connect(self.pm.save_flash)
        ui.all_erase.clicked.connect(self.pm.erase_flash)
        # 导航按钮
        ui.tabbutton_parameterset.clicked.connect(self._handle_param_page)
        ui.tabbutton_log.clicked.connect(self._handle_log_page)
        ui.tabbutton_control.clicked.connect(self._handle_control_page)

        #示波界面
        ui.wave_start.clicked.connect(self._handle_wave_start)
        ui.wave_stop.clicked.connect(self._handle_wave_stop)

    def _handle_conn(self):
        btn = self.mw.ui.connectbutton
        if btn.text() == "连接":
            if self.mw.com_port.connect():
                btn.setText("断开")
                self.mw.com_port.send_packet(Cmd.UC_CONNECT, bytes([]))
                self.mw.ui.comboBox.setEnabled(False)
                print("连接成功")
            else:
                btn.setText("重试")
                print("连接失败")
                self.mw.ui.comboBox.setEnabled(True)
        else:
            self.mw.com_port.send_packet(Cmd.UC_DISCONNECT, bytes([]))
            self.mw.com_port.disconnect()
            btn.setText("连接")
            self.mw.ui.comboBox.setEnabled(True)
            print("断开连接")

    def _handle_param_page(self):
        self.mw.ui.tabpage.setCurrentIndex(0)

    def _handle_log_page(self):
        self.mw.ui.tabpage.setCurrentIndex(1)

    def _handle_control_page(self):
        self.mw.ui.tabpage.setCurrentIndex(2)

    def _handle_wave_start(self):
        self.mw.data.send_stream_id()
        self.mw.waveform_widget.start_oscilloscope()
    def _handle_wave_stop(self):
        self.mw.data.send_none_stream()
        self.mw.waveform_widget.stop_oscilloscope