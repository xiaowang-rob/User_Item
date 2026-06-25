#!/usr/bin/env python3
"""
XDr 统一构建脚本
用法: python3 tools/build.py [gen|build|flash|bf|erase|clean]
"""
import json
import shutil
import sys
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS_DIR))

import build_core
import flash_core

PROJECT_ROOT = TOOLS_DIR.parent
CONFIG_PATH = PROJECT_ROOT / "usr" / "project_config.json"
BUILD_DIR = PROJECT_ROOT / "build"

COMMANDS = {
    "gen":   "生成 usr_config.h 并修改链接脚本",
    "build": "编译工程 (gen + cmake + 输出固件)",
    "flash": "烧录固件",
    "bf":    "build + flash",
    "erase": "擦除芯片",
    "clean": "清除 build/",
}


def load_config():
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
        return json.load(f)


def main():
    if len(sys.argv) < 2 or sys.argv[1] not in COMMANDS:
        print(f"用法: python3 tools/build.py [{'|'.join(COMMANDS)}]")
        for cmd, desc in COMMANDS.items():
            print(f"  {cmd:<6s} {desc}")
        sys.exit(1)

    action = sys.argv[1]
    cfg = load_config()

    if action == "gen":
        build_core.cmd_gen(cfg, PROJECT_ROOT)
    elif action == "build":
        build_core.cmd_build(cfg, PROJECT_ROOT)
    elif action == "flash":
        flash_core.cmd_flash(cfg, PROJECT_ROOT)
    elif action == "bf":
        build_core.cmd_build(cfg, PROJECT_ROOT)
        flash_core.cmd_flash(cfg, PROJECT_ROOT)
    elif action == "erase":
        flash_core.cmd_erase(cfg, PROJECT_ROOT)
    elif action == "clean":
        if BUILD_DIR.exists():
            shutil.rmtree(BUILD_DIR)
            print(f"[OK] 已清除 {BUILD_DIR}")
        else:
            print("build/ 目录不存在，无需清理")


if __name__ == "__main__":
    main()
