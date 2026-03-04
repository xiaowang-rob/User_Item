from PyQt5.QtCore import Qt, QTimer

from ..layer import SiLayer


class LayerChildPage(SiLayer):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.child_page = None
        self.setAttribute(Qt.WA_TransparentForMouseEvents, True)

    def childPage(self):
        return self.child_page

    def setChildPage(self, page):
        # 如果是同一个页面，直接显示（避免重复初始化）
        if self.child_page is page:
            page.show()  # ✅ 确保显示
            page.raise_()  # ✅ 置顶
            self.showLayer()
            return
            
        if self.childPage() is not None:
            self.child_page.deleteLater()
            self.child_page = None

        self.child_page = page
        self.child_page.animationGroup().fromToken("move").setFactor(1/4)
        self.child_page.animationGroup().fromToken("move").setBias(0.5)
        self.child_page.setParent(self)
        
        # ✅ 关键：确保尺寸已正确计算
        self.child_page.content().adjustSize()  # 先调整内容区域
        self.child_page.adjustSize()            # 再调整页面本身
        
        # ✅ 兜底：如果尺寸仍为 0，使用 setSizeRatio 计算
        if self.child_page.width() < 100 or self.child_page.height() < 100:
            w = int(self.width() * 0.6)
            h = int(self.height() * 0.5)
            self.child_page.resize(w, h)
        
        # ✅ 关键：先 show 再 move，避免动画开始时 widget 未显示
        self.child_page.show()
        self.child_page.move((self.width() - self.child_page.width()) // 2, self.height())
        self.showLayer()

    def showLayer(self):
        super().showLayer()
        self.showChildPage()

    def closeLayer(self):
        super().closeLayer()
        self.closeChildPage()

    def showChildPage(self):
        self.setAttribute(Qt.WA_TransparentForMouseEvents, False)
        self.child_page.moveTo((self.width() - self.childPage().width()) // 2, self.height() - self.childPage().height())

    def closeChildPage(self):
        self.setAttribute(Qt.WA_TransparentForMouseEvents, True)
        if self.child_page is not None:
            self.child_page.moveTo((self.width() - self.childPage().width()) // 2, self.height())
            # 只隐藏，不删除，保留引用
            self.child_page.hide()
            # 注释掉以下两行删除逻辑：
            # self.child_page.delete_timer = QTimer()
            # self.child_page.delete_timer.singleShot(500, self.child_page.deleteLater)
            # self.child_page = None  # ❌ 不要清空引用！

    def resizeEvent(self, event):
        super().resizeEvent(event)
        if self.child_page is not None:
            self.child_page.adjustSize()
            self.child_page.move((self.width() - self.childPage().width()) // 2, self.height() - self.childPage().height())
