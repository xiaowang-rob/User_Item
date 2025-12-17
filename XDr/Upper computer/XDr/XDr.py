import sys
#import qdarkstyle
# 从 PyQt5 的 QtWidgets 模块中导入两个核心类：
# - QApplication：管理 GUI 应用程序的控制流和主要设置（每个 PyQt 程序必须有且仅有一个 QApplication 实例）
# - QMainWindow：提供主窗口框架（包含菜单栏、工具栏、状态栏和中心区域）
from PyQt5.QtWidgets import QApplication, QMainWindow

# 导入由 Qt Designer 的 .ui 文件通过 pyuic5 工具生成的 UI 类
# 假设你的 .ui 文件名为 XDrmain.ui，生成的 Python 文件为 XDrmain_ui.py
# 其中定义了一个名为 Ui_XDr 的类，负责构建界面控件
from XDrmain_ui import Ui_XDr
#from user_XDr_ui import Ui_XDr

# 定义一个自定义的主窗口类，继承自 QMainWindow
class MainWindow(QMainWindow):
    def __init__(self):
        # 调用父类（QMainWindow）的构造函数，完成基础初始化
        super().__init__()
          
        # 创建 Ui_XDr 类的实例，该实例包含了所有界面控件的定义
        self.ui = Ui_XDr()
        
        # 调用 setupUi 方法，将界面控件“安装”到当前主窗口（self）上
        # 这一步会把 .ui 文件中设计的所有按钮、标签、布局等添加到窗口中
        self.ui.setupUi(self)




# 程序入口点：如果直接运行本文件__name__会被设置为"__main__"   在其他地方导入的话 __name__会被设置为"XDr"
if __name__ == "__main__":
    # 创建 QApplication 实例，传入命令行参数（sys.argv）
    # 这是 PyQt 应用的起点，负责事件循环、资源管理等
    app = QApplication(sys.argv)
    
    # 创建自定义主窗口实例
    window = MainWindow()
    
    #app.setStyleSheet(qdarkstyle.load_stylesheet_pyqt5())
    # 显示窗口（默认创建的窗口是隐藏的，必须调用 show() 才可见）
    window.show()
    
    # 启动应用程序的事件循环：
    # - app.exec_() 会一直运行，直到用户关闭窗口或调用退出
    # - sys.exit() 确保程序以正确的退出码结束（0 表示正常退出）
    sys.exit(app.exec_())