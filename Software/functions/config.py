"""配置管理：参数配置的保存/加载/覆盖/删除"""

import os
import json
import logging
from PyQt5.QtWidgets import QVBoxLayout, QHBoxLayout, QWidget
from PyQt5.QtCore import Qt
from siui.components.editbox import SiLabeledLineEdit
from siui.components.button import SiPushButtonRefactor
from functions.message_show import (
    send_custom_message,
    send_simple_message,
    MSG_TYPE_SUCCESS,
    MSG_TYPE_INFO,
    MSG_TYPE_WARNING,
    MSG_TYPE_ERROR,
)

# ---------- 日志配置 ----------
logger = logging.getLogger("Config")
logger.setLevel(logging.DEBUG)
if not logger.handlers:
    _handler = logging.StreamHandler()
    _handler.setFormatter(logging.Formatter("[%(levelname)s] %(name)s: %(message)s"))
    logger.addHandler(_handler)


class Pconfig:
    """参数配置管理（保存/加载/覆盖/删除）"""

    def __init__(self, main_window):
        self.mw = main_window
        self.config_dir = "configs"
        os.makedirs(self.config_dir, exist_ok=True)

        self._current_msg_widget = None

        self.combo_config = self.mw.top_area.config_file

        self.load_config_but = self.mw.top_area.load_config
        self.load_config_but.clicked.connect(self.load_selected_config)
        self.save_config_but = self.mw.top_area.save_config
        self.save_config_but.clicked.connect(self.opend_config_input)
        self.remove_config_but = self.mw.top_area.remove_config
        self.remove_config_but.longPressed.connect(self.delete_selected_config)

        self.refresh_config_list()

        # 启动时自动加载 default.json
        default_path = os.path.join(self.config_dir, "default.json")
        if os.path.exists(default_path):
            self._load_config_by_name("default")
            idx = self.combo_config.findText("default")
            if idx >= 0:
                self.combo_config.setCurrentIndex(idx)

    # ───────── helpers ─────────

    def _write_config(self, filepath):
        """将当前 param_list 写入 JSON 文件（核心写入）"""
        with open(filepath, 'w', encoding='utf-8') as f:
            json.dump({"params": self.mw.param_manager.param_list}, f, indent=4)

    def _refresh_combo_selection(self, display_name):
        """刷新下拉列表并选中指定名称"""
        self.refresh_config_list()
        idx = self.combo_config.findText(display_name)
        if idx >= 0:
            self.combo_config.setCurrentIndex(idx)

    def _close_msg_widget(self):
        """关闭自定义消息输入框"""
        if self._current_msg_widget and hasattr(self._current_msg_widget, '_close'):
            self._current_msg_widget._close()
            self._current_msg_widget = None

    # ───────── 保存 / 覆盖 ─────────

    def opend_config_input(self):
        """弹出配置名输入框"""
        input_widget = QWidget()
        input_widget.setFixedSize(300, 160)

        layout = QVBoxLayout(input_widget)
        layout.setContentsMargins(20, 15, 20, 15)
        layout.setSpacing(15)
        self._current_msg_widget = input_widget

        name_input = SiLabeledLineEdit()
        name_input.setTitle("配置名")
        name_input.setPlaceholderText("请输入配置名")
        name_input.setAlignment(Qt.AlignCenter)
        name_input.adjustSize()

        button_widget = QWidget()
        button_layout = QHBoxLayout(button_widget)
        button_layout.setContentsMargins(0, 0, 0, 0)
        button_layout.setSpacing(15)
        button_layout.setAlignment(Qt.AlignCenter)

        save_but = SiPushButtonRefactor()
        save_but.setText("保存")
        save_but.setFixedWidth(100)
        save_but.clicked.connect(lambda: self.save_current_config(name_input.text()))

        cover_but = SiPushButtonRefactor()
        cover_but.setText("覆盖")
        cover_but.setFixedWidth(100)
        cover_but.clicked.connect(lambda: self.cover_current_config(name_input.text()))

        button_layout.addWidget(save_but)
        button_layout.addWidget(cover_but)
        layout.addWidget(name_input)
        layout.addWidget(button_widget)

        send_custom_message(input_widget, MSG_TYPE_INFO)

    def save_current_config(self, config_name=None):
        """保存当前参数到新配置"""
        config_name = config_name.strip() if config_name else ""
        if not config_name:
            send_simple_message(MSG_TYPE_WARNING, "配置名不能为空", True, 800)
            return
        if not config_name.endswith(".json"):
            config_name += ".json"

        filepath = os.path.join(self.config_dir, config_name)
        try:
            self._write_config(filepath)
            display_name = config_name[:-5]
            self._refresh_combo_selection(display_name)
            self._close_msg_widget()
            send_simple_message(MSG_TYPE_SUCCESS, f"配置“{display_name}”保存成功", True, 800)
            logger.info("配置已保存: %s", display_name)
        except Exception as e:
            send_simple_message(MSG_TYPE_ERROR, f"保存失败：{str(e)}", True, 1000)
            logger.error("保存配置失败: %s", e)

    def cover_current_config(self, new_config_name=None):
        """覆盖已有配置（不改名，直接写入）"""
        display_name = self.combo_config.currentText()
        if not display_name:
            send_simple_message(MSG_TYPE_WARNING, "请先选择一个配置", True, 800)
            return

        target_name = (new_config_name.strip()
                       if new_config_name and new_config_name.strip()
                       else display_name)
        if not target_name.endswith(".json"):
            target_name += ".json"

        filepath = os.path.join(self.config_dir, target_name)
        try:
            self._write_config(filepath)
            clean = target_name[:-5]
            self._refresh_combo_selection(clean)
            self._close_msg_widget()
            send_simple_message(MSG_TYPE_SUCCESS, f"配置“{clean}”覆盖成功", True, 800)
            logger.info("配置已覆盖: %s", clean)
        except Exception as e:
            send_simple_message(MSG_TYPE_ERROR, f"覆盖失败：{str(e)}", True, 1000)
            logger.error("覆盖配置失败: %s", e)

    def cover_selected_config(self):
        """覆盖当前 ComboBox 选中的配置（不改名）"""
        display_name = self.combo_config.currentText()
        if not display_name:
            send_simple_message(MSG_TYPE_WARNING, "请先选择一个配置", True, 800)
            return
        filepath = os.path.join(self.config_dir, display_name + ".json")
        try:
            self._write_config(filepath)
            self._close_msg_widget()
            send_simple_message(MSG_TYPE_SUCCESS, f"配置“{display_name}”已覆盖", True, 800)
            logger.info("配置已覆盖(选中): %s", display_name)
        except Exception as e:
            send_simple_message(MSG_TYPE_ERROR, f"覆盖失败：{str(e)}", True, 1000)
            logger.error("覆盖选中配置失败: %s", e)

    # ───────── 加载 ─────────

    def load_selected_config(self):
        """加载 ComboBox 选中的配置"""
        display_name = self.combo_config.currentText()
        if display_name:
            self._load_config_by_name(display_name)

    def _load_config_by_name(self, display_name):
        """根据显示名称加载配置到 param_list"""
        filepath = os.path.join(self.config_dir, display_name + ".json")
        if not os.path.exists(filepath):
            send_simple_message(MSG_TYPE_WARNING, "配置文件不存在", True, 1000)
            return

        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                raw_data = json.load(f)

            # 兼容旧版列表格式 与 标准 dict 格式
            if isinstance(raw_data, list):
                params = raw_data
            elif isinstance(raw_data, dict) and "params" in raw_data:
                params = raw_data["params"]
            else:
                params = []

            if not isinstance(params, list):
                params = []

            param_manager = self.mw.param_manager
            total = len(param_manager.param_list)
            loaded = 0

            for i in range(min(len(params), total)):
                try:
                    value = params[i]
                    if isinstance(value, str):
                        try:
                            value = float(value) if ('.' in value or 'e' in value.lower()) else int(value)
                        except ValueError:
                            continue
                    elif not isinstance(value, (int, float, bool)):
                        continue
                    param_manager.load_param(i, value)
                    loaded += 1
                except (IndexError, TypeError):
                    continue

            send_simple_message(MSG_TYPE_SUCCESS, f"配置“{display_name}”加载成功", True, 800)
            logger.info("配置已加载: %s (%d 项)", display_name, loaded)
        except Exception as e:
            err = f"{type(e).__name__} - {str(e)[:100]}"
            send_simple_message(MSG_TYPE_ERROR, f"加载失败：{err}", True, 2000)
            logger.error("加载配置失败: %s", err)

    # ───────── 删除 ─────────

    def delete_selected_config(self):
        """长按删除当前选中的配置（default.json 受保护）"""
        display_name = self.combo_config.currentText()
        if not display_name:
            send_simple_message(MSG_TYPE_WARNING, "请先选择一个配置", True, 800)
            return
        if (display_name + ".json") == "default.json":
            send_simple_message(MSG_TYPE_ERROR, "不能删除默认配置", True, 800)
            return

        filepath = os.path.join(self.config_dir, display_name + ".json")
        try:
            os.remove(filepath)
            self.refresh_config_list()
            if self.combo_config.count() > 0:
                self.combo_config.setCurrentIndex(0)
            send_simple_message(MSG_TYPE_SUCCESS, f"配置“{display_name}”已删除", True, 800)
            logger.info("配置已删除: %s", display_name)
        except Exception as e:
            send_simple_message(MSG_TYPE_ERROR, f"删除失败：{str(e)}", True, 1000)
            logger.error("删除配置失败: %s", e)

    # ───────── 刷新 ─────────

    def refresh_config_list(self):
        """刷新下拉列表（显示不含 .json 的名称）"""
        self.combo_config.blockSignals(True)
        self.combo_config.clear()
        names = sorted(f[:-5] for f in os.listdir(self.config_dir) if f.endswith('.json'))
        self.combo_config.addItems(names)
        self.combo_config.blockSignals(False)
