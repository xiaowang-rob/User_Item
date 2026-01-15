from PyQt5.QtWidgets import QMainWindow
from siui.core import SiGlobal
from ui.main_window import MainWindowContent
from siui.components.widgets import SiWidget

class MainWindow(QMainWindow):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        
        # 设置窗口属性
        self.setWindowTitle("XDr")
        self.resize(1400, 900)
    

        SiGlobal.siui.windows["MAINWINDOW"] = self
        
        # 创建主内容
        self.content = MainWindowContent(self)
        self.setCentralWidget(self.content)
    
    def resizeEvent(self, event):
        super().resizeEvent(event)
        # MainWindowContent 会处理内部布局

    def content(self):
        return self.content