#!/usr/bin/env python3
"""
XDr 统一构建脚本
用法: python3 tools/build.py [gen|build|flash|bf|erase|clean]
"""
import json
import os
import re
import shutil
import subprocess
import sys
from datetime import datetime
from pathlib import Path

# ── 路径常量 ──
USR_DIR = Path(__file__).resolve().parent.parent
TOOLS_DIR = USR_DIR / "tools"
CONFIG_PATH = USR_DIR / "project_config.json"
FOC_JSON_PATH = USR_DIR / "usr_config.json"
APP_CONFIG_PATH = USR_DIR / "usr_config.h"
CMAKE_DIR = USR_DIR / "cmake"
BUILD_DIR = USR_DIR / "build"
FIRMWARE_OUT = USR_DIR.parent / "firmware_out"

def load_config():
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
        cfg = json.load(f)
    with open(FOC_JSON_PATH, "r", encoding="utf-8") as f:
        foc = json.load(f)
    cfg.update(foc)
    return cfg

def get_bsp_path(cfg):
    return USR_DIR.parent / "bsp" / cfg["bsp"]

def parse_bsp_config(bsp_path):
    """从 bsp/config.h 解析产品信息和硬件参数"""
    config_h = bsp_path / "config.h"
    text = config_h.read_text(encoding="utf-8")

    def extract_str(key):
        m = re.search(rf'#define\s+{key}\s+"([^"]*)"', text)
        return m.group(1) if m else ""

    def extract_num(key):
        m = re.search(rf'#define\s+{key}\s+(\d+)', text)
        return int(m.group(1)) if m else 0

    def extract_hex(key):
        m = re.search(rf'#define\s+{key}\s+(0x[0-9a-fA-F]+)U?', text)
        return int(m.group(1), 16) if m else 0

    def extract_float(key):
        m = re.search(rf'#define\s+{key}\s+([\d.]+)f?', text)
        return float(m.group(1)) if m else 0.0

    return {
        "F_PWM":            extract_float("F_PWM"),
        "T_PWM":            extract_float("T_PWM"),
        "TIC_PWM":          extract_num("TIC_PWM"),
        "T_CON":            extract_float("T_CON"),
        "PROD_SERIES":      extract_str("PROD_SERIES"),
        "FUN_V":            extract_str("FUN_V"),
        "FIRM_V":           extract_str("FIRM_V"),
        "MAX_CURRENT":      extract_num("MAX_CURRENT"),
        "MAX_VOLTAGE":      extract_num("MAX_VOLTAGE"),
        "MIN_VOLTAGE":      extract_num("MIN_VOLTAGE"),
        "MAX_TEMPERATURE":  extract_num("MAX_TEMPERATURE"),
        "T_SAMPLE_us":      extract_float("T_SAMPLE_us"),
        "T_DEATH_us":       extract_float("T_DEATH_us"),
        "T_NOISE_us":       extract_float("T_NOISE_us"),
        "BL_START_ADDR":    extract_hex("BL_START_ADDR"),
        "BL_SIZE_KB":       extract_num("BL_SIZE_KB"),
        "APP_START_ADDR":   extract_hex("APP_START_ADDR"),
        "FLASH_END_ADDR":   extract_hex("FLASH_END_ADDR"),
    }

def compute_filter_coeffs(cfg):
    """计算 Butterworth 滤波器系数"""
    try:
        from scipy import signal
    except ImportError:
        print("错误: 需要 scipy 库。请运行: pip install scipy numpy")
        sys.exit(1)

    # PWM 频率默认 20000
    FS = 20000
    ORDER = 2

    filt = cfg["filter"]

    # 速度 LPF
    fc_speed = filt["pll_bandwidth_hz"] * filt["speed_lpf_factor"]
    sos_w = signal.butter(ORDER, fc_speed, btype="low", output="sos", fs=FS)
    b0, b1, b2, a0, a1, a2 = sos_w[0]
    lpf_w = [b0/a0, b1/a0, b2/a0, -a1/a0, -a2/a0]

    # 电流采样 LPF
    fc_curr = filt["curr_bw_target_hz"] * filt["curr_filter_factor"]
    sos_i = signal.butter(ORDER, fc_curr, btype="low", output="sos", fs=FS)
    b0, b1, b2, a0, a1, a2 = sos_i[0]
    lpf_i = [b0/a0, b1/a0, b2/a0, -a1/a0, -a2/a0]

    return {
        "lpf_w": lpf_w,
        "lpf_i": lpf_i,
        "fc_speed": fc_speed,
        "fc_curr": fc_curr,
        "FS": FS,
    }

def fmt_f(val):
    """格式化浮点数，去掉多余尾零"""
    s = f"{val:.10f}"
    s = s.rstrip("0").rstrip(".")
    if "." not in s:
        s += ".0"
    return s + "f"

def gen_defines(items):
    lines = []
    for item in items:
        name = item["name"]
        val = item["value"]
        comment = item.get("comment", "")
        if comment:
            # 使用两个空格加注释对齐（可选）
            lines.append(f"#define {name:<24s} {val}    /* {comment} */")
        else:
            lines.append(f"#define {name:<24s} {val}")
    return "\n".join(lines) + "\n"

def gen_enum_members(members):
    """从 [{name, comment}] 生成枚举成员行"""
    lines = []
    for i, m in enumerate(members):
        name = m["name"]
        comment = m.get("comment", "")
        comma = "," if i < len(members) - 1 else ""
        if comment:
            lines.append(f"    {name}{comma}    /* {comment} */")
        else:
            lines.append(f"    {name}{comma}")
    return "\n".join(lines)

# ── 生成 usr_config.h ──
def gen_usr_config(cfg, bsp_info, coeffs):
    date_str = datetime.now().strftime("%y%m%d")
    firm_name = f"{cfg['prod_name']}-{bsp_info['PROD_SERIES']}"
    fun_v = bsp_info["FUN_V"]
    firm_v = bsp_info["FIRM_V"]
    author = cfg["author"]

    ctrl = cfg["control"]
    lpf_w = coeffs["lpf_w"]
    lpf_i = coeffs["lpf_i"]
    fc_speed = coeffs["fc_speed"]
    fc_curr = coeffs["fc_curr"]
    FS = coeffs["FS"]
    proto = cfg["protocol"]
    hw = bsp_info

    # Pre-build protocol sections (avoid f-string brace escaping issues)
    cmd_ids_section = gen_defines(proto["cmd_ids"])
    feedback_section = gen_defines(proto["feedback_ids"])
    usb_section = gen_defines([{"name": k, **v} for k, v in proto["usb_format"].items()])
    uart_section = gen_defines([{"name": k, **v} for k, v in proto["uart_format"].items()])
    frame_section = gen_defines([{"name": k, **v} for k, v in proto["frame_length"].items()])
    enum_members = gen_enum_members(proto["data_stream"])
    enum_section = "typedef enum {\n" + enum_members + "\n} DataStreamId;"

    # Vector table offset: 0x0 in bootloader mode, APP_START_ADDR - BL_START_ADDR otherwise
    if cfg.get("firmware_type", "app") == "bl":
        vect_offset = 0x0
    else:
        vect_offset = bsp_info["APP_START_ADDR"] - bsp_info["BL_START_ADDR"]

    content = f"""\
/* ===== 此文件由 build.py 自动生成，请勿手动修改 ===== */
/* 生成时间: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")} */
#ifndef __USR_CONFIG_H
#define __USR_CONFIG_H


#include "bsp_interface.h"

/* ---------- 版本信息 ---------- */
#define FIRM_NAME      "{firm_name}"
#define FIRM_AUTHOR    "{author}"
#define FIRM_V_DATE    "{fun_v}_{firm_v}_{date_str}"
#define FIRM_VERSION   FIRM_NAME " " FIRM_V_DATE

/* ---------- 启动配置 ---------- */
#define VECT_TABLE_OFFSET  0x{vect_offset:X}U  /* 向量表偏移 */

/* ---------- 控制参数 ---------- */
#define F_PWM               {fmt_f(hw['F_PWM'])}
#define T_PWM               {fmt_f(hw['T_PWM'])}
#define TIC_PWM             {hw['TIC_PWM']}
#define T_CON               {fmt_f(hw['T_CON'])}

#define FREQ_CURRENT        {ctrl['f_freq_current']}
#define FREQ_SPEED          {ctrl['f_freq_speed']}
#define FREQ_POSTION        {ctrl['f_freq_position']}

#define F_CURRENT           {fmt_f(hw['F_PWM'] / ctrl['f_freq_current'])}
#define F_SPEED             {fmt_f((hw['F_PWM'] / ctrl['f_freq_current']) / ctrl['f_freq_speed'])}
#define F_POSITION          {fmt_f(((hw['F_PWM'] / ctrl['f_freq_current']) / ctrl['f_freq_speed']) / ctrl['f_freq_position'])}

#define T_DATA_STREAM       {ctrl['t_data_stream_ms']}
#define T_STATE_STREAM      {ctrl['t_state_stream_ms']}
#define TEMP_VBUS_TS_MS     {ctrl['temp_vbus_ts_ms']}

/* ---------- 硬件限制参数 (来自 BSP) ---------- */
#define MAX_CURRENT         {hw['MAX_CURRENT']}       /* MOS管最大电流 (A) */
#define MAX_VOLTAGE         {hw['MAX_VOLTAGE']}       /* 最大电压 (V) */
#define MIN_VOLTAGE         {hw['MIN_VOLTAGE']}       /* 最小电压 (V) */
#define MAX_TEMPERATURE     {hw['MAX_TEMPERATURE']}   /* 最大工作温度 (°C) */
#define T_SAMPLE_us         {fmt_f(hw['T_SAMPLE_us'])}  /* 采样时间 (us) */
#define T_DEATH_us          {fmt_f(hw['T_DEATH_us'])}   /* 死区时间 (us) */
#define T_NOISE_us          {fmt_f(hw['T_NOISE_us'])}   /* 开关噪声时间 (us) */

/* ---------- 巴特沃斯滤波器系数 (CMSIS-DSP, [b0, b1, b2, a1, a2]) ---------- */
/* 速度 LPF — 2nd Order Butterworth LPF, fc={fc_speed:.1f}Hz, fs={FS}Hz */
#define LPF_W_B0    {fmt_f(lpf_w[0])}
#define LPF_W_B1    {fmt_f(lpf_w[1])}
#define LPF_W_B2    {fmt_f(lpf_w[2])}
#define LPF_W_A1    {fmt_f(lpf_w[3])}
#define LPF_W_A2    {fmt_f(lpf_w[4])}

/* 电流采样 LPF — 2nd Order Butterworth LPF, fc={fc_curr:.1f}Hz, fs={FS}Hz */
#define LPF_I_B0    {fmt_f(lpf_i[0])}
#define LPF_I_B1    {fmt_f(lpf_i[1])}
#define LPF_I_B2    {fmt_f(lpf_i[2])}
#define LPF_I_A1    {fmt_f(lpf_i[3])}
#define LPF_I_A2    {fmt_f(lpf_i[4])}

/* ========== 通讯协议 (固件/上位机共用) ========== */

/* ---------- CMD ID ---------- */
{cmd_ids_section}

/* ---------- 反馈 ID ---------- */
{feedback_section}

/* ---------- USB 协议格式 ---------- */
{usb_section}

/* ---------- UART 协议格式 ---------- */
{uart_section}

/* ---------- CAN 协议 ---------- */
/* 默认唯一扩展帧 ID, 频率 1M */

/* ---------- 帧长度 ---------- */
{frame_section}

/* ---------- 数据流 ID ---------- */
{enum_section}

#endif /* __USR_CONFIG_H */
"""
    APP_CONFIG_PATH.write_text(content, encoding="utf-8")
    print(f"[OK] 生成 usr_config.h")
    print(f"     FIRM_NAME:   {firm_name}")
    print(f"     AUTHOR:      {author}")
    print(f"     FIRM_V_DATE: {fun_v}_{firm_v}_{date_str}")

# ── gen 命令 ──
def cmd_gen(cfg):
    bsp_path = get_bsp_path(cfg)
    if not bsp_path.exists():
        print(f"错误: BSP 路径不存在: {bsp_path}")
        sys.exit(1)

    bsp_info = parse_bsp_config(bsp_path)
    coeffs = compute_filter_coeffs(cfg)
    gen_usr_config(cfg, bsp_info, coeffs)


# ── 固件输出 ──
def copy_firmware_output(firm_version):
    """生成 .bin/.hex 并复制到 firmware_out/"""
    FIRMWARE_OUT.mkdir(parents=True, exist_ok=True)

    elf = BUILD_DIR / "firmware.elf"
    if not elf.exists():
        return

    # 文件名中空格替换为下划线
    safe_name = firm_version.replace(" ", "_")

    bin_dst = FIRMWARE_OUT / f"{safe_name}.bin"
    hex_dst = FIRMWARE_OUT / f"{safe_name}.hex"

    subprocess.run(["arm-none-eabi-objcopy", "-O", "binary", str(elf), str(bin_dst)], check=True)
    subprocess.run(["arm-none-eabi-objcopy", "-O", "ihex", str(elf), str(hex_dst)], check=True)

    print(f"[OK] 固件输出: {bin_dst}")
    print(f"[OK] 固件输出: {hex_dst}")

# ── build 命令 ──
def cmd_build(cfg):
    cmd_gen(cfg)

    # 清理 CMake 缓存（可选）
    if BUILD_DIR.exists():
        shutil.rmtree(BUILD_DIR)

    bsp_rel = cfg["bsp"]
    build_type = cfg["build_type"]
    firmware_type = cfg.get("firmware_type", "app")
    toolchain = CMAKE_DIR / "arm-gcc.cmake"

    cmake_args = [
        "cmake",
        f"-B{BUILD_DIR}",
        f"-S{USR_DIR}",
        f"-DBSP={bsp_rel}",
        f"-DFIRMWARE_TYPE={firmware_type}",
        f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
        f"-DCMAKE_BUILD_TYPE={build_type}",
    ]
    
    print(f"\n>>> cmake configure")
    r = subprocess.run(cmake_args)
    if r.returncode != 0:
        sys.exit(r.returncode)

    print(f"\n>>> cmake build (Ctrl+C to stop)")
    proc = subprocess.Popen(
        ["cmake", "--build", str(BUILD_DIR), "--", "-k"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
        universal_newlines=True
    )

    # 匹配进度行（支持 Make 的 [ 12%] 和 Ninja 的 [12/456]）
    # 同时排除明显不是进度的行（例如包含 “Built target” 等）
    progress_pattern = re.compile(r'^\s*\[\s*\d+[%/]\d*\]\s+(Building|Linking)')
    error_pattern = re.compile(r'(error:|undefined reference|multiple definition|ld returned \d+ exit status)', re.IGNORECASE)
    warning_pattern = re.compile(r'warning:', re.IGNORECASE)

    error_count = 0
    warning_count = 0
    last_was_progress = False

    # 辅助函数：清除ANSI转义序列
    def clean_ansi(line):
        return re.sub(r'\x1b\[[0-9;]*m', '', line)

    for raw_line in proc.stdout:
        line = raw_line.rstrip('\n\r')
        if not line:
            continue

        # 去除ANSI转义码，用于模式匹配（但原始行保留用于输出）
        plain_line = clean_ansi(line)

        # 判断是否为进度行
        is_progress = bool(progress_pattern.match(plain_line))
        # 检查是否包含错误/警告
        has_error = bool(error_pattern.search(plain_line))
        has_warning = bool(warning_pattern.search(plain_line))

        if has_error:
            error_count += 1
        if has_warning:
            warning_count += 1

        # 输出策略
        if is_progress and not has_error and not has_warning:
            # 正常的进度行：覆盖当前行
            print(f'\r{line}', end='', flush=True)
            last_was_progress = True
        else:
            # 非进度行或包含错误/警告
            if last_was_progress:
                # 上次是覆盖行，需要先换行，使新内容从新行开始
                print()  # 换行
                last_was_progress = False
            # 着色输出
            if has_error:
                print(f'\033[31m{line}\033[0m')
            elif has_warning:
                print(f'\033[33m{line}\033[0m')
            else:
                print(line)

    proc.wait()
    # 最后如果还在覆盖模式，补一个换行
    if last_was_progress:
        print()

    # 输出统计信息
    print(f"\n========== 编译统计 ==========")
    if error_count > 0:
        print(f"\033[31m错误数量: {error_count}\033[0m")
    else:
        print(f"错误数量: 0")
    if warning_count > 0:
        print(f"\033[33m警告数量: {warning_count}\033[0m")
    else:
        print(f"警告数量: 0")
    print(f"===============================\n")

    if proc.returncode != 0:
        print(f"[错误] 编译失败（返回码 {proc.returncode}）")
        sys.exit(proc.returncode)
    else:
        print("[OK] 编译成功")

    # 复制固件
    bsp_info = parse_bsp_config(get_bsp_path(cfg))
    date_str = datetime.now().strftime("%y%m%d")
    firm_version = f"{cfg['prod_name']}-{bsp_info['PROD_SERIES']}_{bsp_info['FUN_V']}_{bsp_info['FIRM_V']}_{date_str}"
    copy_firmware_output(firm_version)

# ── flash 命令 ──
def check_debugger(cfg):
    # 从配置中读取调试器类型，默认回退到 stlink
    dbg_type = cfg.get("debugger_type", "stlink")
    if dbg_type not in cfg["debugger"]:
        print(f"错误: 配置中未找到调试器类型 '{dbg_type}'，可用的有: {list(cfg['debugger'].keys())}")
        sys.exit(1)

    dbg = cfg["debugger"][dbg_type]
    interface = dbg["interface"]
    target = dbg["target"]

    print(f"检查 {dbg_type} 仿真器连接...")
    cmd = ["openocd", "-f", interface, "-f", target, "-c", "init; exit"]
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
        if r.returncode != 0:
            print(f"错误: 无法连接到 {dbg_type} 仿真器\n{r.stderr}")
            sys.exit(1)
        print(f"{dbg_type} 仿真器连接正常")
    except FileNotFoundError:
        print("错误: 未找到 openocd")
        sys.exit(1)
    except subprocess.TimeoutExpired:
        print(f"错误: {dbg_type} openocd 连接超时")
        sys.exit(1)

    return interface, target

def cmd_flash(cfg):
    interface, target = check_debugger(cfg)
    elf = BUILD_DIR / "firmware.elf"
    if not elf.exists():
        print(f"错误: ELF 文件不存在: {elf}，请先编译")
        sys.exit(1)

    print("烧录中... (使用 ELF 内部地址)")
    cmd = [
        "openocd", "-f", interface, "-f", target,
        "-c", f"program {elf} verify reset exit"
    ]
    r = subprocess.run(cmd)
    if r.returncode != 0:
        print("烧录失败!")
        sys.exit(1)
    print("[OK] 烧录完成")

def cmd_bf(cfg):
    cmd_build(cfg)
    cmd_flash(cfg)

def cmd_erase(cfg):
    interface, target = check_debugger(cfg)
    print("擦除芯片...")
    cmd = ["openocd", "-f", interface, "-f", target,
           "-c", "init; reset halt; flash erase_sector 0 0 last; exit"]
    subprocess.run(cmd)

def cmd_clean():
    if BUILD_DIR.exists():
        shutil.rmtree(BUILD_DIR)
        print(f"[OK] 已清除 {BUILD_DIR}")
    else:
        print("build/ 目录不存在，无需清理")

# ── 主入口 ──
def main():
    if len(sys.argv) < 2:
        print("用法: python3 tools/build.py [gen|build|flash|bf|erase|clean]")
        print()
        print("  gen    生成 usr_config.h")
        print("  build  gen + cmake + 编译 + 输出 bin/hex (根据 project_config.json 中的 firmware_type 切换 BL/APP)")
        print("  flash  烧录固件")
        print("  bf     build + flash")
        print("  erase  擦除芯片")
        print("  clean  清除 build/")
        sys.exit(1)

    action = sys.argv[1]
    cfg = load_config()

    actions = {
        "gen":    lambda: cmd_gen(cfg),
        "build":  lambda: cmd_build(cfg),
        "flash":  lambda: cmd_flash(cfg),
        "bf":     lambda: cmd_bf(cfg),
        "erase":  lambda: cmd_erase(cfg),
        "clean":  cmd_clean,
    }

    if action in actions:
        actions[action]()
    else:
        print(f"未知操作: {action}")
        sys.exit(1)

if __name__ == "__main__":
    main()
