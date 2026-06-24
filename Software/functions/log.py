import struct
import logging
from protocol import Lidx, Midx, Cidx

# ---------- 日志配置 ----------
logger = logging.getLogger("LogManager")
logger.setLevel(logging.INFO)
if not logger.handlers:
    _handler = logging.StreamHandler()
    _handler.setFormatter(logging.Formatter("[%(levelname)s] %(name)s: %(message)s"))
    logger.addHandler(_handler)

class LogManager:
    def __init__(self, main_window):
        self.mw = main_window
        self.com = self.mw.comport

        # 假设 data_page 中 log_num 已改为 QComboBox
        self.log_combo= self.mw.data_page.log_num
        self.read_log_btn = self.mw.data_page.read_log_btn
        self.clear_log_btn = self.mw.data_page.clear_log_btn
        self.show_log_btn = self.mw.data_page.show_log_btn

        # 信号连接
        self.read_log_btn.clicked.connect(self.read_log)
        self.clear_log_btn.longPressed.connect(self.clear_log)
        self.show_log_btn.clicked.connect(self.show_log)

        # 界面映射（用于显示单条日志详情）
        self.log_map = self.mw.ui_map.log_map
        self.logs = []   # 存储所有日志条目，每条为混合列表（前8字节 int，后 float）

    def add_log(self, log_bytes: bytes):
        """解析并添加一条日志到列表"""
        if len(log_bytes) < 8:
            logger.error("日志数据长度不足 8 字节")
            return

        # 解析前 8 个字节为整数
        head_ints = list(log_bytes[:8])

        # 剩余部分解析为 float (小端)
        remaining = log_bytes[8:]
        if len(remaining) % 4 != 0:
            logger.error("日志数据剩余字节长度不是 4 的倍数，无法解析 float")
            return

        floats = []
        for i in range(len(remaining) // 4):
            chunk = remaining[i*4 : (i+1)*4]
            val = struct.unpack("<f", chunk)[0]
            floats.append(val)

        log_entry = head_ints + floats
        self.logs.append(log_entry)
        logger.debug(f"添加日志 #{len(self.logs)}，编号字段值: {log_entry[Lidx.num]}")

        # 向 ComboBox 添加显示项（限制最大条数防止 UI 卡顿）
        MAX_LOG_ITEMS = 200
        log_num = log_entry[Lidx.num]
        item_text = f"第 {log_num + 1} 条日志"
        self.log_combo.addItem(item_text)

        if self.log_combo.count() > MAX_LOG_ITEMS:
            # 移除最旧的条目
            self.log_combo.removeItem(0)
            self.logs.pop(0)

    def show_log(self):
        """显示当前选中日志的详细信息"""
        index = self.log_combo.currentIndex()
        if index < 0 or index >= len(self.logs):
            logger.warning(f"日志索引 {index} 无效")
            return

        log = self.logs[index]
        logger.info(f"显示日志 #{index} (编号 {log[Lidx.num]})")

        # 映射常量对应的 UI 控件更新
        # 整数部分（前8字节）索引 0-7
        self.log_map[Lidx.num].setText(str(log[Lidx.num]))
        self.log_map[Lidx.time].setText(str(log[Lidx.time]))

        # 枚举类型转换
        self.log_map[Lidx.fault].setText(Midx.fault_state[log[Lidx.fault]])
        self.log_map[Lidx.warning].setText(Midx.warning_state[log[Lidx.warning]])
        self.log_map[Lidx.sensor_mode].setText(Midx.sensor_mode[log[Lidx.sensor_mode]])
        self.log_map[Lidx.run_mode].setText(Midx.run_mode[log[Lidx.run_mode]])
        self.log_map[Lidx.can_status].setText(Midx.drive_state[log[Lidx.can_status]])
        self.log_map[Lidx.encode_status].setText(Midx.drive_state[log[Lidx.encode_status]])

        # 浮点数部分（索引从 Lidx.voltage 开始到 Lidx.log_num 结束？）
        # 假设 Lidx.voltage 之后都是 float 字段，且映射表顺序一致
        for i in range(Lidx.voltage, Lidx.log_num):
            if i < len(log):
                val_text = f"{log[i]:.3f}"
            else:
                val_text = "N/A"
            self.log_map[i].setText(val_text)

    def read_log(self):
        """请求下位机发送日志数据"""
        logger.info("请求读取日志")
        self.com.send_packet(Cidx.LOG_GET, bytes())

    def clear_log(self):
        """清空本地日志列表并下发擦除命令"""
        self.log_combo.clear()
        self.logs.clear()
        logger.info("本地日志已清空，发送擦除命令")
        self.com.send_packet(Cidx.LOG_ERASE, bytes())