import struct
import threading
import time
from UI.data_ui_map import Pidx,Cidx,Midx

class ParameterManager:
    def __init__(self, main_window, com_port):
        self.mw = main_window
        self.com = com_port

        self.all_read_but=self.mw.parameter_page.all_read_button
        self.all_write_but=self.mw.parameter_page.all_write_button
        self.save_flash_but=self.mw.parameter_page.all_save_button
        self.erase_flash_but=self.mw.parameter_page.all_erase_button

        self.all_read_but.clicked.connect(self.read_all)
        self.all_write_but.clicked.connect(self.write_all)
        self.save_flash_but.clicked.connect(self.save_flash)
        self.erase_flash_but.longPressed.connect(self.erase_flash)

        self.param_map=self.mw.ui_map.param_map
        self.param_show_map=self.mw.ui_map.param_show_map
        self.target_val_show=self.mw.control_page.control_target_show
        # 建立 索引(Int) -> UI控件 的映射

    def write_all(self):
        """遍历映射表发送参数包"""
        def task():
            for idx, widget in self.param_map.items():
                print(f"正在处理参数 idx={idx}, widget={widget}")  
                try:
                    if idx < Pidx.CAN_ID:
                        match idx:
                            # 下拉列表
                            case Pidx.SENSOR_MODE| Pidx.LOOP_MODE|Pidx.CAN_MODE|Pidx.MOTOR_WIRE_SEQUENCE|Pidx.FAN_MODE|Pidx.VAGUE_PID_MODE|Pidx.PVT_MODE|Pidx.WEAKMAG_MODE:
                                val = widget.currentIndex()
                                                                
                            # 数字输入框
                            case Pidx.MOTOR_POLEPAIRS|Pidx.FREQ_CURRENT_LOOP|Pidx.FREQ_SPEED_LOOP|Pidx.FREQ_POSITION_LOOP:
                                val = int(widget.text())
                                
                        print(f"{idx}")
                        raw_val = struct.pack('<B', val)
                    elif idx < Pidx.F_PWM:
                        val = int(widget.text())
                        if val < 0:
                            raise ValueError("uint32值不能为负值")
                        raw_val = struct.pack('<I', val)
                    elif idx < Pidx.THETA_OFFSET:
                        continue
                    else:
                        val_str = widget.text()
                        raw_val = struct.pack('<f', float(val_str))
                    # 组合数据段：参数索引(1b) + 数据(4b)
                    payload = bytes([idx]) + raw_val
                    self.com.send_packet(Cidx.PARAM_WRITE, payload)
                    time.sleep(0.002)
                    print(f"参数{idx}写入成功,{payload}")
                except Exception as e: 
                    print(f"参数{idx}写入失败: {e}")
                    continue
                
            
            print("参数批量发送完成")
            self.read_all()
        
        threading.Thread(target=task, daemon=True).start()

    def read_all(self):
        """发送读取指令"""
        self.com.send_packet(Cidx.PARAM_READ, bytes([0xff]))

    def show_param(self,index,data):
        if index < Pidx.CAN_ID:
            val=struct.unpack('<B', data)[0]
            match index:
                # 下拉列表
                case Pidx.SENSOR_MODE| Pidx.LOOP_MODE|Pidx.CAN_MODE|Pidx.MOTOR_WIRE_SEQUENCE|Pidx.FAN_MODE|Pidx.VAGUE_PID_MODE|Pidx.PVT_MODE|Pidx.WEAKMAG_MODE:
                    self.param_map[index].setCurrentIndex(val)    
                    if index in self.param_show_map:
                        self.param_show_map[index].setText(self.param_map[index].currentText())
                    if index == Pidx.LOOP_MODE:
                        self.target_val_show.setText(Midx.target_value[val])                

                # 数字输入框
                case Pidx.MOTOR_POLEPAIRS|Pidx.FREQ_CURRENT_LOOP|Pidx.FREQ_SPEED_LOOP|Pidx.FREQ_POSITION_LOOP:
                    self.param_map[index].setText(str(val))
        elif index < Pidx.F_PWM:
            val=struct.unpack('<i', data)[0]
            self.param_map[index].setText(str(val))
            if(index in self.param_show_map):
                self.param_show_map[index].setText(str(val))

        else:
            val=struct.unpack('<f', data)[0]
            self.param_map[index].setText(f"{val:.6g}")

    def save_flash(self):
        self.com.send_packet(Cidx.PARAM_SAVE, bytes())

    def erase_flash(self):
        self.com.send_packet(Cidx.PARAM_ERASE, bytes())


