from siui.components.widgets import SiWidget

class LogsTab(SiWidget):
    def __init__(self, parent):
        super().__init__(parent)
        self.parent = parent
        
        # 背景色
        self.setStyleSheet("background-color: #2d2d2d;")
        
        # 这里添加日志管理控件
        # 目前只是空白页面