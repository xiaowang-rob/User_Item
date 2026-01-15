from siui.components.widgets import SiWidget

class ControlTab(SiWidget):
    def __init__(self, parent):
        super().__init__(parent)
        self.parent = parent
        
        # 背景色
        self.setStyleSheet("background-color: #2d2d2d;")
        
        # 这里添加控制参数控件
        # 例如：PID参数、速度设置、加速度限制等