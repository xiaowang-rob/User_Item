from siui.components.widgets.navigation_bar import SiNavigationBarH
from PyQt5.QtWidgets import  QHBoxLayout
from siui.components.button import SiPushButtonRefactor
from PyQt5.QtCore import Qt

class MiddleArea:
    def __init__(self,main_window):
        self.mw = main_window
        self.cmdbutton_area=main_window.ui.cmdbutton_area
        self.navegation_area = self.mw.ui.navegation_area
        self.navegation_area.setMinimumSize(200, 42)
        
        navegation_layout = QHBoxLayout(self.navegation_area)
        navegation_layout.setContentsMargins(16, 0, 16, 0)  # 无内边距
        navegation_layout.setSpacing(0)  # 按钮间距
        navegation_layout.setAlignment(Qt.AlignBottom)

        self.navegation_bar = SiNavigationBarH(self.mw.ui.navegation_area)
        self.navegation_bar.addItem("参数")
        self.navegation_bar.addItem("日志")
        self.navegation_bar.addItem("控制")
        
        self.navegation_bar.setCurrentIndex(0)
        self.navegation_bar.adjustSize()
        
        navegation_layout.addWidget(self.navegation_bar)

        self.navegation_bar.indexChanged.connect(self.mw.ui.stackedWidget.setCurrentIndex)
        self.navegation_bar._on_index_changed(0)

        self.ENable_button=SiPushButtonRefactor()
        self.ENable_button.setText("使能")
        self.ENable_button.adjustSize()

        self.DEnable_button=SiPushButtonRefactor()
        self.DEnable_button.setText("失能")
        self.DEnable_button.adjustSize()

        self.reset_button=SiPushButtonRefactor()
        self.reset_button.setText("复位")
        self.reset_button.adjustSize()

        self.tunningstart_button=SiPushButtonRefactor()
        self.tunningstart_button.setText("开始整定")
        self.tunningstart_button.adjustSize()

        self.brake_button=SiPushButtonRefactor()
        self.brake_button.setText("制动")
        self.brake_button.adjustSize()

        self.protectreset_button=SiPushButtonRefactor()
        self.protectreset_button.setText("保护复位")
        self.protectreset_button.adjustSize()


        button_layout = QHBoxLayout(self.cmdbutton_area)
        button_layout.setContentsMargins(24, 0, 12, 0)  # 无内边距
        button_layout.setSpacing(16)  # 按钮间距
        button_layout.addWidget(self.ENable_button)
        button_layout.addWidget(self.DEnable_button)
        button_layout.addWidget(self.reset_button)
        button_layout.addWidget(self.tunningstart_button)
        button_layout.addWidget(self.brake_button)
        button_layout.addWidget(self.protectreset_button)



