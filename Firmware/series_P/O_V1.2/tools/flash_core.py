"""烧录与擦除功能（OpenOCD）"""
import subprocess
import sys
from pathlib import Path

from utils import get_cmake_project_name


def _check_debugger(cfg):
    """检查调试器连接并返回 (interface, target) 路径"""
    dbg_type = cfg.get("debugger_type", "stlink")
    debuggers = cfg.get("debugger", {})
    if dbg_type not in debuggers:
        sys.exit(f"错误: 未配置调试器 '{dbg_type}'，可用: {list(debuggers.keys())}")

    dbg = debuggers[dbg_type]
    interface, target = dbg["interface"], dbg["target"]

    print(f"检查 {dbg_type} 仿真器连接...")
    try:
        r = subprocess.run(
            ["openocd", "-f", interface, "-f", target, "-c", "init; exit"],
            capture_output=True, text=True, timeout=10,
        )
        if r.returncode != 0:
            sys.exit(f"错误: 无法连接到 {dbg_type}\n{r.stderr}")
    except FileNotFoundError:
        sys.exit("错误: 未找到 openocd")
    except subprocess.TimeoutExpired:
        sys.exit(f"错误: {dbg_type} openocd 连接超时")

    print(f"{dbg_type} 仿真器连接正常")
    return interface, target


def _get_nrst_args(project_root):
    """获取软件复位参数（如果配置了的话）"""
    cfg = project_root / "openocd_reset.cfg"
    if cfg.exists():
        print("已启用软件复位（无硬件 NRST）")
        return ["-f", str(cfg)]
    return []


def cmd_flash(cfg, project_root):
    """烧录固件"""
    interface, target = _check_debugger(cfg)
    elf = project_root / "build" / f"{get_cmake_project_name(project_root)}.elf"
    if not elf.exists():
        sys.exit(f"错误: ELF 文件不存在: {elf}，请先编译")

    nrst_args = _get_nrst_args(project_root)
    print("烧录中... (使用 ELF 内部地址)")
    r = subprocess.run([
        "openocd", "-f", interface, "-f", target, *nrst_args,
        "-c", "init", "-c", "reset halt",
        "-c", f"program {elf} verify",
        "-c", "resume", "-c", "exit",
    ])
    if r.returncode != 0:
        sys.exit("烧录失败!")
    print("[OK] 烧录完成")


def cmd_erase(cfg, project_root):
    """擦除芯片"""
    interface, target = _check_debugger(cfg)
    nrst_args = _get_nrst_args(project_root)
    print("擦除芯片...")
    subprocess.run([
        "openocd", "-f", interface, "-f", target, *nrst_args,
        "-c", "init; reset halt; flash erase_sector 0 0 last; exit",
    ])
