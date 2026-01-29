from PyQt5.QtWidgets import QWidget, QHBoxLayout, QVBoxLayout, QScrollArea
from siui.components import (
    SiPushButton,
    SiTitledWidgetGroup,
)

class ParameterPage():
    def __init__(self, main_window):
        self.mw = main_window
        self.widget=main_window.ui.parameter_page


        main_layout=QHBoxLayout()
        main_layout.setContentsMargins(24,0,24,0)
        main_layout.setSpacing(24)

        self.control_group=QVBoxLayout(self.widget)
        self.control_group.setContentsMargins(0,0,0,0)
        self.control_group.setSpacing(12)


