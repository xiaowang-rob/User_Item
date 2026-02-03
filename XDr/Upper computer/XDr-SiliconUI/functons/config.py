import os
import json
from PyQt5.QtWidgets import QVBoxLayout, QHBoxLayout, QWidget
from PyQt5.QtCore import Qt
from siui.components.editbox import SiLabeledLineEdit
from siui.components.button import SiPushButtonRefactor
from UI.data_ui_map import Pidx, Midx
from functons.message_show import (
    send_custom_message,
    send_simple_message,
    MSG_TYPE_NORMAL,
    MSG_TYPE_SUCCESS,
    MSG_TYPE_INFO,
    MSG_TYPE_WARNING,
    MSG_TYPE_ERROR,
)


class Pconfig:
    def __init__(self, main_window):
        self.mw = main_window
        self.config_dir = "configs"
        os.makedirs(self.config_dir, exist_ok=True)

        self.combo_config = self.mw.top_area.config_file

        self.load_config_but=self.mw.top_area.load_config
        self.load_config_but.clicked.connect(self.load_selected_config)
        self.save_config_but=self.mw.top_area.save_config
        self.save_config_but.clicked.connect(self.opend_config_input)
        self.remove_config_but=self.mw.top_area.remove_config
        self.remove_config_but.longPressed.connect(self.delete_selected_config)

        self.param_map = self.mw.ui_map.param_map
        self.param_show_map = self.mw.ui_map.param_show_map
        self.target_val_show = self.mw.control_page.control_target_show

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


    def opend_config_input(self):
        input_widget = QWidget()
        input_widget.setFixedSize(300, 160)

        layout = QVBoxLayout(input_widget)
        layout.setContentsMargins(20, 15, 20, 15)
        layout.setSpacing(15)

        name_input = SiLabeledLineEdit()
        name_input.setTitle("配置名")
        name_input.setPlaceholderText("请输入配置名")
        name_input.setAlignment(Qt.AlignCenter)
        name_input.adjustSize()

        # 水平按钮布局
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

    # ======================
    # 保存当前参数到配置文件
    # ======================
    def save_current_config(self, config_name=None):
        config_name = config_name.strip() if config_name else ""
        if not config_name:
            send_simple_message(MSG_TYPE_WARNING,"配置名不能为空",True,800)
            return
            
        if not config_name.endswith(".json"):
            config_name += ".json"

        filepath = os.path.join(self.config_dir, config_name)

        # 收集当前所有参数值
        config_data = {}
        for idx, widget in self.param_map.items():
            try:
                if idx < Pidx.CAN_ID:
                    match idx:
                        case (Pidx.SENSOR_MODE | Pidx.LOOP_MODE | Pidx.CAN_MODE |
                              Pidx.MOTOR_WIRE_SEQUENCE | Pidx.FAN_MODE |
                              Pidx.VAGUE_PID_MODE | Pidx.PVT_MODE | Pidx.WEAKMAG_MODE):
                            val = widget.currentIndex()
                        case (Pidx.MOTOR_POLEPAIRS | Pidx.FREQ_CURRENT_LOOP |
                              Pidx.FREQ_SPEED_LOOP | Pidx.FREQ_POSITION_LOOP):
                            val = int(widget.text())
                    config_data[idx] = {"type": "uint8", "value": val}

                elif idx < Pidx.F_PWM:
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
                
            send_simple_message(MSG_TYPE_SUCCESS,f"配置“{display_name}”保存成功",True,800)
        except Exception as e:
            send_simple_message(MSG_TYPE_ERROR, f"保存失败：{str(e)}", True, 1000 )

    # ======================
    # 覆盖当前配置（直接用当前名称原地覆盖，无确认框）
    # ======================
    def cover_current_config(self,new_config_name=None):
        display_name = self.combo_config.currentText()
        if not display_name:
            send_simple_message( MSG_TYPE_WARNING,"请先选择一个配置", True,800)
            return
        new_name = new_config_name.strip() if new_config_name else ""
        self.delete_selected_config()
        if (not new_name) or (display_name == "default"):
            print("命名不改变")
            self.save_current_config(display_name)
        else:
            print("命名改变")
            self.save_current_config(new_name)


    # ======================
    # 加载选中的配置（通过 ComboBox）
    # ======================
    def load_selected_config(self):
        display_name = self.combo_config.currentText()
        if not display_name:
            return
        self._load_config_by_name(display_name)

    # ======================
    # 删除当前选中的配置（长按即确认，无确认框）
    # ======================
    def delete_selected_config(self):
        display_name = self.combo_config.currentText()
        if not display_name:
            send_simple_message( MSG_TYPE_WARNING,"请先选择一个配置",True,800 )
            return
        
        config_name = display_name + ".json"
        if config_name == "default.json":
            send_simple_message( MSG_TYPE_ERROR,"不能删除默认配置",True,800 )
            return

        # 长按即确认删除，直接执行
        filepath = os.path.join(self.config_dir, config_name)
        try:
            os.remove(filepath)
            self.refresh_config_list()
            # 自动切换到第一个配置（通常是 default）
            if self.combo_config.count() > 0:
                self.combo_config.setCurrentIndex(0)
            # 0.5s自动关闭的成功消息
            send_simple_message( MSG_TYPE_SUCCESS,f"配置“{display_name}”已删除", True, 800)
        except Exception as e:
            send_simple_message(MSG_TYPE_ERROR, f"删除失败：{str(e)}", True, 1000 )

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
            send_simple_message( MSG_TYPE_WARNING,f"配置文件不存在",True,1000 )
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
                            case (Pidx.SENSOR_MODE | Pidx.LOOP_MODE | Pidx.CAN_MODE |
                                  Pidx.MOTOR_WIRE_SEQUENCE | Pidx.FAN_MODE |
                                  Pidx.VAGUE_PID_MODE | Pidx.PVT_MODE | Pidx.WEAKMAG_MODE):
                                widget.setCurrentIndex(val)
                                if idx in self.param_show_map:
                                    self.param_show_map[idx].setText(widget.currentText())
                                if idx == Pidx.LOOP_MODE:
                                    self.target_val_show.setText(Midx.target_value[val])
                            case (Pidx.MOTOR_POLEPAIRS | Pidx.FREQ_CURRENT_LOOP |
                                  Pidx.FREQ_SPEED_LOOP | Pidx.FREQ_POSITION_LOOP):
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
            send_simple_message( MSG_TYPE_ERROR,f"加载配置失败：{str(e)}",True,2000 )