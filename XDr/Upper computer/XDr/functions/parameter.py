import struct
import threading
from PyQt5.QtWidgets import QFileDialog
import time

# 命令 ID 定义
class Cmd:

    UC_CONNECT    = 0xF0
    UC_DISCONNECT = 0xFE
    START_TUNNING = 0xF1
    BRAKE         = 0xF2
    FOC_NRST      = 0xF3
    ENABLE        = 0xF4
    DISABLE       = 0xF5
    PROTECT_RESET = 0xF6
    LOG_GET       = 0xF7
    LOG_ERASE     = 0xF8
    PARAM_ERASE   = 0x01
    PARAM_WRITE   = 0x02
    PARAM_READ    = 0x03
    PARAM_SAVE    = 0x04
    CMD_REFVALUE_SET = 0x21
    CMD_MODE_SET     = 0x22
    CMD_STREAM_GET   = 0x23
    CMD_STREAM_SET   = 0x25

class PIdx:
    # bool 类型参数
    FOC_MODE             = 0
    LOOP_MODE            = 1
    SW_CANQUEUE          = 2
    SW_WEAKMAG           = 3
    SW_FAN               = 4
    SW_VAGUE_PID         = 5
    SW_PVT               = 6

    MOTOR_WIRE_SEQUENCE  = 7
    MOTOR_POLEPAIRS      = 8
    FREQ_CURRENT_LOOP    = 9
    FREQ_SPEED_LOOP      = 10
    FREQ_POSITION_LOOP   = 11
    #u32 类型参数
    CAN_ID               = 12
    # float 类型参数
    F_PWM                = 13
    F_CURRENT_LOOP       = 14
    F_SPEED_LOOP         = 15
    F_POSITION_LOOP      = 16
    THETA_OFFSET         = 17
    MOTOR_RS             = 18
    MOTOR_LS             = 19
    MOTOR_PSIF           = 20
    MOTOR_KE             = 21
    MOTOR_J              = 22
    MOTOR_B              = 23
    KP_CURRENT           = 24
    KI_CURRENT           = 25
    KP_WEAKMAG           = 26
    KI_WEAKMAG           = 27
    KP_SPEED             = 28
    KI_SPEED             = 29
    KP_POSITION          = 30
    KI_POSITION          = 31
    KD_POSITION          = 32
    LIMIT_CURRENT        = 33
    LIMIT_SPEED          = 34
    LIMIT_POSITION_MIN   = 35
    LIMIT_POSITION_MAX   = 36
    TOLERANCE_TIME       = 37
    TOLERANCE_VOLTAGE    = 38
    TOLERANCE_CURRENT    = 39
    TOLERANCE_SPEED      = 40
    TOLERANCE_POSITION   = 41
    STARTUP_ACC          = 42
    ALIGN_CURRENT        = 43
    ALIGN_TIME           = 44
    OPEN_LOOP_CURRENT    = 45
    OPEN_LOOP_SPEED      = 46
    CHANGE_LOOP_SPEED    = 47

class ParameterManager:
    def __init__(self, main_window, com_port):
        self.mw = main_window
        self.com = com_port
        self.system_desc = "无"

        self.refvalue_map = ["目标电压/V","目标电流/A", "目标速度/rpm", "目标位置/°"]
        # 建立 索引(Int) -> UI控件 的映射
        self.param_map = {
            #模式
            PIdx.FOC_MODE:self.mw.ui.FOCmode,
            PIdx.LOOP_MODE:self.mw.ui.LOOPmode,
            PIdx.SW_CANQUEUE:self.mw.ui.CANmode,
            PIdx.SW_WEAKMAG:self.mw.ui.weakmag,
            PIdx.SW_FAN:self.mw.ui.FAN,
            PIdx.SW_VAGUE_PID:self.mw.ui.vaguePID,
            PIdx.SW_PVT:self.mw.ui.PVT,
            #控制参数
            PIdx.CAN_ID:     self.mw.ui.CANID,
            PIdx.KP_CURRENT: self.mw.ui.CURKp,
            PIdx.KI_CURRENT: self.mw.ui.CURKi,
            PIdx.KP_WEAKMAG: self.mw.ui.WEAKMAGKp,
            PIdx.KI_WEAKMAG: self.mw.ui.WEAKMAGKi,
            PIdx.KP_SPEED:   self.mw.ui.SPEEDKp,
            PIdx.KI_SPEED:   self.mw.ui.SPEEDKi,
            PIdx.KP_POSITION:self.mw.ui.POSKp,
            PIdx.KI_POSITION:self.mw.ui.POSKi,
            PIdx.KD_POSITION:self.mw.ui.POSKd,
            #电机参数
            PIdx.THETA_OFFSET:self.mw.ui.thetaoffset,
            PIdx.MOTOR_WIRE_SEQUENCE:self.mw.ui.WireSequence,
            PIdx.MOTOR_POLEPAIRS:self.mw.ui.Pole_pires,
            PIdx.MOTOR_RS:   self.mw.ui.Rs,
            PIdx.MOTOR_LS:   self.mw.ui.Ls,
            PIdx.MOTOR_PSIF: self.mw.ui.psif,
            PIdx.MOTOR_KE:   self.mw.ui.Ke,
            PIdx.MOTOR_J:    self.mw.ui.J_val,
            PIdx.MOTOR_B:    self.mw.ui.B_val,
            #静态参数
            PIdx.F_PWM:self.mw.ui.f_pwm,
            PIdx.F_CURRENT_LOOP:self.mw.ui.f_current,
            PIdx.F_SPEED_LOOP:self.mw.ui.f_speed,
            PIdx.F_POSITION_LOOP:self.mw.ui.f_postion,
            PIdx.FREQ_CURRENT_LOOP:self.mw.ui.freq_cur,
            PIdx.FREQ_SPEED_LOOP:self.mw.ui.freq_speed,
            PIdx.FREQ_POSITION_LOOP:self.mw.ui.freq_pos,
            PIdx.LIMIT_CURRENT:self.mw.ui.limit_current,
            PIdx.LIMIT_SPEED:self.mw.ui.limit_speed,
            PIdx.LIMIT_POSITION_MIN:self.mw.ui.min_postion,
            PIdx.LIMIT_POSITION_MAX:self.mw.ui.max_postion,
            PIdx.TOLERANCE_TIME:self.mw.ui.tolerate_time,
            PIdx.TOLERANCE_VOLTAGE:self.mw.ui.tolerate_voltage,
            PIdx.TOLERANCE_CURRENT:self.mw.ui.tolerate_current,
            PIdx.TOLERANCE_SPEED:self.mw.ui.tolerate_speed,
            PIdx.TOLERANCE_POSITION:self.mw.ui.tolerate_postion,
            PIdx.STARTUP_ACC:self.mw.ui.acceleration,
            PIdx.ALIGN_CURRENT:self.mw.ui.align_current,
            PIdx.ALIGN_TIME:self.mw.ui.align_time,
            PIdx.OPEN_LOOP_CURRENT:self.mw.ui.openloop_current,
            PIdx.OPEN_LOOP_SPEED:self.mw.ui.openloop_speed,
            PIdx.CHANGE_LOOP_SPEED:self.mw.ui.changeloop_speed,
        }
        self.param_show_map = {
            PIdx.FOC_MODE:self.mw.ui.FOCmodeshow,
            PIdx.LOOP_MODE:self.mw.ui.loopmodeshow,
            PIdx.SW_CANQUEUE:self.mw.ui.canmodeshow,
            #todo: 双态开关绑定处理
            PIdx.CAN_ID:self.mw.ui.CANIDshow,
        }

    def write_all(self):
        """遍历映射表发送参数包"""
        def task():
            for idx, widget in self.param_map.items():
                print(f"正在处理参数 idx={idx}, widget={widget}")  
                try:
                    if idx < PIdx.CAN_ID:
                        match idx:
                            # 下拉列表
                            case PIdx.FOC_MODE| PIdx.LOOP_MODE|PIdx.SW_CANQUEUE|PIdx.MOTOR_WIRE_SEQUENCE:
                                val = widget.currentIndex()
                                
                            # 双态开关
                            case PIdx.SW_FAN|PIdx.SW_VAGUE_PID|PIdx.SW_PVT|PIdx.SW_WEAKMAG:
                                val=0
                                #todo: 双态开关处理
                                
                            # 数字输入框
                            case PIdx.MOTOR_POLEPAIRS|PIdx.FREQ_CURRENT_LOOP|PIdx.FREQ_SPEED_LOOP|PIdx.FREQ_POSITION_LOOP:
                                val = int(widget.text())
                                
                        print(f"{idx}")
                        raw_val = struct.pack('<B', val)
                    elif idx < PIdx.F_PWM:
                        val = int(widget.text())
                        if val < 0:
                            raise ValueError("uint32值不能为负值")
                        raw_val = struct.pack('<I', val)
                    elif idx < PIdx.THETA_OFFSET:
                        continue
                    else:
                        val_str = widget.text()
                        raw_val = struct.pack('<f', float(val_str))
                    # 组合数据段：参数索引(1b) + 数据(4b)
                    payload = bytes([idx]) + raw_val
                    self.com.send_packet(Cmd.PARAM_WRITE, payload)
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
        self.com.send_packet(Cmd.PARAM_READ, bytes([0xff]))

    def show_param(self,index,data):
        if index < PIdx.CAN_ID:
            val=struct.unpack('<B', data)[0]
            match index:
                # 下拉列表
                case PIdx.FOC_MODE| PIdx.LOOP_MODE|PIdx.SW_CANQUEUE|PIdx.MOTOR_WIRE_SEQUENCE:
                    self.param_map[index].setCurrentIndex(val)    
                    if index in self.param_show_map:
                        self.param_show_map[index].setText(self.param_map[index].currentText())
                    if index == PIdx.LOOP_MODE:
                        self.mw.ui.controlval_show.setText(self.refvalue_map[val])                
                # 双态开关
                case PIdx.SW_FAN|PIdx.SW_VAGUE_PID|PIdx.SW_PVT|PIdx.SW_WEAKMAG:
                    print("双态开关处理")
                    #todo: 双态开关处理
                # 数字输入框
                case PIdx.MOTOR_POLEPAIRS|PIdx.FREQ_CURRENT_LOOP|PIdx.FREQ_SPEED_LOOP|PIdx.FREQ_POSITION_LOOP:
                    self.param_map[index].setText(str(val))
        elif index < PIdx.F_PWM:
            val=struct.unpack('<i', data)[0]
            self.param_map[index].setText(str(val))
            if(index in self.param_show_map):
                self.param_show_map[index].setText(str(val))

        else:
            val=struct.unpack('<f', data)[0]
            self.param_map[index].setText(f"{val:.6g}")

    def save_flash(self):
        self.com.send_packet(Cmd.PARAM_SAVE, bytes())

    def erase_flash(self):
        self.com.send_packet(Cmd.PARAM_ERASE, bytes())