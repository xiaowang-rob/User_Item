import os
import json
from PyQt5.QtWidgets import QInputDialog, QMessageBox
from .parameter import PIdx


class Pconfig:
    def __init__(self, main_window):
        self.mw = main_window
        self.config_dir = "configs"
        os.makedirs(self.config_dir, exist_ok=True)

        self.combo_config = self.mw.ui.configfile
        self.param_map = self.mw.param_manager.param_map
        self.param_show_map = self.mw.param_manager.param_show_map
        # 假设 refvalue_map 在 main_window 中
        self.refvalue_map = getattr(self.mw, 'refvalue_map', {})

        # 刷新配置列表（显示不含 .json 的名称）
        self.refresh_config_list()

        # 启动时自动加载 default.json（如果存在）
        default_path = os.path.join(self.config_dir, "default.json")
        if os.path.exists(default_path):
            self._load_config_by_name("default")
            # 确保 ComboBox 选中 "default"
            idx = self.combo_config.findText("default")
            if idx >= 0:
                self.combo_config.setCurrentIndex(idx)

    # ======================
    # 保存当前参数到配置文件
    # ======================
    def save_current_config(self):
        config_name, ok = QInputDialog.getText(
            self.mw, "保存配置", "请输入配置名称:", text="new_config"
        )
        if not ok or not config_name.strip():
            return

        config_name = config_name.strip()
        if not config_name.endswith(".json"):
            config_name += ".json"

        filepath = os.path.join(self.config_dir, config_name)

        # 收集当前所有参数值
        config_data = {}
        for idx, widget in self.param_map.items():
            try:
                if idx < PIdx.CAN_ID:
                    match idx:
                        case (PIdx.FOC_MODE | PIdx.LOOP_MODE | PIdx.SW_CANQUEUE |
                              PIdx.MOTOR_WIRE_SEQUENCE | PIdx.SW_FAN |
                              PIdx.SW_VAGUE_PID | PIdx.SW_PVT | PIdx.SW_WEAKMAG):
                            val = widget.currentIndex()
                        case (PIdx.MOTOR_POLEPAIRS | PIdx.FREQ_CURRENT_LOOP |
                              PIdx.FREQ_SPEED_LOOP | PIdx.FREQ_POSITION_LOOP):
                            val = int(widget.text())
                    config_data[idx] = {"type": "uint8", "value": val}

                elif idx < PIdx.F_PWM:
                    val = int(widget.text())
                    config_data[idx] = {"type": "int32", "value": val}
                else:
                    val = float(widget.text())
                    config_data[idx] = {"type": "float32", "value": val}
            except Exception as e:
                print(f"读取参数 {idx} 失败: {e}")
                continue

        try:
            with open(filepath, 'w', encoding='utf-8') as f:
                json.dump(config_data, f, indent=4, ensure_ascii=False)
            
            self.refresh_config_list()
            display_name = config_name[:-5]
            index = self.combo_config.findText(display_name)
            if index >= 0:
                self.combo_config.setCurrentIndex(index)
        except Exception as e:
            QMessageBox.critical(self.mw, "错误", f"保存失败：{e}")

    # ======================
    # 加载选中的配置（通过 ComboBox）
    # ======================
    def load_selected_config(self):
        display_name = self.combo_config.currentText()
        if not display_name:
            return
        self._load_config_by_name(display_name)

    # ======================
    # 删除当前选中的配置
    # ======================
    def delete_selected_config(self):
        display_name = self.combo_config.currentText()
        if not display_name:
            return
        config_name = display_name + ".json"
        if config_name == "default.json":
            QMessageBox.warning(self.mw, "提示", "不能删除默认配置")
            return

        reply = QMessageBox.question(
            self.mw, "确认删除",
            f"确定要删除配置 “{display_name}” 吗？",
            QMessageBox.Yes | QMessageBox.No
        )
        if reply != QMessageBox.Yes:
            return

        filepath = os.path.join(self.config_dir, config_name)
        try:
            os.remove(filepath)
            self.refresh_config_list()
            # 自动切换到第一个（默认）
            if self.combo_config.count() > 0:
                self.combo_config.setCurrentIndex(0)
        except Exception as e:
            QMessageBox.critical(self.mw, "错误", f"删除失败：{e}")

    # ======================
    # 刷新 ComboBox 列表（显示不含 .json）
    # ======================
    def refresh_config_list(self):
        self.combo_config.blockSignals(True)
        self.combo_config.clear()
        files = [f for f in os.listdir(self.config_dir) if f.endswith('.json')]
        files.sort()
        display_names = [f[:-5] for f in files]  # 移除 .json 后缀
        self.combo_config.addItems(display_names)
        self.combo_config.blockSignals(False)

    # ======================
    # 内部方法：根据显示名称加载配置
    # ======================
    def _load_config_by_name(self, display_name: str):
        config_name = display_name + ".json"
        filepath = os.path.join(self.config_dir, config_name)
        if not os.path.exists(filepath):
            return

        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                config_data = json.load(f)

            for idx_str, item in config_data.items():
                idx = int(idx_str)
                if idx not in self.param_map:
                    continue
                widget = self.param_map[idx]
                val = item["value"]
                try:
                    if item["type"] == "uint8":
                        match idx:
                            case (PIdx.FOC_MODE | PIdx.LOOP_MODE | PIdx.SW_CANQUEUE |
                                  PIdx.MOTOR_WIRE_SEQUENCE | PIdx.SW_FAN |
                                  PIdx.SW_VAGUE_PID | PIdx.SW_PVT | PIdx.SW_WEAKMAG):
                                widget.setCurrentIndex(val)
                                if idx in self.param_show_map:
                                    self.param_show_map[idx].setText(widget.currentText())
                                if idx == PIdx.LOOP_MODE:
                                    self.mw.ui.controlval_show.setText(self.refvalue_map.get(val, ""))
                            case (PIdx.MOTOR_POLEPAIRS | PIdx.FREQ_CURRENT_LOOP |
                                  PIdx.FREQ_SPEED_LOOP | PIdx.FREQ_POSITION_LOOP):
                                widget.setText(str(val))
                    elif item["type"] == "int32":
                        widget.setText(str(val))
                        if idx in self.param_show_map:
                            self.param_show_map[idx].setText(str(val))
                    elif item["type"] == "float32":
                        widget.setText(f"{val:.6g}")
                except Exception as e:
                    print(f"加载参数 {idx} 失败: {e}")
                    continue
        except Exception as e:
            print(f"加载配置 '{display_name}' 失败: {e}")