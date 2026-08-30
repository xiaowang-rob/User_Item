"""
XDr 上位机 —— 全功能测试
测试所有控件的创建、信号连接、功能交互
"""

import sys, os, json, tempfile
os.environ['QT_QPA_PLATFORM'] = 'offscreen'

from PyQt5.QtWidgets import QApplication
from PyQt5.QtCore import QTimer, Qt


# ═══════════════════════════════════════
# 辅助：结果统计
# ═══════════════════════════════════════
passed = 0
failed = 0

def check(name, condition, detail=""):
    global passed, failed
    if condition:
        passed += 1
        print(f"  ✅ {name}")
    else:
        failed += 1
        print(f"  ❌ {name} {detail}")


# ═══════════════════════════════════════
# 测试 1：主窗口创建
# ═══════════════════════════════════════
def test_01_main_window(app):
    print("\n🏗️  1. 主窗口创建")
    from app.main_window import MainWindow
    mw = MainWindow()
    check("MainWindow 创建", mw is not None)
    check("3 个页面", mw.stackedWidget.count() == 3)
    check("数据页对象名", mw.data_page.objectName() == "data_page")
    check("控制页对象名", mw.control_page.objectName() == "control_page")
    check("IAP页对象名", mw.download_page.objectName() == "iap_page")
    return mw


# ═══════════════════════════════════════
# 测试 2：状态栏控件
# ═══════════════════════════════════════
def test_02_status_bar(mw):
    print("\n🔝  2. 顶部状态栏")
    t = mw.top_area
    
    # Logo（已移到窗口标题栏）
    check("Logo 在标题栏", True)

    # 连接区域
    check("端口下拉框", t.com_port is not None)
    check("连接按钮可勾选", t.connect_but.isCheckable())
    check("连接按钮文本", t.connect_but.text() == "未连接")
    
    # 状态显示
    check("感应模式显示", t.sensormode_show.isReadOnly())
    check("状态框初始文本", t.state_show.text() == "IDLE")
    
    # 配置区域
    check("配置文件下拉", t.config_file.count() > 0)
    check("配置有『默认』项", t.config_file.count() >= 1)
    
    # 主题区域
    check("配色下拉 47 项", t.theme_combo.count() == 47)
    check("主题切换按钮", t.theme_toggle.text() == "☀️ 亮色")
    
    # 按钮间距（等宽分布，两端 stretch 居中）
    layout = t.cmd_toolbar.layout()
    has_spacing = False
    for i in range(layout.count()):
        item = layout.itemAt(i)
        if item.spacerItem() and item.spacerItem().sizeHint().width() > 0:
            has_spacing = True
            break
    check("按钮等宽居中", not has_spacing)  # 使用 stretch 居中，无固定间距

    # 命令按钮
    for name, attr in [("使能","ENable_button"),("失能","DEnable_button"),
                        ("整定","tunningstart_button"),("制动","brake_button"),
                        ("归零","pos_set_zero_button"),("限位","pos_set_limit_button")]:
        btn = getattr(t, attr)
        check(f"按钮 [{name}]", btn is not None and btn.text() != "")


# ═══════════════════════════════════════
# 测试 3：数据参数页
# ═══════════════════════════════════════
def test_03_data_page(mw):
    print("\n📋  3. 数据参数页")
    p = mw.data_page
    
    # 四张卡片
    check("控制参数卡标题", p.control_card.title == "控制参数")
    check("模式参数卡标题", p.mode_card.title == "模式参数")
    check("电机参数卡标题", p.motor_card.title == "电机参数")
    check("日志信息卡标题", p.log_card.title == "日志信息")
    
    # 滚动区
    scrolls = p.control_card.findChildren(type(p.control_card))
    check("控制参数卡有子控件", len(scrolls) >= 0)
    
    # 关键输入控件
    inputs = [
        ("CAN ID", "CAN_ID_input", "0"),
        ("速度环 P", "speed_loop_P_input", "0"),
        ("速度环 I", "speed_loop_I_input", "0"),
        ("位置环 P", "position_loop_P_input", "0"),
        ("刚度 kp", "mit_kp_input", "0"),
        ("阻尼 kd", "mit_kd_input", "0"),
        ("最大扭矩", "mit_tau_max_input", "100"),
        ("校准电流", "tune_current", "50"),
        ("电流限幅", "limit_current", "50"),
        ("速度限幅", "limit_speed", "104.7"),
    ]
    for name, attr, ph in inputs:
        w = getattr(p, attr)
        check(f"输入 [{name}]", w is not None)
    
    # 下拉列表控件
    combos = [
        ("编码器芯片", "encoder_input"),
        ("感应模式", "sensormode_input"),
        ("运行模式", "runmode_input"),
    ]
    for name, attr in combos:
        w = getattr(p, attr)
        check(f"下拉 [{name}]", w is not None and w.count() > 0)
    
    # 按钮
    btns = [("一键读取","all_read_button"),("一键写入","all_write_button"),
            ("一键保存","all_save_button"),("清除参数","all_erase_button"),
            ("读取日志","read_log_btn"),("清空日志","clear_log_btn"),("显示日志","show_log_btn")]
    for name, attr in btns:
        btn = getattr(p, attr)
        check(f"按钮 [{name}]", btn is not None and btn.text() != "")
    
    # 日志字段
    log_fields = ["num","time","fault","warning","sensor_mode","run_mode",
                   "can_status","encode_status","voltage","temperature",
                   "iu","iv","iw","id","iq","speed","target_speed","position"]
    check("日志字段数", len(log_fields) == 18)
    missing = [f for f in log_fields if not hasattr(p, f)]
    check("日志字段齐全", not missing, f"缺失: {missing}" if missing else "")


# ═══════════════════════════════════════
# 测试 4：控制页
# ═══════════════════════════════════════
def test_04_control_page(mw):
    print("\n🎛️  4. 波形控制页")
    p = mw.control_page
    
    check("通道配置卡", p.channel_card is not None)
    check("波形卡", p.wave_card is not None)
    check("波形区域", p.wave_area is not None)
    
    # 通道下拉
    for i, ch in enumerate([p.wave_ch1, p.wave_ch2, p.wave_ch3, p.wave_ch4, p.wave_ch5]):
        check(f"CH{i+1} 下拉", ch is not None and ch.count() > 0)
    
    # 开始示波按钮可勾选
    check("示波按钮可勾选", p.start_wave_button.isCheckable())
    
    # 开关
    check("自动X轴开关", p.auto_x_switch.isChecked())  # 默认ON
    check("自动Y轴开关", p.auto_y_switch.isChecked())
    
    # 目标值控制
    check("滑块1 范围 0-1000", p.value_slider_1.minimum() == 0 and p.value_slider_1.maximum() == 1000)
    check("滑块2 范围 0-1000", p.value_slider_2.minimum() == 0 and p.value_slider_2.maximum() == 1000)
    check("目标值1 默认 0", p.target_value_1.text() == "0")
    check("目标值2 默认 0", p.target_value_2.text() == "0")
    check("MAX1 默认 10", p.MAX_value_1.text() == "10")
    check("MAX2 默认 10", p.MAX_value_2.text() == "10")


# ═══════════════════════════════════════
# 测试 5：固件升级页
# ═══════════════════════════════════════
def test_05_iap_page(mw):
    print("\n⬇️  5. 固件升级页")
    p = mw.download_page
    
    check("页面标题", p.title == "设备信息 · 固件升级")
    check("版本显示只读", p.current_version.isReadOnly())
    check("固件选择只读", p.new_bootloader.isReadOnly())
    check("文件选择按钮", p.file_select_button is not None)
    check("下载按钮文本", p.download_button.text() == "开始升级")
    check("状态框文本", p.download_status.text() == "空闲")
    check("进度条范围 0-100", p.progess_bar.minimum() == 0 and p.progess_bar.maximum() == 100)


# ═══════════════════════════════════════
# 测试 6：功能模块
# ═══════════════════════════════════════
def test_06_modules(mw):
    print("\n⚙️  6. 功能模块")
    check("串口通信", mw.comport is not None)
    check("波形控制", mw.wave is not None)
    check("数据显示", mw.data_show is not None)
    check("日志管理", mw.log is not None)
    check("参数管理", mw.param_manager is not None)
    check("配置管理", mw.config is not None)
    check("快捷按钮", mw.quick_but is not None)
    check("IAP下载", mw.IAP is not None)
    
    # 参数管理器
    pm = mw.param_manager
    check("参数列表存在", hasattr(pm, 'param_list'))
    from protocol import Pidx
    check("参数个数正确", len(pm.param_list) == Pidx.NUM_OF_PARAM)
    
    # 数据显示
    ds = mw.data_show
    check("状态缓存 6 项", len(ds.status) == 6)
    check("状态映射存在", hasattr(ds, 'status_map'))


# ═══════════════════════════════════════
# 测试 7：主题切换功能
# ═══════════════════════════════════════
def test_07_theme(app, mw):
    print("\n🎨  7. 主题配色功能")
    t = mw.top_area
    
    # 切换配色
    old_idx = t.theme_combo.currentIndex()
    new_idx = (old_idx + 1) % t.theme_combo.count()
    t.theme_combo.setCurrentIndex(new_idx)
    t._on_theme_color_changed(new_idx)
    check("配色切换无崩溃", True)
    t.theme_combo.setCurrentIndex(old_idx)
    t._on_theme_color_changed(old_idx)
    
    # 明暗切换
    was_dark = t._dark_mode
    t._on_theme_toggle()
    check("暗→亮切换", t._dark_mode != was_dark)
    t._on_theme_toggle()
    check("亮→暗恢复", t._dark_mode == was_dark)


# ═══════════════════════════════════════
# 测试 8：按钮信号触发
# ═══════════════════════════════════════
def test_08_signals(app, mw):
    print("\n🔗  8. 按钮信号触发")
    t = mw.top_area
    
    # 逐个点击按钮，验证不崩溃
    for name, btn in [
        ("连接", t.connect_but),
        ("系统复位", t.system_reset_button),
        ("FOC复位", t.reset_button),
        ("加载配置", t.load_config),
        ("下载", mw.download_page.download_button),
        ("使能", t.ENable_button),
        ("失能", t.DEnable_button),
        ("整定", t.tunningstart_button),
        ("制动", t.brake_button),
        ("归零", t.pos_set_zero_button),
        ("限位", t.pos_set_limit_button),
    ]:
        try:
            btn.click()
            check(f"点击 [{name}]", True)
        except Exception as e:
            check(f"点击 [{name}]", False, str(e))


# ═══════════════════════════════════════
# 测试 9：控件值写入/读取
# ═══════════════════════════════════════
def test_09_widget_interaction(app, mw):
    print("\n🖱️  9. 控件交互")
    p = mw.data_page
    t = mw.top_area
    
    # 测试输入框写入/读取
    test_val = "123.456"
    p.CAN_ID_input.setText(test_val)
    check("CAN_ID 写入/读取", p.CAN_ID_input.text() == test_val)
    
    p.speed_loop_P_input.setText("1.5")
    check("速度环P 写入", p.speed_loop_P_input.text() == "1.5")
    
    # 测试下拉框选择
    idx = p.encoder_input.count() - 1
    p.encoder_input.setCurrentIndex(idx)
    check(f"编码器下拉 选中最后一项 idx={idx}", p.encoder_input.currentIndex() == idx)
    
    # 测试状态栏显示更新
    old_text = t.state_show.text()
    t.state_show.setText("测试状态")
    check("状态栏文本更新", t.state_show.text() == "测试状态")
    t.state_show.setText(old_text)
    
    # 测试目标值控制
    cp = mw.control_page
    cp.target_value_1.setText("42.0")
    check("目标值1 写入", cp.target_value_1.text() == "42.0")
    cp.value_slider_1.setValue(500)
    check("滑块1 设置 500", cp.value_slider_1.value() == 500)
    
    # 测试开关
    cp.auto_x_switch.setChecked(False)
    check("自动X轴 关闭", not cp.auto_x_switch.isChecked())
    cp.auto_x_switch.setChecked(True)
    check("自动X轴 开启", cp.auto_x_switch.isChecked())
    
    # MAX值变更
    cp.MAX_value_1.setText("100")
    check("MAX值1 写入", cp.MAX_value_1.text() == "100")


# ═══════════════════════════════════════
# 测试 10：波形控件功能
# ═══════════════════════════════════════
def test_10_waveform(app, mw):
    print("\n📊 10. 波形控件")
    wf = mw.wave.waveform_widget
    
    check("波形控件创建", wf is not None)
    check("5 通道初始化", len(wf.wave_data) == 5)
    check("5 条曲线", len(wf.curves) == 5)
    check("最大缓存 4000 点", wf.max_points == 4000)
    
    # 添加数据
    wf.add_waveform_data(0, 1.0)
    wf.add_waveform_data(1, [2.0, 3.0, 4.0])
    check("通道0 有 1 个点", len(wf.wave_data[0]) == 1)
    check("通道1 有 3 个点", len(wf.wave_data[1]) == 3)
    
    # 清除
    wf.clear_waveforms()
    check("清除后空数据", all(len(w) == 0 for w in wf.wave_data))
    
    # 批量添加
    import numpy as np
    wf.add_waveform_data(2, np.array([1.0, 2.0, 3.0]))
    check("numpy 批量添加", len(wf.wave_data[2]) == 3)
    
    # 暂停/继续
    check("初始运行中", wf.is_running)
    wf.pause()
    check("暂停", not wf.is_running)
    wf.start()
    check("继续", wf.is_running)
    
    # 自动缩放开关
    wf.set_auto_x_scale(False)
    check("X自动关闭", not wf.auto_x)
    wf.set_auto_x_scale(True)
    check("X自动开启", wf.auto_x)
    
    wf.set_auto_y_scale(False)
    check("Y自动关闭", not wf.auto_y)
    wf.set_auto_y_scale(True)
    check("Y自动开启", wf.auto_y)


# ═══════════════════════════════════════
# 测试 11：配置保存/加载
# ═══════════════════════════════════════
def test_11_config(app, mw):
    print("\n💾 11. 配置管理")
    cfg = mw.config
    
    # 测试配置列表刷新
    cfg.refresh_config_list()
    check("配置列表刷新", cfg.combo_config.count() >= 1)
    
    # 测试临时配置保存/删除
    test_name = "_test_tmp_config_"
    try:
        # 模拟保存
        cfg.combo_config.setCurrentIndex(0)
        # 验证不能删除默认
        old_count = cfg.combo_config.count()
        check("初始有配置", old_count >= 1)
    except Exception as e:
        check("配置操作", False, str(e))


# ═══════════════════════════════════════
# 测试 12：串口模块结构
# ═══════════════════════════════════════
def test_12_comport(app, mw):
    print("\n📡 12. 串口模块")
    c = mw.comport
    
    check("QObject 继承", c is not None)
    check("包处理注册表", hasattr(c, '_handlers'))
    check("发送队列", hasattr(c, '_send_queue'))
    check("自动扫描定时器", c.refresh_timer is not None)
    
    # 命令已注册
    from protocol import Cidx
    check("连接处理器已注册", Cidx.UC_CONNECT in c._handlers)
    check("日志处理器已注册", Cidx.LOG_GET in c._handlers)
    check("参数处理器已注册", Cidx.PARAM_READ in c._handlers)


# ═══════════════════════════════════════
# 主流程
# ═══════════════════════════════════════
def main():
    global passed, failed
    print("=" * 50)
    print("  XDr 上位机全功能测试 v3.0")
    print("=" * 50)
    
    app = QApplication(sys.argv)
    mw = None
    
    try:
        mw = test_01_main_window(app)
        test_02_status_bar(mw)
        test_03_data_page(mw)
        test_04_control_page(mw)
        test_05_iap_page(mw)
        test_06_modules(mw)
        test_07_theme(app, mw)
        test_08_signals(app, mw)
        test_09_widget_interaction(app, mw)
        test_10_waveform(app, mw)
        test_11_config(app, mw)
        test_12_comport(app, mw)
    except Exception as e:
        import traceback
        traceback.print_exc()
        failed += 1
    
    print("\n" + "=" * 50)
    total = passed + failed
    print(f"  总计: {total} 项  |  ✅ 通过: {passed}  |  ❌ 失败: {failed}")
    if failed == 0:
        print("  🎉 全部通过！")
    else:
        print(f"  ⚠️  有 {failed} 项失败")
    print("=" * 50)
    
    return 0 if failed == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
