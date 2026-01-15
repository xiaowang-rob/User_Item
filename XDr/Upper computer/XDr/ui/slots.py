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
        
        # 参数批量操作
        ui.all_write.clicked.connect(self.pm.write_all)
        ui.all_read.clicked.connect(self.pm.read_all)
        ui.all_save.clicked.connect(self.pm.save_flash)
        ui.all_erase.clicked.connect(self.pm.erase_flash)
        
        # 快捷动作
        ui.Enable.clicked.connect(lambda: self.mw.com_port.send_packet(Cmd.ENABLE, bytes([0x01])))
        ui.Disable.clicked.connect(lambda: self.mw.com_port.send_packet(Cmd.DISABLE, bytes([0x01])))

    def _handle_conn(self):
        btn = self.mw.ui.connectbutton
        if btn.isChecked():
            if self.mw.com_port.connect():
                btn.setText("断开")
                self.mw.com_port.send_packet(Cmd.UC_CONNECT, bytes([0x01]))
            else:
                btn.setChecked(False)
        else:
            self.mw.com_port.send_packet(Cmd.UC_DISCONNECT, bytes([0x01]))
            self.mw.com_port.disconnect()
            btn.setText("连接")