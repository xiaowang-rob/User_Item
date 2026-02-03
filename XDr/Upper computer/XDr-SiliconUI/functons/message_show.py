
from PyQt5.QtWidgets import QWidget
from siui.templates.application.components.layer.layer_right_message_sidebar.layer_right_message_sidebar import LayerRightMessageSidebar
from siui.templates.application.components.message.box import SiSideMessageBox

MSG_TYPE_NORMAL = 0    # 白色
MSG_TYPE_SUCCESS = 1   # 绿色成功
MSG_TYPE_INFO = 2      # 蓝色信息
MSG_TYPE_WARNING = 3   # 黄色警告
MSG_TYPE_ERROR = 4     # 红色错误

# 全局侧边栏实例
_GLOBAL_SIDEBAR = None


def init_message_system(parent_window: QWidget):
    global _GLOBAL_SIDEBAR
    if _GLOBAL_SIDEBAR is not None:
        return

    _GLOBAL_SIDEBAR = LayerRightMessageSidebar(parent_window)
    
    # 👇 立即设置初始位置（避免第一次 resize 前位置为 (0,0)）
    parent_rect = parent_window.geometry()
    _GLOBAL_SIDEBAR.setGeometry(
        parent_rect.width() - 400,
        80,
        400,
        min(600, parent_rect.height() - 100)
    )
    
    _GLOBAL_SIDEBAR.hide()  # 初始隐藏，发送时再 show

    # 绑定 resize
    original_resize = parent_window.resizeEvent
    def new_resize(event):
        w = event.size().width()
        h = event.size().height()
        _GLOBAL_SIDEBAR.setGeometry(w - 400, 80, 400, min(600, h - 100))
        original_resize(event)
    parent_window.resizeEvent = new_resize


def send_simple_message(
    type_=MSG_TYPE_INFO,
    text="这是一条测试消息\n比具标题信息更加简洁方便",
    auto_close=False,
    auto_close_duration=1000,
):
    if _GLOBAL_SIDEBAR is None:
        raise RuntimeError("请先调用 init_message_system(main_window)")
    fold_after = auto_close_duration if auto_close else None
    _GLOBAL_SIDEBAR.send(text=text, msg_type=type_, fold_after=fold_after)
    _GLOBAL_SIDEBAR.show()
    _GLOBAL_SIDEBAR.raise_()

def send_titled_message(
    type_=MSG_TYPE_SUCCESS,
    title="Sent Successfully",
    text="A titled message has been successfully sent to the sidebar.",
    auto_close=False,
    auto_close_duration=1000,
):
    if _GLOBAL_SIDEBAR is None:
        raise RuntimeError("请先调用 init_message_system(main_window)")
    fold_after = auto_close_duration if auto_close else None
    _GLOBAL_SIDEBAR.send(title=title, text=text, msg_type=type_, fold_after=fold_after)
    _GLOBAL_SIDEBAR.show()
    _GLOBAL_SIDEBAR.raise_()

def send_custom_message(
    content_widget: QWidget,
    type_=MSG_TYPE_INFO,
    auto_close=False,
    auto_close_duration=1000,
):
    """
    发送自定义消息到侧边栏。
    
    :param content_widget: 已构建好的 QWidget，作为消息主体内容（会直接加入消息框容器）
    :param type_: 消息类型（颜色风格）
    :param auto_close: 是否自动关闭
    :param auto_close_duration: 自动关闭延迟（毫秒）
    """
    if _GLOBAL_SIDEBAR is None:
        raise RuntimeError("请先调用 init_message_system(main_window)")


    fold_after = auto_close_duration if auto_close else None

    # 创建消息框
    message_box = SiSideMessageBox()
    message_box.setMessageType(type_)

    # 将用户提供的 widget 添加到消息框的内容容器中
    container = message_box.content().container()
    container.setSpacing(0)
    container.addPlaceholder(16)
    container.addWidget(content_widget)
    container.addPlaceholder(16)

    # 调整大小以适应内容
    message_box.adjustSize()

    if fold_after is not None:
        message_box.setFoldAfter(fold_after)

    # 发送并显示
    _GLOBAL_SIDEBAR.sendMessageBox(message_box)
    _GLOBAL_SIDEBAR.show()
    _GLOBAL_SIDEBAR.raise_()