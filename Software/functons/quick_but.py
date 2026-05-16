"""快捷操作按钮绑定 —— 电机控制与数值设定"""

import struct
import logging
from functons.message_show import (
    send_simple_message,
    send_titled_message,
    MSG_TYPE_INFO,
    MSG_TYPE_SUCCESS,
)
from protocol import Cidx

# ---------- 日志配置 ----------
logger = logging.getLogger("QuickBut")
logger.setLevel(logging.DEBUG)
if not logger.handlers:
    _handler = logging.StreamHandler()
    _handler.setFormatter(logging.Formatter("[%(levelname)s] %(name)s: %(message)s"))
    logger.addHandler(_handler)


class QuickBut:
    """快捷操作按钮 —— 控制/调谐/复位/数值设定"""

    def __init__(self, main_window, comport):
        self.mw = main_window
        self.com = comport

        self.driver_message = self.mw.top_area.system_message
        self.driver_message.clicked.connect(self._handleSystemMessage)

        # ── 控制按钮 ──
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

        # ── 数值控制 ──
        self.MAX_val_input = self.mw.control_page.MAX_value
        self.value_slider = self.mw.control_page.value_slider
        self.target_val_input = self.mw.control_page.target_value
        self.write_val_button = self.mw.control_page.write_value_button
        self.target_show = self.mw.control_page.control_target_show

        self.MAX_val_input.textChanged.connect(self.MAX_value_changed)
        self.value_slider.valueChanged.connect(self.value_slider_changed)
        self.write_val_button.clicked.connect(self.write_value)

    # ── 辅助 ──

    def _handleSystemMessage(self):
        """显示设备信息弹窗"""
        send_titled_message(MSG_TYPE_INFO, "设备信息", self.mw.system_message)

    def _log_cmd(self, name):
        """记录命令发送日志"""
        logger.info("发送指令: %s", name)

    # ── 电机控制指令 ──

    def enable_button_clicked(self):
        self.com.send_packet(Cidx.CMD_ENABLE, bytes())
        self._log_cmd("使能")

    def disable_button_clicked(self):
        self.com.send_packet(Cidx.CMD_DISABLE, bytes())
        self._log_cmd("失能")

    def reset_button_clicked(self):
        self.com.send_packet(Cidx.FOC_NRST, bytes())
        self._log_cmd("FOC 复位")

    def tunningstart_button_clicked(self):
        self.com.send_packet(Cidx.START_TUNNING, bytes())
        self._log_cmd("启动调谐")

    def brake_button_clicked(self):
        self.com.send_packet(Cidx.BRAKE, bytes())
        self._log_cmd("刹车")

    def system_reset_button_clicked(self):
        self.com.send_packet(Cidx.CMD_SYSTEM_RESET, bytes())
        self._log_cmd("系统复位")

    def set_zero_position_button_clicked(self):
        self.com.send_packet(Cidx.CMD_SET_ZERO_POS, bytes())
        self._log_cmd("归零")

    def set_limit_position_button_clicked(self):
        self.com.send_packet(Cidx.CMD_SET_LIMIT_POS, bytes())
        self._log_cmd("限位设定")

    # ── 数值控制 ──

    def value_slider_mapping(self, rel_value):
        """将滑块相对值 (0-1000) 映射到实际值"""
        max_val = float(self.MAX_val_input.text())
        return int((rel_value / 1000) * max_val)

    def MAX_value_changed(self, text):
        """最大值输入变更时重置滑块"""
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
        """滑块拖动 → 更新显示值并发送参考值"""
        val = float(value / 1000) * float(self.MAX_val_input.text())
        val = float(f"{val:.3g}")
        self.value_slider.setText(str(val))
        self.target_val_input.setText(str(val))
        self.send_val_ref()

    def write_value(self):
        """写入目标值（输入框确认）"""
        max_val = float(self.MAX_val_input.text())
        target_val = float(self.target_val_input.text())
        if abs(target_val) > abs(max_val):
            target_val = max_val
        self.value_slider.setValue(int(abs(target_val) / abs(max_val) * 1000))
        self.target_val_input.setText(str(f"{target_val:.3g}"))
        self.send_val_ref()

    def send_val_ref(self):
        """发送目标参考值到设备"""
        value = float(self.target_val_input.text())
        val_ref = struct.pack("<f", value)
        self.com.send_packet(Cidx.CMD_REFVALUE_SET, val_ref)
        logger.debug("发送参考值: %.3f", value)
