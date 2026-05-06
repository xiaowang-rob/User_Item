"""烧录与擦除功能（OpenOCD + Cortex-Debug）"""
import subprocess
import sys
import re
from pathlib import Path

def get_cmake_project_name(project_root):
    """从顶层 CMakeLists.txt 解析 CMAKE_PROJECT_NAME"""
    cmake_file = project_root / "CMakeLists.txt"
    if not cmake_file.exists():
        sys.exit("错误: 找不到顶层 CMakeLists.txt")
    text = cmake_file.read_text(encoding="utf-8")
    m = re.search(r'set\s*\(\s*CMAKE_PROJECT_NAME\s+(\w+)\s*\)', text)
    if not m:
        sys.exit("错误: 在 CMakeLists.txt 中未找到 CMAKE_PROJECT_NAME")
    return m.group(1)

def check_debugger(cfg):
    """检查调试器连接并返回 interface 和 target 路径"""
    dbg_type = cfg.get("debugger_type", "stlink")
    debuggers = cfg.get("debugger", {})
    if dbg_type not in debuggers:
        print(f"错误: 未配置调试器 '{dbg_type}'，可用: {list(debuggers.keys())}")
        sys.exit(1)

    dbg = debuggers[dbg_type]
    interface = dbg["interface"]
    target = dbg["target"]

    print(f"检查 {dbg_type} 仿真器连接...")
    cmd = ["openocd", "-f", interface, "-f", target, "-c", "init; exit"]
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
        if r.returncode != 0:
            print(f"错误: 无法连接到 {dbg_type}\n{r.stderr}")
            sys.exit(1)
        print(f"{dbg_type} 仿真器连接正常")
    except FileNotFoundError:
        print("错误: 未找到 openocd")
        sys.exit(1)
    except subprocess.TimeoutExpired:
        print(f"错误: {dbg_type} openocd 连接超时")
        sys.exit(1)

    return interface, target

def cmd_flash(cfg, project_root):
    """烧录固件"""
    interface, target = check_debugger(cfg)
    project_name = get_cmake_project_name(project_root)
    elf = project_root / "build" / f"{project_name}.elf"
    if not elf.exists():
        print(f"错误: ELF 文件不存在: {elf}，请先编译")
        sys.exit(1)

    # 可选的无 NRST 配置
    no_nrst_cfg = project_root / "openocd_reset.cfg"
    nrst_args = []
    if no_nrst_cfg.exists():
        nrst_args = ["-f", str(no_nrst_cfg)]
        print("已启用软件复位（无硬件 NRST）")

    print("烧录中... (使用 ELF 内部地址)")
    cmd = [
        "openocd", "-f", interface, "-f", target,
        *nrst_args,
        "-c", "init",
        "-c", "reset halt",
        "-c", f"program {elf} verify",
        "-c", "resume",
        "-c", "exit"
    ]
    r = subprocess.run(cmd)
    if r.returncode != 0:
        print("烧录失败!")
        sys.exit(1)
    print("[OK] 烧录完成")

def cmd_erase(cfg, project_root):
    """擦除芯片"""
    interface, target = check_debugger(cfg)
    no_nrst_cfg = project_root / "openocd_reset.cfg"
    nrst_args = []
    if no_nrst_cfg.exists():
        nrst_args = ["-f", str(no_nrst_cfg)]
        print("已启用软件复位（无硬件 NRST）")
    print("擦除芯片...")
    cmd = ["openocd", "-f", interface, "-f", target,
           *nrst_args,
           "-c", "init; reset halt; flash erase_sector 0 0 last; exit"]
    subprocess.run(cmd)