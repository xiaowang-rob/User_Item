import struct
import threading
import time
import logging
from shared_constants import Pidx, Cidx, Midx, Fidx

# ---------- 日志配置 ----------
logger = logging.getLogger("ParameterManager")
logger.setLevel(logging.DEBUG)
if not logger.handlers:
    _handler = logging.StreamHandler()
    _handler.setFormatter(logging.Formatter("[%(levelname)s] %(name)s: %(message)s"))
    logger.addHandler(_handler)


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

    # ---------- 数据打包/解包 ----------
    def _pack_value(self, index, ui_control):
        """根据参数索引类型返回 (结构体格式字符串, 数值, 字节数据)"""
        # 下拉列表控件（具有 currentIndex 方法） 与 数字输入框的分辨
        if hasattr(ui_control, 'currentIndex') and hasattr(ui_control, 'currentText'):
            # 下拉列表，获取索引值（除了 MOTOR_POLEPAIRS 以外都是下拉）
            val = ui_control.currentIndex()
            return "B", val, struct.pack("<B", val)
        else:
            # 数字输入框，先取文本再转换
            text = ui_control.text()
            if index < Pidx.CAN_ID:                     # u8 整数
                val = int(text)
                return "B", val, struct.pack("<B", val)
            elif index < Pidx.THETA_OFFSET:             # uint32
                val = int(text)
                if val < 0:
                    raise ValueError("uint32 值不能为负")
                return "I", val, struct.pack("<I", val)
            else:                                       # float
                val = float(text)
                return "f", val, struct.pack("<f", val)

    def _unpack_value(self, index, data):
        """将字节数据解包为 Python 值，数据类型与 _pack_value 一致"""
        if index < Pidx.CAN_ID:
            return struct.unpack("<B", data)[0]
        elif index < Pidx.THETA_OFFSET:
            return struct.unpack("<i", data)[0]    # 注意：原代码用 <i (signed int)
        else:
            return struct.unpack("<f", data)[0]

    # ---------- 写入所有参数 ----------
    def write_all(self):
        """遍历映射表发送参数，最后发送执行反馈"""
        def task():
            total = len(self.param_map)
            for seq, (idx, widget) in enumerate(self.param_map.items(), 1):
                try:
                    fmt, val, payload = self._pack_value(idx, widget)
                    data = bytes([idx]) + payload
                    self.com.send_packet(Cidx.PARAM_WRITE, data)
                    logger.debug(f"写入参数 #{seq}/{total} idx={idx} val={val}")
                    time.sleep(0.002)
                except Exception as e:
                    logger.error(f"参数{idx}写入异常: {e}")
                    continue

            # 反馈执行命令
            back_payload = bytes([Pidx.NUM_OF_PARAM]) + struct.pack("<B", Fidx.FEEDBACK_EXECUTE)
            self.com.send_packet(Cidx.PARAM_WRITE, back_payload)
            logger.info(f"全部参数写入完毕，反馈包已发送")
            self.read_all()

        threading.Thread(target=task, daemon=True).start()

    # ---------- 读取全部参数 ----------
    def read_all(self):
        """发送读取全部参数指令"""
        logger.info("请求读取所有参数")
        self.com.send_packet(Cidx.PARAM_READ, bytes([0xFF]))

    # ---------- 接收参数数据 ----------
    def add_param(self, index, data):
        """解析接收到的参数并更新界面"""
        print(f"接收到参数 idx={index} data={data}")
        val = self._unpack_value(index, data)
        self.param_list[index] = val
        self.show_param(index, val)
        logger.debug(f"更新参数 idx={index} val={val}")

    def load_param(self, index, data):
        """从本地加载参数（不解析，直接数值）"""
        self.param_list[index] = data
        self.show_param(index, data)

    # ---------- 参数 UI 显示 ----------
    def show_param(self, index, value):
        """根据参数类型刷新对应控件"""
        # 下拉列表或特殊整数
        if index < Pidx.CAN_ID:
            if index == Pidx.MOTOR_POLEPAIRS:
                self.param_map[index].setText(str(value))
            else:
                self.param_map[index].setCurrentIndex(value)
                # 同步显示当前文本
                if index in self.param_show_map:
                    self.param_show_map[index].setText(
                        self.param_map[index].currentText()
                    )
                # 运行模式特殊处理
                if index == Pidx.RUN_MODE:
                    self.target_val_show.setText(Midx.target_type[value])
        elif index < Pidx.THETA_OFFSET:
            # uint32 / int
            self.param_map[index].setText(str(value))
            if index in self.param_show_map:
                self.param_show_map[index].setText(str(value))
        else:
            # float
            self.param_map[index].setText(f"{value:.6g}")

    def param_all_show(self):
        """刷新全部参数显示"""
        for idx in range(Pidx.NUM_OF_PARAM):
            if idx < len(self.param_list):
                self.show_param(idx, self.param_list[idx])

    # ---------- Flash 操作 ----------
    def save_flash(self):
        logger.info("发送保存参数到 Flash 命令")
        self.com.send_packet(Cidx.PARAM_SAVE, bytes())

    def erase_flash(self):
        logger.info("发送擦除 Flash 参数命令")
        self.com.send_packet(Cidx.PARAM_ERASE, bytes())