import ctypes
import os
import sys

def get_windows_scaling_factor():
    # Linux 下直接返回 1，不调用 Windows API
    if sys.platform != "win32":
        return 1.0
    try:
        user32 = ctypes.windll.user32
        user32.SetProcessDPIAware()
        scaling_factor = user32.GetDpiForSystem()
        return scaling_factor / 96.0
    except Exception as e:
        print("无法获取缩放比例，设置为1，错误:", e)
        return 1


def reload_scale_factor():
    # 只在 Windows 下设置 QT_SCALE_FACTOR
    if sys.platform == "win32":
        set_scale_factor(get_windows_scaling_factor(), identity="Windows API")
    # Linux 下不做任何缩放设置，交由 Qt 的 High DPI 属性处理


def set_scale_factor(factor, identity="External calls"):
    os.environ["QT_SCALE_FACTOR"] = str(factor)
    print("已将环境变量 QT_SCALE_FACTOR 设为", factor, f" (来源: {identity})")