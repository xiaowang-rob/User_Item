from siui.components.widgets import SiWidget

class ParamsTab(SiWidget):
    def __init__(self, parent):
        super().__init__(parent)
        self.parent = parent
        
        # 背景色
        self.setStyleSheet("background-color: #2d2d2d;")
        
        # 这里添加参数管理控件
        # 目前只是空白页面