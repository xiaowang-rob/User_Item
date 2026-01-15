# main.py (修正文件名拼写)
import sys
import time

from PyQt5.QtCore import QTimer
from PyQt5.QtWidgets import QApplication
from core.app import MainWindow  # 修正导入路径
from siui.core import SiGlobal
from resources.styles import GLOBAL_STYLES


if __name__ == "__main__":
    app = QApplication(sys.argv)
    
    SiGlobal.siui.loadColors(GLOBAL_STYLES)

    window = MainWindow()
    window.show()
    
    sys.exit(app.exec_())