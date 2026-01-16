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
    MOTOR_POLEPAIRS      = 7
    FREQ_CURRENT_LOOP    = 8
    FREQ_SPEED_LOOP      = 9
    FREQ_POSITION_LOOP   = 10
    #u32 类型参数
    CAN_ID               = 11
    # float 类型参数
    F_PWM                = 12
    F_CURRENT_LOOP       = 13
    F_SPEED_LOOP         = 14
    F_POSITION_LOOP      = 15
    THETA_OFFSET         = 16
    MOTOR_RS             = 17
    MOTOR_LS             = 18
    MOTOR_PSIF           = 19
    MOTOR_KE             = 20
    MOTOR_J              = 21
    MOTOR_B              = 22
    KP_CURRENT           = 23
    KI_CURRENT           = 24
    KP_WEAKMAG           = 25
    KI_WEAKMAG           = 26
    KP_SPEED             = 27
    KI_SPEED             = 28
    KP_POSITION          = 29
    KI_POSITION          = 30
    KD_POSITION          = 31
    LIMIT_CURRENT        = 32
    LIMIT_SPEED          = 33
    LIMIT_POSITION_MIN   = 34
    LIMIT_POSITION_MAX   = 35
    TOLERANCE_TIME       = 36
    TOLERANCE_VOLTAGE    = 37
    TOLERANCE_CURRENT    = 38
    TOLERANCE_SPEED      = 39
    TOLERANCE_POSITION   = 40
    STARTUP_POS_GRAD     = 41
    STARTUP_SPE_GRAD     = 42
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
            PIdx.STARTUP_POS_GRAD:self.mw.ui.postion_speed,
            PIdx.STARTUP_SPE_GRAD:self.mw.ui.acceleration,
            PIdx.ALIGN_CURRENT:self.mw.ui.align_current,
            PIdx.ALIGN_TIME:self.mw.ui.align_time,
            PIdx.OPEN_LOOP_CURRENT:self.mw.ui.openloop_current,
            PIdx.OPEN_LOOP_SPEED:self.mw.ui.openloop_speed,
            PIdx.CHANGE_LOOP_SPEED:self.mw.ui.changeloop_speed,

        }
        print("param_map keys:", sorted(self.param_map.keys()))

    def write_all(self):
        """遍历映射表发送参数包"""
        def task():
            for idx, widget in self.param_map.items():
                print(f"正在处理参数 idx={idx}, widget={widget}")  
                try:
                    if idx < PIdx.CAN_ID:
                        print(f"{idx}")
                        if idx<PIdx.SW_WEAKMAG:
                            val = widget.currentIndex()
                        elif idx>=PIdx.MOTOR_POLEPAIRS:
                            val = int(widget.text())
                        else:
                            val=0;
                        if not (0<=val<=255):
                            raise ValueError("uint8值超出范围")
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
        
        threading.Thread(target=task, daemon=True).start()

    def read_all(self):
        """发送读取指令"""
        self.com.send_packet(Cmd.PARAM_READ, bytes([0xff]))

    def show_param(self,index,data):
        if index < PIdx.CAN_ID:
            val=struct.unpack('<B', data)[0]
            if index < PIdx.SW_WEAKMAG:
                self.param_map[index].setCurrentIndex(val)
            elif index>=PIdx.MOTOR_POLEPAIRS:
                self.param_map[index].setText(str(val))
        elif index < PIdx.F_PWM:
            val=struct.unpack('<i', data)[0]
            self.param_map[index].setText(str(val))
        else:
            val=struct.unpack('<f', data)[0]
            self.param_map[index].setText(f"{val:.6g}")

    def save_flash(self):
        self.com.send_packet(Cmd.PARAM_SAVE, bytes())

    def erase_flash(self):
        self.com.send_packet(Cmd.PARAM_ERASE, bytes())