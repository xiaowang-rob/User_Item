"""构建核心逻辑：生成 usr_config.h、CMake 编译"""

import re
import shutil
import subprocess
import sys
from datetime import datetime


# 导入同目录下的工具模块
import config_parser
import filter_coeffs
import linker_mod


# ── 辅助函数 ──
def fmt_f(val):
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
            lines.append(f"#define {name:<24s} {val}    /* {comment} */")
        else:
            lines.append(f"#define {name:<24s} {val}")
    return "\n".join(lines) + "\n"


def gen_enum_members(members):
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


def gen_usr_config(cfg, bsp_info, coeffs, output_path):
    date_str = datetime.now().strftime("%y%m%d")
    firm_name = f"{cfg['prod_name']}-{bsp_info['PROD_SERIES']}"
    fun_v = bsp_info["FUN_V"]
    firm_v = bsp_info["FIRM_V"]
    author = cfg["author"]

    ctrl = cfg["control"]
    lpf_w = coeffs["lpf_w"]
    fc_speed = coeffs["fc_speed"]
    FS = coeffs["FS"]
    hw = bsp_info

    if bsp_info["FIRMWARE_TYPE"] == "BL":
        vect_offset = 0x0
    else:
        vect_offset = bsp_info["APP_START_ADDR"] - bsp_info["BL_START_ADDR"]

    content = f"""\
/* ===== 此文件由 build.py 自动生成，请勿手动修改，相关配置在 usr_config.json 中 ===== */
/* 生成时间: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")} */
#ifndef __USR_CONFIG_H
#define __USR_CONFIG_H


/* ---------- 版本信息 ---------- */
#define FIRM_NAME      "{firm_name}"
#define FIRM_AUTHOR    "{author}"
#define FIRM_V_DATE    "{fun_v}_{firm_v}_{date_str}"
#define FIRM_VERSION   FIRM_NAME " " FIRM_V_DATE

/* ---------- 启动配置 ---------- */
#define VECT_TABLE_OFFSET  0x{vect_offset:X}U

/* ---------- 控制参数 ---------- */
#define F_PWM               {fmt_f(hw["F_PWM"])}
#define T_PWM               {fmt_f(hw["T_PWM"])}
#define TIC_PWM             {hw["TIC_PWM"]}
#define T_CON               {fmt_f(hw["T_CON"])}

#define FREQ_CURRENT        {ctrl["f_freq_current"]}
#define FREQ_SPEED          {ctrl["f_freq_speed"]}
#define FREQ_POSTION        {ctrl["f_freq_position"]}

#define F_CURRENT           {fmt_f(hw["F_PWM"] / ctrl["f_freq_current"])}
#define F_SPEED             {fmt_f((hw["F_PWM"] / ctrl["f_freq_current"]) / ctrl["f_freq_speed"])}
#define F_POSITION          {fmt_f(((hw["F_PWM"] / ctrl["f_freq_current"]) / ctrl["f_freq_speed"]) / ctrl["f_freq_position"])}

#define T_DATA_STREAM       {ctrl["t_data_stream_ms"]}
#define T_STATE_STREAM      {ctrl["t_state_stream_ms"]}
#define TEMP_VBUS_TS_MS     {ctrl["temp_vbus_ts_ms"]}

/* ---------- 硬件限制参数 (来自 BSP) ---------- */
#define MAX_CURRENT         {hw["MAX_CURRENT"]}
#define MAX_VOLTAGE         {hw["MAX_VOLTAGE"]}
#define MIN_VOLTAGE         {hw["MIN_VOLTAGE"]}
#define MAX_TEMPERATURE     {hw["MAX_TEMPERATURE"]}
#define T_SAMPLE_us         {fmt_f(hw["T_SAMPLE_us"])}
#define T_DEATH_us          {fmt_f(hw["T_DEATH_us"])}
#define T_NOISE_us          {fmt_f(hw["T_NOISE_us"])}

/* ---------- 速度 LPF 滤波器系数 (Butterworth) ---------- */
/* 速度 LPF — 2nd Order Butterworth LPF, fc={fc_speed:.1f}Hz, fs={FS}Hz */
#define LPF_W_B0    {fmt_f(lpf_w[0])}
#define LPF_W_B1    {fmt_f(lpf_w[1])}
#define LPF_W_B2    {fmt_f(lpf_w[2])}
#define LPF_W_A1    {fmt_f(lpf_w[3])}
#define LPF_W_A2    {fmt_f(lpf_w[4])}

#endif /* __USR_CONFIG_H */
"""
    output_path.write_text(content, encoding="utf-8")
    print(f"[OK] 生成 usr_config.h")


# ── 主要命令 ──
def get_cmake_project_name(project_root):
    """从顶层 CMakeLists.txt 解析 CMAKE_PROJECT_NAME"""
    cmake_file = project_root / "CMakeLists.txt"
    if not cmake_file.exists():
        sys.exit("错误: 找不到顶层 CMakeLists.txt")
    text = cmake_file.read_text(encoding="utf-8")
    m = re.search(r"set\s*\(\s*CMAKE_PROJECT_NAME\s+(\w+)\s*\)", text)
    if not m:
        sys.exit("错误: 在 CMakeLists.txt 中未找到 CMAKE_PROJECT_NAME")
    return m.group(1)


def cmd_gen(cfg, project_root):
    config_h = project_root / "bsp" / "config.h"
    bsp_info = config_parser.parse_bsp_config(config_h)
    coeffs = filter_coeffs.compute_filter_coeffs(cfg)

    usr_config_h = project_root / "usr" / "usr_config.h"
    gen_usr_config(cfg, bsp_info, coeffs, usr_config_h)

    ld_file = project_root / "STM32F405XX_FLASH.ld"
    if ld_file.exists():
        linker_mod.modify_linker_script(config_h, ld_file)

    # 生成协议定义文件
    codegen_script = project_root / "tools" / "codegen.py"
    json_path = project_root / "usr" / "usr_config.json"
    proto_h = project_root / "usr" / "protocol_defs.h"
    proto_py = project_root / "usr" / "shared_constants.py"
    subprocess.run(
        [
            sys.executable,
            str(codegen_script),
            str(json_path),
            str(proto_h),
            str(proto_py),
        ],
        check=True,
    )
    print("[OK] 协议定义文件已生成")


def check_and_clean_if_type_changed(project_root, firmware_type):
    """如果编译类型变化，清除 build 目录"""
    build_dir = project_root / "build"
    marker = build_dir / ".last_firmware_type"

    if build_dir.exists():
        if marker.exists():
            last_type = marker.read_text(encoding="utf-8").strip()
            if last_type != firmware_type:
                print(
                    f"检测到编译类型变化 ({last_type} -> {firmware_type})，清理 build 目录..."
                )
                shutil.rmtree(build_dir)
                build_dir.mkdir(parents=True)
        else:
            print("未找到编译类型记录，清理 build 目录以确保一致性...")
            shutil.rmtree(build_dir)
            build_dir.mkdir(parents=True)
    else:
        build_dir.mkdir(parents=True)

    marker.write_text(firmware_type, encoding="utf-8")


def cmd_build(cfg, project_root):
    """完整编译流程：gen + cmake"""
    # 先生成 usr_config.h 和 ld
    cmd_gen(cfg, project_root)

    # 重新读取 bsp 获取 firmware_type
    bsp_info = config_parser.parse_bsp_config(project_root / "bsp" / "config.h")
    firmware_type = bsp_info["FIRMWARE_TYPE"]

    # 检查类型变化，必要时清理
    check_and_clean_if_type_changed(project_root, firmware_type)

    build_dir = project_root / "build"
    # 使用实际存在的工具链文件
    toolchain = project_root / "cmake" / "gcc-arm-none-eabi.cmake"
    build_type = cfg.get("build_type", "Debug")

    cmake_cache = build_dir / "CMakeCache.txt"
    if not cmake_cache.exists():
        print("运行 cmake 配置...")
        cmake_args = [
            "cmake",
            f"-B{build_dir}",
            f"-S{project_root}",
            f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
            f"-DCMAKE_BUILD_TYPE={build_type}",
        ]
        r = subprocess.run(cmake_args)
        if r.returncode != 0:
            sys.exit(r.returncode)

    print(f"\n>>> cmake build (类型: {firmware_type}, Ctrl+C to stop)")
    proc = subprocess.Popen(
        ["cmake", "--build", str(build_dir), "--", "-k"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
        universal_newlines=True,
    )

    progress_pattern = re.compile(r"^\s*\[\s*\d+[%/]\d*\]\s+(Building|Linking)")
    error_pattern = re.compile(
        r"(error:|undefined reference|multiple definition|ld returned \d+ exit status)",
        re.IGNORECASE,
    )
    warning_pattern = re.compile(r"warning:", re.IGNORECASE)

    error_count = 0
    warning_count = 0
    last_was_progress = False

    def clean_ansi(line):
        return re.sub(r"\x1b\[[0-9;]*m", "", line)

    for raw_line in proc.stdout:
        line = raw_line.rstrip("\n\r")
        if not line:
            continue

        plain_line = clean_ansi(line)
        is_progress = bool(progress_pattern.match(plain_line))
        has_error = bool(error_pattern.search(plain_line))
        has_warning = bool(warning_pattern.search(plain_line))

        if has_error:
            error_count += 1
        if has_warning:
            warning_count += 1

        if is_progress and not has_error and not has_warning:
            print(f"\r{line}", end="", flush=True)
            last_was_progress = True
        else:
            if last_was_progress:
                print()
                last_was_progress = False
            if has_error:
                print(f"\033[31m{line}\033[0m")
            elif has_warning:
                print(f"\033[33m{line}\033[0m")
            else:
                print(line)

    proc.wait()
    if last_was_progress:
        print()

    print(f"\n========== 编译统计 ==========")
    if error_count > 0:
        print(f"\033[31m错误数量: {error_count}\033[0m")
    else:
        print("错误数量: 0")
    if warning_count > 0:
        print(f"\033[33m警告数量: {warning_count}\033[0m")
    else:
        print("警告数量: 0")
    print("===============================\n")

    if proc.returncode != 0:
        print(f"[错误] 编译失败（返回码 {proc.returncode}）")
        sys.exit(proc.returncode)

    # 编译成功，生成 bin/hex 输出
    project_name = get_cmake_project_name(project_root)
    elf = build_dir / f"{project_name}.elf"
    if elf.exists():
        try:
            # 输出固件大小统计
            result = subprocess.run(
                ["arm-none-eabi-size", str(elf)],
                capture_output=True,
                text=True,
                check=True,
            )
            print("\n========== 固件大小 ==========")
            print(result.stdout)
            print("===============================")

            # 拷贝 bin/hex
            firm_name = f"{cfg['prod_name']}-{bsp_info['PROD_SERIES']}"
            fun_v = bsp_info["FUN_V"]
            firm_v = bsp_info["FIRM_V"]
            print(f"firm_version: {firm_v}")
            date_str = datetime.now().strftime("%y%m%d")
            dist_name = f"{firm_name} {fun_v}_{firm_v}_{date_str}"

            firmware_out = project_root.parent.parent.parent / "firmware_out"
            firmware_out.mkdir(parents=True, exist_ok=True)
            bin_dst = firmware_out / f"{dist_name}.bin"
            hex_dst = firmware_out / f"{dist_name}.hex"

            subprocess.run(
                ["arm-none-eabi-objcopy", "-O", "binary", str(elf), str(bin_dst)],
                check=True,
            )
            subprocess.run(
                ["arm-none-eabi-objcopy", "-O", "ihex", str(elf), str(hex_dst)],
                check=True,
            )
            print(f"[OK] 固件输出: {bin_dst}")
            print(f"[OK] 固件输出: {hex_dst}")
        except subprocess.CalledProcessError:
            print("[警告] 无法生成 bin/hex 或获取大小信息")
