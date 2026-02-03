from functions.parameter import Cmd
import struct

from PyQt5.QtWidgets import QMessageBox
class Slots:
    def __init__(self, main_window, param_manager):
        self.mw = main_window
        self.pm = param_manager
        self._bind_slots()
        self.mw.ui.tabpage.setCurrentIndex(0)

    def _bind_slots(self):
        ui = self.mw.ui
        
        # 连接设备区域
        ui.connectbutton.clicked.connect(self._handle_conn)
        ui.systemdesc.clicked.connect(self._handle_system_message)

        # 参数配置文件
        ui.loadconfigfile.clicked.connect(self._handle_load_config_file)
        ui.saveconfigfile.clicked.connect(self._handle_save_config_file)
        ui.removeconfigfile.clicked.connect(self._handle_remove_config_file)
        
        # 快捷动作
        ui.Enable.clicked.connect(lambda: self.mw.com_port.send_packet(Cmd.ENABLE, bytes()))
        ui.Disable.clicked.connect(lambda: self.mw.com_port.send_packet(Cmd.DISABLE, bytes()))
        ui.Tunningstart.clicked.connect(lambda: self.mw.com_port.send_packet(Cmd.START_TUNNING, bytes()))
        ui.FOCreset.clicked.connect(lambda: self.mw.com_port.send_packet(Cmd.FOC_NRST, bytes()))
        ui.Brake.clicked.connect(lambda: self.mw.com_port.send_packet(Cmd.BRAKE, bytes()))
        ui.Protectreset.clicked.connect(lambda: self.mw.com_port.send_packet(Cmd.PROTECT_RESET, bytes()))

        # 导航按钮
        ui.tabbutton_parameterset.toggled.connect(self._handle_param_page)
        ui.tabbutton_log.toggled.connect(self._handle_log_page)
        ui.tabbutton_control.toggled.connect(self._handle_control_page)

        # 参数批量操作
        ui.all_write.clicked.connect(self.pm.write_all)
        ui.all_read.clicked.connect(self.pm.read_all)
        ui.all_save.clicked.connect(self.pm.save_flash)
        ui.all_erase.clicked.connect(self.pm.erase_flash)

        # 日志操作
        ui.log_read.clicked.connect(self._handle_log_read)
        ui.log_erase.clicked.connect(self._handle_log_erase)

        #示波界面
        ui.wave_start.clicked.connect(self._handle_wave_start)
        ui.wave_stop.clicked.connect(self._handle_wave_stop)
        ui.VAL_slider.valueChanged.connect(self._handle_val_slider)
        ui.VAL_write.clicked.connect(self._handle_val_write)
    def _handle_conn(self):
        btn = self.mw.ui.connectbutton
        if btn.text() == "连接":
            if self.mw.com_port.connect():
                btn.setText("断开")
                self.mw.com_port.send_packet(Cmd.UC_CONNECT, bytes())
                self.mw.ui.comboBox.setEnabled(False)
                print("连接成功")
            else:
                btn.setText("重试")
                print("连接失败")
                self.mw.ui.comboBox.setEnabled(True)
        else:
            self.mw.com_port.send_packet(Cmd.UC_DISCONNECT, bytes())
            self.mw.com_port.disconnect()
            btn.setText("连接")
            self.mw.ui.comboBox.setEnabled(True)
            print("断开连接")

    def _handle_system_message(self):
         QMessageBox.information(self.mw, "系统参数", self.mw.param_manager.system_desc)


    def _handle_load_config_file(self):
        self.mw.pconfig.load_selected_config()

    def _handle_save_config_file(self):
        self.mw.pconfig.save_current_config()

    def _handle_remove_config_file(self):
        self.mw.pconfig.delete_selected_config()



    def _handle_param_page(self,checked:bool):
        if not checked:
            return
        self.mw.ui.tabbutton_log.setChecked(False)
        self.mw.ui.tabbutton_control.setChecked(False)
        self.mw.ui.tabpage.setCurrentIndex(0)

    def _handle_log_page(self,checked:bool):
        if not checked:
            return
        self.mw.ui.tabbutton_parameterset.setChecked(False)
        self.mw.ui.tabbutton_control.setChecked(False)
        self.mw.ui.tabpage.setCurrentIndex(1)

    def _handle_control_page(self,checked:bool):
        if not checked:
            return
        self.mw.ui.tabbutton_parameterset.setChecked(False)
        self.mw.ui.tabbutton_log.setChecked(False)
        self.mw.ui.tabpage.setCurrentIndex(2)

    def _handle_wave_start(self):
        self.mw.data.send_stream_id()
        self.mw.waveform_widget.start_oscilloscope()
    def _handle_wave_stop(self):
        self.mw.data.send_none_stream()
        self.mw.waveform_widget.stop_oscilloscope

    def _handle_val_slider(self,value):
        min_val = int(self.mw.ui.VAL_min.text())
        max_val = int(self.mw.ui.VAL_max.text())
        self.mw.ui.VAL_slider.setMinimum(min_val*100)
        self.mw.ui.VAL_slider.setMaximum(max_val*100)
        val=value/1000*(max_val-min_val)+min_val
        self.mw.ui.VAL.setText(f"{val:.2g}")
        self.send_val_ref()

    def _handle_val_write(self):
        min_val=int(self.mw.ui.VAL_min.text())
        max_val=int(self.mw.ui.VAL_max.text())
        val=(float(self.mw.ui.VAL.text())-min_val)/(max_val-min_val)*1000
        self.mw.ui.VAL_slider.setValue(int(val))
        self.send_val_ref()

    def send_val_ref(self):
        idx=self.mw.ui.LOOPmode.currentIndex()
        match idx:
            case 0|1:#电流电压环
                value=float(self.mw.ui.VAL.text())
            case 2:#速度环
                value=self.rpm_to_rad( float(self.mw.ui.VAL.text()))
            case 3|4:#位置环
                value=self.deg_to_rad( float(self.mw.ui.VAL.text()))
        val_ref= struct.pack('<f', value)
        self.mw.com_port.send_packet(Cmd.CMD_REFVALUE_SET, val_ref)
    def rpm_to_rad(self, rpm):
        return rpm * 2 * 3.1415926 / 60
    
    def deg_to_rad(self, deg):
        return deg * 3.1415926 / 180
    
    def _handle_log_read(self):
        self.mw.ui.LogList.clear()
        self.mw.com_port.send_packet(Cmd.LOG_GET, bytes())

    def _handle_log_erase(self):
        self.mw.ui.LogList.clear()
        self.mw.com_port.send_packet(Cmd.LOG_ERASE, bytes())