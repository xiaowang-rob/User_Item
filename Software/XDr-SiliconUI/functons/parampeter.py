import struct
import threading
import time
from shared_constants import (
    Pidx,
    Cidx,
    Midx,
    Fidx,
)


class ParameterManager:
    def __init__(self, main_window, com_port):
        self.mw = main_window
        self.com = com_port

        self.param_list = [0] * Pidx.NUM_OF_PARAM

        self.all_read_but = self.mw.data_page.all_read_button
        self.all_write_but = self.mw.data_page.all_write_button
        self.save_flash_but = self.mw.data_page.all_save_button
        self.erase_flash_but = self.mw.data_page.all_erase_button

        self.all_read_but.clicked.connect(self.read_all)
        self.all_write_but.clicked.connect(self.write_all)
        self.save_flash_but.clicked.connect(self.save_flash)
        self.erase_flash_but.longPressed.connect(self.erase_flash)

        self.param_map = self.mw.ui_map.param_map
        self.param_show_map = self.mw.ui_map.param_show_map
        self.target_val_show = self.mw.control_page.control_target_show
        # 建立 索引(Int) -> UI控件 的映射

    def write_all(self):
        """遍历映射表发送参数包"""

        def task():
            for idx, lineedit in self.param_map.items():
                try:
                    if idx < Pidx.CAN_ID:
                        match idx:
                            # 下拉列表
                            case (
                                Pidx.ENCODER_CHIP
                                | Pidx.SENSOR_MODE
                                | Pidx.RUN_MODE
                                | Pidx.CAN_MODE
                                | Pidx.TRAJ_TYPE
                                | Pidx.VAGUE_PID_MODE
                                | Pidx.PVT_MODE
                            ):
                                val = lineedit.currentIndex()

                            # 数字输入框
                            case Pidx.MOTOR_POLEPAIRS:
                                val = int(lineedit.text())

                        raw_val = struct.pack("<B", val)
                    elif idx < Pidx.THETA_OFFSET:
                        val = int(lineedit.text())
                        if val < 0:
                            raise ValueError("uint32值不能为负值")
                        raw_val = struct.pack("<I", val)
                    elif idx < Pidx.THETA_OFFSET:
                        continue
                    else:
                        val = float(lineedit.text())
                        raw_val = struct.pack("<f", val)
                    # 组合数据段：参数索引(1b) + 数据(4b)
                    payload = bytes([idx]) + raw_val
                    self.com.send_packet(Cidx.PARAM_WRITE, payload)
                    time.sleep(0.002)
                except Exception as e:
                    print(f"参数{idx}写入失败: {e}")
                    continue
            payload=bytes([Pidx.NUM_OF_PARAM]) + struct.pack("<B", Fidx.FEEDBACK_EXECUTE)
            self.com.send_packet(Cidx.PARAM_WRITE, payload)
            print(f"发送完成{payload}")
            self.read_all()

        threading.Thread(target=task, daemon=True).start()

    def read_all(self):
        """发送读取指令"""
        self.com.send_packet(Cidx.PARAM_READ, bytes([0xFF]))

    def add_param(self, index, data):
        if index < Pidx.CAN_ID:
            val = struct.unpack("<B", data)[0]
        elif index < Pidx.THETA_OFFSET:
            val = struct.unpack("<i", data)[0]
        else:
            val = struct.unpack("<f", data)[0]
        self.param_list[index] = val
        self.show_param(index, val)

    def load_param(self, index, data):
        self.param_list[index] = data
        self.show_param(index, data)

    def show_param(self, index, data):
        if index < Pidx.CAN_ID:
            match index:
                # 下拉列表
                case (
                    Pidx.ENCODER_CHIP
                    | Pidx.SENSOR_MODE
                    | Pidx.RUN_MODE
                    | Pidx.CAN_MODE
                    | Pidx.TRAJ_TYPE
                    | Pidx.VAGUE_PID_MODE
                    | Pidx.PVT_MODE
                ):
                    self.param_map[index].setCurrentIndex(data)
                    if index in self.param_show_map:
                        self.param_show_map[index].setText(
                            self.param_map[index].currentText()
                        )
                    if index == Pidx.RUN_MODE:
                        self.target_val_show.setText(Midx.target_type[data])

                # 数字输入框
                case Pidx.MOTOR_POLEPAIRS:
                    self.param_map[index].setText(str(data))
        elif index < Pidx.THETA_OFFSET:
            self.param_map[index].setText(str(data))
            if index in self.param_show_map:
                self.param_show_map[index].setText(str(data))
        else:
            self.param_map[index].setText(f"{data:.6g}")

    def param_all_show(self):
        for idx in Pidx.NUM_OF_PARAM:
            self.show_param(idx, self.param_list[idx])

    def save_flash(self):
        self.com.send_packet(Cidx.PARAM_SAVE, bytes())

    def erase_flash(self):
        self.com.send_packet(Cidx.PARAM_ERASE, bytes())
