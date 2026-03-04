import os
import json
from PyQt5.QtWidgets import QVBoxLayout, QHBoxLayout, QWidget
from PyQt5.QtCore import Qt
from siui.components.editbox import SiLabeledLineEdit
from siui.components.button import SiPushButtonRefactor
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
        # 修复：覆盖操作不再删除文件，直接写入
        cover_but.clicked.connect(lambda: self.cover_current_config(name_input.text()))

        button_layout.addWidget(save_but)
        button_layout.addWidget(cover_but)

        layout.addWidget(name_input)
        layout.addWidget(button_widget)

        send_custom_message(input_widget, MSG_TYPE_INFO)

    # ======================
    # 保存当前参数到配置文件（直接保存 param_list，不关心类型）
    # ======================
    def save_current_config(self, config_name=None):
        config_name = config_name.strip() if config_name else ""
        if not config_name:
            send_simple_message(MSG_TYPE_WARNING,"配置名不能为空",True,800)
            return
            
        if not config_name.endswith(".json"):
            config_name += ".json"

        filepath = os.path.join(self.config_dir, config_name)

        try:
            # 直接保存整个 param_list，不区分类型
            with open(filepath, 'w', encoding='utf-8') as f:
                json.dump({"params": self.mw.param_manager.param_list}, f, indent=4)
            
            self.refresh_config_list()
            display_name = config_name[:-5]
            index = self.combo_config.findText(display_name)
            if index >= 0:
                self.combo_config.setCurrentIndex(index)
                
            send_simple_message(MSG_TYPE_SUCCESS,f"配置“{display_name}”保存成功",True,800)
        except Exception as e:
            send_simple_message(MSG_TYPE_ERROR, f"保存失败：{str(e)}", True, 1000 )

    # ======================
    # 覆盖指定名称的配置（直接写入，不删除重建）
    # ======================
    def cover_current_config(self, new_config_name=None):
        """覆盖配置：直接写入文件，不先删除"""
        display_name = self.combo_config.currentText()
        if not display_name:
            send_simple_message(MSG_TYPE_WARNING,"请先选择一个配置", True,800)
            return
        
        # 如果输入了新名称，用新名称覆盖；否则用当前选中名称覆盖
        target_name = new_config_name.strip() if new_config_name and new_config_name.strip() else display_name
        
        if not target_name.endswith(".json"):
            target_name += ".json"
        
        filepath = os.path.join(self.config_dir, target_name)
        
        try:
            # 直接写入，不删除
            with open(filepath, 'w', encoding='utf-8') as f:
                json.dump({"params": self.mw.param_manager.param_list}, f, indent=4)
            
            self.refresh_config_list()
            display_name_clean = target_name[:-5]
            index = self.combo_config.findText(display_name_clean)
            if index >= 0:
                self.combo_config.setCurrentIndex(index)
                
            send_simple_message(MSG_TYPE_SUCCESS, f"配置“{display_name_clean}”覆盖成功", True, 800)
        except Exception as e:
            send_simple_message(MSG_TYPE_ERROR, f"覆盖失败：{str(e)}", True, 1000)

    # ======================
    # 【新增】严格覆盖当前选中的配置（不改名，直接替换内容）
    # ======================
    def cover_selected_config(self):
        """覆盖当前 ComboBox 选中的配置文件（不改名，不删除，直接写入）"""
        display_name = self.combo_config.currentText()
        if not display_name:
            send_simple_message(MSG_TYPE_WARNING, "请先选择一个配置", True, 800)
            return
        
        config_name = display_name + ".json"
        filepath = os.path.join(self.config_dir, config_name)
        
        try:
            with open(filepath, 'w', encoding='utf-8') as f:
                json.dump({"params": self.mw.param_manager.param_list}, f, indent=4)
            send_simple_message(MSG_TYPE_SUCCESS, f"配置“{display_name}”已覆盖", True, 800)
        except Exception as e:
            send_simple_message(MSG_TYPE_ERROR, f"覆盖失败：{str(e)}", True, 1000)

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
    # 内部方法：根据显示名称加载配置到 param_list
    # ======================
    def _load_config_by_name(self, display_name: str):
        config_name = display_name + ".json"
        filepath = os.path.join(self.config_dir, config_name)
        if not os.path.exists(filepath):
            send_simple_message(MSG_TYPE_WARNING, f"配置文件不存在", True, 1000)
            return

        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                raw_data = json.load(f)
            
            # 防护1：处理直接列表格式（旧版保存）
            if isinstance(raw_data, list):
                params = raw_data
            # 防护2：处理标准字典格式 {"params": [...]}
            elif isinstance(raw_data, dict) and "params" in raw_data:
                params = raw_data["params"]
            # 防护3：处理错误格式
            else:
                params = []
            
            # ====== 关键修复1：确保 params 是列表（在 len() 调用前） ======
            if not isinstance(params, list):
                params = []
            # ==========================================================
            
            # 使用 load_param 逐个加载参数，保留原始类型
            param_manager = self.mw.param_manager
            total_params = len(param_manager.param_list)
            loaded_count = 0
            
            for i in range(min(len(params), total_params)):
                try:
                    value = params[i]
                    # 关键修复2：仅做必要类型转换（字符串数字 → 数字），其他保留原类型
                    if isinstance(value, str):
                        # 尝试转为数字，失败则跳过
                        try:
                            value = float(value) if '.' in value or 'e' in value.lower() else int(value)
                        except ValueError:
                            continue  # 非数字字符串跳过
                    elif not isinstance(value, (int, float, bool)):
                        continue  # 非法类型跳过
                    
                    param_manager.load_param(i, value)  # 保留原始类型
                    loaded_count += 1
                except (IndexError, TypeError):
                    continue  # 安全跳过异常项
            
            send_simple_message(MSG_TYPE_SUCCESS, f"配置“{display_name}”加载成功", True, 800)
        except Exception as e:
            error_msg = f"加载失败: {type(e).__name__} - {str(e)[:100]}"
            send_simple_message(MSG_TYPE_ERROR, error_msg, True, 2000)  