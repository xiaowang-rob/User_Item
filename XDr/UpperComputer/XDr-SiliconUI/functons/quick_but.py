
from UI.data_ui_map import Cidx
from functons.message_show import (
    send_simple_message,
    send_titled_message,
    MSG_TYPE_NORMAL ,   
    MSG_TYPE_SUCCESS ,  
    MSG_TYPE_INFO ,     
    MSG_TYPE_WARNING, 
    MSG_TYPE_ERROR,    
)
import struct
from UI.data_ui_map import Cidx,Midx

class QuickBut:
    def __init__(self, main_window,comport):
        self.mw = main_window
        self.com = comport
        


        self.driver_message = self.mw.top_area.system_message
        self.driver_message.clicked.connect(self._handleSystemMessage)

        self.foc_enable = self.mw.mid_area.ENable_button
        self.foc_disable = self.mw.mid_area.DEnable_button
        self.foc_reset = self.mw.mid_area.reset_button
        self.foc_tunningstart = self.mw.mid_area.tunningstart_button
        self.foc_brake = self.mw.mid_area.brake_button
        self.foc_protectreset = self.mw.mid_area.protectreset_button

        self.foc_enable.clicked.connect(self.enable_button_clicked)
        self.foc_disable.clicked.connect(self.disable_button_clicked)
        self.foc_reset.clicked.connect(self.reset_button_clicked)
        self.foc_tunningstart.clicked.connect(self.tunningstart_button_clicked)
        self.foc_brake.clicked.connect(self.brake_button_clicked)
        self.foc_protectreset.clicked.connect(self.protectreset_button_clicked)


        self.MIN_val_input=self.mw.control_page.MIN_value
        self.MAX_val_input=self.mw.control_page.MAX_value
        self.value_slider=self.mw.control_page.value_slider
        self.target_val_input=self.mw.control_page.target_value
        self.write_val_button=self.mw.control_page.write_value_button
        self.target_show=self.mw.control_page.control_target_show

        self.MIN_val_input.textChanged.connect(self.MIN_value_changed)
        self.MAX_val_input.textChanged.connect(self.MAX_value_changed)
        self.value_slider.valueChanged.connect(self.value_slider_changed)
        self.write_val_button.clicked.connect(self.write_value)


    def _handleSystemMessage(self):
        send_titled_message(MSG_TYPE_INFO,"设备信息",self.mw.system_message)


    def  enable_button_clicked(self):
        self.com.send_packet(Cidx.ENABLE,bytes())

    def disable_button_clicked(self):
        self.com.send_packet(Cidx.DISABLE,bytes())

    def reset_button_clicked(self):
        self.com.send_packet(Cidx.FOC_NRST,bytes())

    def tunningstart_button_clicked(self):
        self.com.send_packet(Cidx.START_TUNNING,bytes())

    def brake_button_clicked(self):
        self.com.send_packet(Cidx.BRAKE,bytes())

    def protectreset_button_clicked(self):
        self.com.send_packet(Cidx.PROTECT_RESET,bytes())



    def value_slider_mapping(self, rel_value):
        min_val=float(self.MIN_val_input.text())
        max_val=float(self.MAX_val_input.text())
        return int((rel_value/1000)*(max_val-min_val)+min_val)

    def MIN_value_changed(self, text):
        try:
            val=float(text)
            if val>0:
                self.MIN_val_input.setText(str(0))
                return

            self.target_val_input.setText(str(0))
            self.value_slider.setValue(self.value_slider_mapping(0))
        except (ValueError, TypeError):
            self.MIN_val_input.setText(str(0))
    def MAX_value_changed(self, text):
        try:
            val=float(text)
            if val<1:
                self.MAX_val_input.setText(str(10))
                return
            self.target_val_input.setText(str(0))
            self.value_slider.setValue(self.value_slider_mapping(0))
        except (ValueError, TypeError):
            self.MAX_val_input.setText(str(10))

    def value_slider_changed(self, value):
        val=float(value/1000)*(float(self.MAX_val_input.text())-float(self.MIN_val_input.text()))+float(self.MIN_val_input.text())
        val=float(f"{val:.3g}")
        self.value_slider.setText(str(val))
        self.target_val_input.setText(str(val))
        self.send_val_ref()

    def write_value(self):
        min_val=float(self.MIN_val_input.text())
        max_val=float(self.MAX_val_input.text())
        target_val=float(self.target_val_input.text())
        if target_val<min_val:
            target_val=min_val
        if target_val>max_val:
            target_val=max_val
        self.value_slider.setValue(int((target_val-min_val)/(max_val-min_val)*1000))
        self.target_val_input.setText(str(f"{target_val:.3g}"))
        self.send_val_ref()

    def send_val_ref(self):
        value=float(self.target_val_input.text())
        val_ref= struct.pack('<f', value)
        self.com.send_packet(Cidx.CMD_REFVALUE_SET, val_ref)
