from functons.message_show import (
    send_simple_message,
    send_titled_message,
    MSG_TYPE_NORMAL,
    MSG_TYPE_SUCCESS,
    MSG_TYPE_INFO,
    MSG_TYPE_WARNING,
    MSG_TYPE_ERROR,
)
import struct
from shared_constants import Cidx


class QuickBut:
    def __init__(self, main_window, comport):
        self.mw = main_window
        self.com = comport

        self.driver_message = self.mw.top_area.system_message
        self.driver_message.clicked.connect(self._handleSystemMessage)

        self.foc_enable = self.mw.mid_area.ENable_button
        self.foc_disable = self.mw.mid_area.DEnable_button
        self.foc_reset = self.mw.top_area.reset_button
        self.foc_tunningstart = self.mw.mid_area.tunningstart_button
        self.foc_brake = self.mw.mid_area.brake_button

        self.sys_reset = self.mw.top_area.system_reset_button
        self.set_zero_pos = self.mw.mid_area.pos_set_zero_button
        self.set_limit_pos = self.mw.mid_area.pos_set_limit_button

        self.foc_enable.clicked.connect(self.enable_button_clicked)
        self.foc_disable.clicked.connect(self.disable_button_clicked)
        self.foc_reset.clicked.connect(self.reset_button_clicked)
        self.foc_tunningstart.clicked.connect(self.tunningstart_button_clicked)
        self.foc_brake.clicked.connect(self.brake_button_clicked)

        self.sys_reset.clicked.connect(self.system_reset_button_clicked)
        self.set_zero_pos.clicked.connect(self.set_zero_position_button_clicked)
        self.set_limit_pos.clicked.connect(self.set_limit_position_button_clicked)

        self.MAX_val_input = self.mw.control_page.MAX_value
        self.value_slider = self.mw.control_page.value_slider
        self.target_val_input = self.mw.control_page.target_value
        self.write_val_button = self.mw.control_page.write_value_button
        self.target_show = self.mw.control_page.control_target_show

        self.MAX_val_input.textChanged.connect(self.MAX_value_changed)
        self.value_slider.valueChanged.connect(self.value_slider_changed)
        self.write_val_button.clicked.connect(self.write_value)

    def _handleSystemMessage(self):
        send_titled_message(MSG_TYPE_INFO, "设备信息", self.mw.system_message)

    def enable_button_clicked(self):
        self.com.send_packet(Cidx.ENABLE, bytes())

    def disable_button_clicked(self):
        self.com.send_packet(Cidx.DISABLE, bytes())

    def reset_button_clicked(self):
        self.com.send_packet(Cidx.FOC_NRST, bytes())

    def tunningstart_button_clicked(self):
        self.com.send_packet(Cidx.START_TUNNING, bytes())

    def brake_button_clicked(self):
        self.com.send_packet(Cidx.BRAKE, bytes())

    def system_reset_button_clicked(self):
        self.com.send_packet(Cidx.CMD_SYSTEM_RESET, bytes())

    def set_zero_position_button_clicked(self):
        self.com.send_packet(Cidx.CMD_SET_ZERO_POS, bytes())

    def set_limit_position_button_clicked(self):
        self.com.send_packet(Cidx.CMD_SET_LIMIT_POS, bytes())

    def value_slider_mapping(self, rel_value):
        max_val = float(self.MAX_val_input.text())
        return int((rel_value / 1000) * max_val)

    def MAX_value_changed(self, text):
        if text == "-":
            return
        try:
            val = abs(float(text))
            if val < 1:
                self.MAX_val_input.setText(str(10))
                return
            self.target_val_input.setText(str(0))
            self.value_slider.setValue(self.value_slider_mapping(0))
        except (ValueError, TypeError):
            self.MAX_val_input.setText(str(10))

    def value_slider_changed(self, value):
        val = float(value / 1000) * float(self.MAX_val_input.text())
        val = float(f"{val:.3g}")
        self.value_slider.setText(str(val))
        self.target_val_input.setText(str(val))
        self.send_val_ref()

    def write_value(self):
        max_val = float(self.MAX_val_input.text())
        target_val = float(self.target_val_input.text())
        if abs(target_val) > abs(max_val):
            target_val = max_val
        self.value_slider.setValue(int(abs(target_val) / abs(max_val) * 1000))
        self.target_val_input.setText(str(f"{target_val:.3g}"))
        self.send_val_ref()

    def send_val_ref(self):
        value = float(self.target_val_input.text())
        val_ref = struct.pack("<f", value)
        self.com.send_packet(Cidx.CMD_REFVALUE_SET, val_ref)
