from .parameter import Cmd
from PyQt5.QtWidgets import QMessageBox

import struct
class DataProcess:
    def __init__(self, main_window):
        self.main_window = main_window
    def handle_received_data(self, cmd_id: int, data: bytes):
        """处理已解析的有效数据包"""
        try:
            match cmd_id:
                case Cmd.UC_CONNECT:  # UC连接成功 返回系统参数
                    byte_len=int(len(data)/4)+3
                    if byte_len == 6:#已经连接 则接收状态并反馈
                        for i in range(byte_len):
                            if i<4:
                                self.main_window.data.set_status(i,data[i])
                            else:
                                self.main_window.data.set_status(i,struct.unpack('<f',data[(i-3)*4:(i-2)*4])[0])
                        self.main_window.data.show_status()
                        self.main_window.com_port.send_packet(Cmd.UC_CONNECT,bytes())
                    else:
                        self.main_window.param_manager.system_desc = data.decode('utf-8')
                    return
                case Cmd.LOG_GET:  # 日志读取返回
                    self.main_window.log.add_log(data)
                case Cmd.LOG_ERASE:
                    if(data[0] == 0xfe):
                        QMessageBox.information(self.main_window, "提示", "日志已清除")
                    else:
                        QMessageBox.warning(self.main_window, "警告", "日志清除失败")
                    return
                case Cmd.PARAM_ERASE:  # 参数读取返回
                    if(data[0] == 0xfe):
                        QMessageBox.information(self.main_window, "提示", "参数已清除")
                    else:
                        QMessageBox.warning(self.main_window, "警告", "参数清除失败")
                    return
                case Cmd.PARAM_SAVE:  # 参数保存返回
                    if(data[0] == 0xfe):
                        QMessageBox.information(self.main_window, "提示", "参数已保存")
                    else:
                        QMessageBox.warning(self.main_window, "警告", "参数保存失败")
                    return
                case Cmd.PARAM_READ:
                    idx=data[0]
                    self.main_window.param_manager.show_param(idx, data[1:])
                case Cmd.CMD_STREAM_SET:  # 监控值返回
                    byte_len=int(len(data)/4)
                    for i in range(byte_len):
                        self.main_window.data.set_data(i,struct.unpack('<f',data[i*4:(i+1)*4])[0])
                    self.main_window.data.show_data()
                    return


        except Exception as e:
            print(f"数据处理异常: {e}")
       