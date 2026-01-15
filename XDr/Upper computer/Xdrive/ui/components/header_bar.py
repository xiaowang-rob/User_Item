from siui.components.widgets import SiWidget
from siui.components.widgets import SiDenseVContainer, SiDenseHContainer
from siui.components import SiPushButton
from siui.components.widgets import SiLabel
from siui.core import SiGlobal

from siui.components.button import (
    SiCapsuleButton,
    SiCheckBox,
    SiCheckBoxRefactor,
    SiFlatButton,
    SiFlatButtonWithIndicator,
    SiLongPressButtonRefactor,
    SiOptionButton,
    SiProgressPushButton,
    SiPushButtonRefactor,
    SiRadioButton,
    SiRadioButtonR,
    SiRadioButtonWithAvatar,
    SiRadioButtonWithDescription,
    SiSwitchRefactor,
    SiToggleButtonRefactor,
)
from siui.components.combobox_ import SiCapsuleComboBox

class HeaderBar(SiWidget):
    def __init__(self, parent):
        super().__init__(parent)
        self.parent = parent
        
        # 从 SiGlobal 颜色组获取背景色
        bg_color = SiGlobal.siui.colors["header_background"]
        self.setStyleSheet(f"background-color: {bg_color};")
        
        # 创建主垂直容器，用于四行布局
        self.main_container = SiDenseVContainer(self)
        self.main_container.setSpacing(8)  # 行间距
        self.main_container.setAdjustWidgetsSize(True)
        
        # 创建四行容器
        self.row1 = SiDenseHContainer(self)
        self.row1.setSpacing(10)
        self.row1.setAdjustWidgetsSize(True)
        
        self.row2 = SiDenseHContainer(self)
        self.row2.setSpacing(10)
        self.row2.setAdjustWidgetsSize(True)
        
        self.row3 = SiDenseHContainer(self)
        self.row3.setSpacing(10)
        self.row3.setAdjustWidgetsSize(True)
        
        self.row4 = SiDenseHContainer(self)
        self.row4.setSpacing(10)
        self.row4.setAdjustWidgetsSize(True)
        
        # 创建控件
        # 第一行
        self.device_combo = SiCapsuleComboBox(self)
        self.device_combo.resize(120, 32)
        self.device_combo.setTitle("设备")
        self.device_combo.setEditable(False)
        self.device_combo.addItem("未选择设备", "请选择设备")
        
        self.connect_btn = SiCapsuleButton(self)
        self.connect_btn.resize(120, 32)
        self.connect_btn.setText("连接")
        self.connect_btn.setToolTip("未连接")
        
        # 第二行示例
        self.second_col1 = SiCapsuleButton(self)
        self.second_col1.resize(100, 32)
        self.second_col1.setText("第二行1")
        
        self.second_col2 = SiCapsuleButton(self)
        self.second_col2.resize(100, 32)
        self.second_col2.setText("第二行2")
        
        # 第三行示例
        self.third_col1 = SiCapsuleButton(self)
        self.third_col1.resize(80, 32)
        self.third_col1.setText("第三行")
        
        # 第四行 - 导航栏
        self.nav_btn1 = SiCapsuleButton(self)
        self.nav_btn1.resize(100, 32)
        self.nav_btn1.setText("参数")
        
        self.nav_btn2 = SiCapsuleButton(self)
        self.nav_btn2.resize(100, 32)
        self.nav_btn2.setText("日志")
        
        self.nav_btn3 = SiCapsuleButton(self)
        self.nav_btn3.resize(100, 32)
        self.nav_btn3.setText("控制")
        
        # 添加控件到行容器
        self.row1.addWidget(self.device_combo, side="left")
        self.row1.addWidget(self.connect_btn, side="left")
        
        self.row2.addWidget(self.second_col1, side="left")
        self.row2.addWidget(self.second_col2, side="left")
        
        self.row3.addWidget(self.third_col1, side="left")
        
        # 第四行 - 添加导航按钮
        self.row4.addWidget(self.nav_btn1, side="left")
        self.row4.addWidget(self.nav_btn2, side="left")
        self.row4.addWidget(self.nav_btn3, side="left")
        
        # 将行容器添加到主容器
        self.main_container.addWidget(self.row1, side="top")
        self.main_container.addWidget(self.row2, side="top")
        self.main_container.addWidget(self.row3, side="top")
        self.main_container.addWidget(self.row4, side="top")
        
        # 初始化时调整容器大小
        self._setup_geometry()

    def _setup_geometry(self):
        # 设置容器几何尺寸和大小
        self.main_container.setGeometry(0, 0, self.width(), self.height())
        # 强制调整容器大小以适应内容
        self.main_container.adjustSize()

    def resizeEvent(self, event):
        super().resizeEvent(event)
        # 设置容器几何尺寸
        self.main_container.setGeometry(0, 0, self.width(), self.height())
        # 调整容器大小以适应新的尺寸
        self.main_container.adjustSize()