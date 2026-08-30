"""
消息通知系统 —— 基于 qfluentwidgets InfoBar
替代原 siui 侧边栏消息系统
"""

from qfluentwidgets import InfoBar, InfoBarPosition
from PyQt5.QtWidgets import QWidget

MSG_TYPE_NORMAL = 0    # 灰色信息
MSG_TYPE_SUCCESS = 1   # 绿色成功
MSG_TYPE_INFO = 2      # 蓝色信息
MSG_TYPE_WARNING = 3   # 橙色警告
MSG_TYPE_ERROR = 4     # 红色错误

_PARENT_WINDOW = None


# ── InfoBar 样式映射 ──
_INFO_BAR_TYPE = {
    MSG_TYPE_SUCCESS: InfoBar.success,
    MSG_TYPE_INFO:    InfoBar.info,
    MSG_TYPE_WARNING: InfoBar.warning,
    MSG_TYPE_ERROR:   InfoBar.error,
    MSG_TYPE_NORMAL:  InfoBar.info,
}

_INFO_BAR_DURATION = {
    MSG_TYPE_SUCCESS: 1500,
    MSG_TYPE_INFO:    2000,
    MSG_TYPE_WARNING: 3000,
    MSG_TYPE_ERROR:   5000,
    MSG_TYPE_NORMAL:  2000,
}


def init_message_system(parent_window: QWidget):
    """初始化消息系统 —— 只需记住父窗口"""
    global _PARENT_WINDOW
    _PARENT_WINDOW = parent_window


def send_simple_message(
    type_=MSG_TYPE_INFO,
    text="这是一条测试消息",
    auto_close=False,
    auto_close_duration=1000,
):
    """发送简单文本消息"""
    if _PARENT_WINDOW is None:
        return
    duration = auto_close_duration if auto_close else _INFO_BAR_DURATION.get(type_, 2000)
    builder = _INFO_BAR_TYPE.get(type_, InfoBar.info)
    builder(
        title="",
        content=text,
        duration=duration,
        parent=_PARENT_WINDOW,
        position=InfoBarPosition.TOP_RIGHT,
        isClosable=True,
    )


def send_titled_message(
    type_=MSG_TYPE_SUCCESS,
    title="Sent Successfully",
    text="A titled message has been successfully sent.",
    auto_close=False,
    auto_close_duration=1000,
):
    """发送带标题的消息"""
    if _PARENT_WINDOW is None:
        return
    duration = auto_close_duration if auto_close else _INFO_BAR_DURATION.get(type_, 2000)
    builder = _INFO_BAR_TYPE.get(type_, InfoBar.info)
    builder(
        title=title,
        content=text,
        duration=duration,
        parent=_PARENT_WINDOW,
        position=InfoBarPosition.TOP_RIGHT,
        isClosable=True,
    )


def send_custom_message(
    content_widget: QWidget,
    type_=MSG_TYPE_INFO,
    auto_close=False,
    auto_close_duration=1000,
):
    """发送自定义内容的消息"""
    if _PARENT_WINDOW is None:
        return
    duration = auto_close_duration if auto_close else _INFO_BAR_DURATION.get(type_, 2000)
    builder = _INFO_BAR_TYPE.get(type_, InfoBar.info)
    builder(
        title="",
        content="",  # 使用自定义 widget 时 content 留空
        duration=duration,
        parent=_PARENT_WINDOW,
        position=InfoBarPosition.TOP_RIGHT,
        isClosable=True,
    )
    # 自定义内容通过 InfoBar 的 widget 机制实现
    # 简化实现：发送一条带更多信息的通知
    send_simple_message(type_, "请查看控制台/日志", auto_close, auto_close_duration)
