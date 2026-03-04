from UI.data_ui_map import Cidx
from PyQt5.QtWidgets import QMessageBox
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
class DataProcess:
    def __init__(self, main_window):
        self.mw = main_window
    def handle_received_data(self, cmd_id: int, data: bytes):
        """处理已解析的有效数据包"""
        try:
            match cmd_id:
                case Cidx.UC_CONNECT:  # UC连接成功 返回系统参数
                    byte_len=int(len(data)/4)+3
                    if byte_len == 6:#已经连接 则接收状态并反馈
                        self.mw.comport.update_status_time()
                        
                        for i in range(byte_len):
                            if i < 4:
                                self.mw.data_show.set_status(i, data[i])
                            else:
                                self.mw.data_show.set_status(i, struct.unpack('<f', data[(i-3)*4:(i-2)*4])[0])
                        self.mw.data_show.show_status()
                        self.mw.comport.send_packet(Cidx.UC_CONNECT, bytes()) 
                    else:
                        # 解析 system_message 字符串，按逗号分隔
                        sys_msg=data.decode()
                        parts = sys_msg.split(',')
                        version = parts[0].strip()+" "+parts[1].strip()
                        self.mw.IAP.set_current_version(version)
                        # 定义字段标签（项目名称）
                        labels = ["设备名称", "版本", "作者", "最大电流", "电压范围", "最大温度"]
                        # 构建带标签的多行字符串
                        formatted_lines = []
                        for i, label in enumerate(labels):
                            value = parts[i].strip() if i < len(parts) else ""
                            formatted_lines.append(f"{label}: {value}")

                        # 用换行符连接所有行
                        self.mw.system_message = "\n".join(formatted_lines)
                    return
                case Cidx.LOG_GET:  # 日志读取返回
                    self.mw.log.add_log(data)
                case Cidx.LOG_ERASE:
                    if(data[0] == 0xfe):
                        send_titled_message(MSG_TYPE_SUCCESS, "提示", "日志已清除",True,1000)
                    else:
                        send_titled_message(MSG_TYPE_ERROR, "错误", "日志清除失败")
                    return
                case Cidx.PARAM_ERASE:  # 参数读取返回
                    if(data[0] == 0xfe):
                        send_titled_message(MSG_TYPE_SUCCESS, "提示", "参数已清除",True,1000)
                    else:
                        send_titled_message(MSG_TYPE_ERROR, "错误", "参数清除失败")
                    return
                case Cidx.PARAM_SAVE:  # 参数保存返回
                    if(data[0] == 0xfe):
                        send_titled_message(MSG_TYPE_SUCCESS, "提示", "参数已保存",True,1000)
                    else:
                        send_titled_message(MSG_TYPE_ERROR, "错误", "参数保存失败")
                    return
                case Cidx.PARAM_READ:
                    idx=data[0]
                    self.mw.param_manager.add_param(idx, data[1:])
                case Cidx.CMD_STREAM_SET:  # 监控值返回
                    byte_len=int(len(data)/4)
                    for i in range(byte_len):
                        self.mw.wave.add_data_by_index(i,struct.unpack('<f',data[i*4:(i+1)*4])[0])
                    return
                
                case Cidx.CMD_IAP_ENTER,Cidx.CMD_IAP_ERASE,Cidx.CMD_IAP_WRITE,Cidx.CMD_IAP_VERIFY,Cidx.CMD_IAP_EXIT:
                    self.mw.IAP.iap_cmd_received(cmd_id, data)
                    return


        except Exception as e:
            print(f"数据处理异常: {e}")
       