#!/usr/bin/env python3
"""
XDr 统一构建脚本（精简入口）
用法: python3 tools/build.py [gen|build|flash|bf|erase|clean]

特性：
- BSP 路径固定为项目根下的 bsp/
- 编译类型（BL/APP）从 bsp/config.h 读取
- 支持软件复位烧录
"""
import json
import shutil
import sys
from pathlib import Path

# 确保可以导入 tools/ 下的模块
TOOLS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS_DIR))

import build_core
import flash_core

# ── 路径常量 ──
USR_DIR = Path(__file__).resolve().parent.parent          # 项目根目录
CONFIG_PATH = USR_DIR /"usr" / "project_config.json"
FOC_JSON_PATH = USR_DIR / "usr" / "usr_config.json"
BUILD_DIR = USR_DIR / "build"

def load_config():
    """加载 project_config.json 和 usr_config.json"""
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
        cfg = json.load(f)
    with open(FOC_JSON_PATH, "r", encoding="utf-8") as f:
        foc = json.load(f)
    cfg.update(foc)
    return cfg

def cmd_clean():
    """清除 build 目录"""
    if BUILD_DIR.exists():
        shutil.rmtree(BUILD_DIR)
        print(f"[OK] 已清除 {BUILD_DIR}")
    else:
        print("build/ 目录不存在，无需清理")

def main():
    if len(sys.argv) < 2:
        print("用法: python3 tools/build.py [gen|build|flash|bf|erase|clean]")
        print()
        print("  gen    生成 usr_config.h 并修改链接脚本")
        print("  build  编译工程 (gen + cmake + 输出固件)")
        print("  flash  烧录固件")
        print("  bf     build + flash")
        print("  erase  擦除芯片")
        print("  clean  清除 build/")
        sys.exit(1)

    action = sys.argv[1]
    cfg = load_config()

    if action == "gen":
        build_core.cmd_gen(cfg, USR_DIR)
    elif action == "build":
        build_core.cmd_build(cfg, USR_DIR)
    elif action == "flash":
        flash_core.cmd_flash(cfg, USR_DIR)
    elif action == "bf":
        build_core.cmd_build(cfg, USR_DIR)
        flash_core.cmd_flash(cfg, USR_DIR)
    elif action == "erase":
        flash_core.cmd_erase(cfg, USR_DIR)
    elif action == "clean":
        cmd_clean()
    else:
        print(f"未知操作: {action}")
        sys.exit(1)

if __name__ == "__main__":
    main()