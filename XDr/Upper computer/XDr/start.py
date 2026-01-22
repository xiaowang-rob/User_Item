import sys
from PyQt5.QtWidgets import QApplication
from ui.main_window import MainWindow

if __name__ == "__main__":
    app = QApplication(sys.argv)
    # 应用深色主题
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())