"""快捷操作按钮绑定 —— 电机控制与数值设定"""

import struct
import logging
from functions.message_show import (
    send_simple_message,
    send_titled_message,
    MSG_TYPE_INFO,
    MSG_TYPE_SUCCESS,
)
from protocol import Cidx

# ---------- 日志配置 ----------
logger = logging.getLogger("QuickBut")
logger.setLevel(logging.INFO)
if not logger.handlers:
    _handler = logging.StreamHandler()
    _handler.setFormatter(logging.Formatter("[%(levelname)s] %(name)s: %(message)s"))
    logger.addHandler(_handler)


class QuickBut:
    """快捷操作按钮 —— 控制/调谐/复位/数值设定"""

    def __init__(self, main_window, comport):
        self.mw = main_window
        self.com = comport

        self.driver_message = None
        self.send_buf=[0,0]
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
        self.MAX_val_input_1 = self.mw.control_page.MAX_value_1
        self.MAX_val_input_2 = self.mw.control_page.MAX_value_2
        self.value_slider_1 = self.mw.control_page.value_slider_1
        self.value_slider_2 = self.mw.control_page.value_slider_2

        self.target_val_input_1 = self.mw.control_page.target_value_1
        self.target_val_input_2 = self.mw.control_page.target_value_2
        
        self.write_val_button_1 = self.mw.control_page.write_value_button_1
        self.write_val_button_2= self.mw.control_page.write_value_button_2

        self.target_show_1 = self.mw.control_page.control_target_show_1
        self.target_show_2 = self.mw.control_page.control_target_show_2

        self.MAX_val_input_1.textChanged.connect(self.MAX_value_changed_1)
        self.MAX_val_input_2.textChanged.connect(self.MAX_value_changed_2)

        self.value_slider_1.valueChanged.connect(self.value_slider_changed_1)
        self.value_slider_2.valueChanged.connect(self.value_slider_changed_2)

        self.write_val_button_1.clicked.connect(self.write_value_1)
        self.write_val_button_2.clicked.connect(self.write_value_2)

    # ── 辅助 ──

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
        pass  # 已废弃，由各通道内联处理

    def MAX_value_changed_1(self, text):
        """最大值输入变更时重置滑块"""
        if text == "-":
            return
        try:
            val = abs(float(text))
            if val < 1e-6:
                self.MAX_val_input_1.setText(str(10))
                return
            self.target_val_input_1.setText(str(0))
            self.value_slider_1.setValue(0)
        except (ValueError, TypeError):
            self.MAX_val_input_1.setText(str(10))

    def MAX_value_changed_2(self, text):
        """最大值输入变更时重置滑块"""
        if text == "-":
            return
        try:
            val = abs(float(text))
            if val < 1e-6:
                self.MAX_val_input_2.setText(str(10))
                return
            self.target_val_input_2.setText(str(0))
            self.value_slider_2.setValue(0)
        except (ValueError, TypeError):
            self.MAX_val_input_2.setText(str(10))

    def value_slider_changed_1(self, value):
        """滑块拖动 → 更新显示值并发送参考值"""
        max_val = float(self.MAX_val_input_1.text())
        val = (value / 1000) * max_val
        # 四舍五入到3位有效数字，避免 str -> float 反复转换
        val = round(val, 3) if abs(val) >= 0.001 else val
        self.target_val_input_1.setText(str(val))
        self.send_val_ref()

    def value_slider_changed_2(self, value):
        """滑块拖动 → 更新显示值并发送参考值"""
        max_val = float(self.MAX_val_input_2.text())
        val = (value / 1000) * max_val
        # 四舍五入到3位有效数字，避免 str -> float 反复转换
        val = round(val, 3) if abs(val) >= 0.001 else val
        self.target_val_input_2.setText(str(val))
        self.send_val_ref()

    def write_value_1(self):
        """写入目标值（输入框确认）"""
        max_val = float(self.MAX_val_input_1.text())
        target_val = float(self.target_val_input_1.text())
        if max_val>0:# 正滑条
            if target_val<0:
                target_val = 0
            elif target_val>max_val:
                target_val = max_val
        else:# 负滑条
            if target_val>0:
                target_val = 0
            elif target_val<max_val:
                target_val = max_val
        slider_pos = int(abs(target_val) / abs(max_val) * 1000) if max_val != 0 else 0
        self.value_slider_1.setValue(min(slider_pos, 1000))
        self.target_val_input_1.setText(str(f"{target_val:.3g}"))
        self.send_val_ref()

    def write_value_2(self):
        """写入目标值（输入框确认）"""
        max_val = float(self.MAX_val_input_2.text())
        target_val = float(self.target_val_input_2.text())
        if max_val>0:# 正滑条
            if target_val<0:
                target_val = 0
            elif target_val>max_val:
                target_val = max_val
        else:# 负滑条
            if target_val>0:
                target_val = 0
            elif target_val<max_val:
                target_val = max_val
        slider_pos = int(abs(target_val) / abs(max_val) * 1000) if max_val != 0 else 0
        self.value_slider_2.setValue(min(slider_pos, 1000))
        self.target_val_input_2.setText(str(f"{target_val:.3g}"))
        self.send_val_ref()

    def send_val_ref(self):
        """发送目标参考值到设备"""
        value_1 = float(self.target_val_input_1.text())
        value_2 = float(self.target_val_input_2.text())
        value=[value_1,value_2]
        val_ref = struct.pack("<f", value_1)+struct.pack("<f", value_2)
        self.com.send_packet(Cidx.CMD_REFVALUE_SET, val_ref)
        logger.debug("发送参考值: value_1=%.3f, value_2=%.3f", value_1, value_2)
