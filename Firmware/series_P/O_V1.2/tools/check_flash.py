#!/usr/bin/env python3
import sys
import subprocess
from pathlib import Path

ELF_PATH = Path(__file__).resolve().parent.parent / "build" / "firmware.elf"
# 可选：保存已烧录文件的 hash 或时间戳，本文简单判断 ELF 是否存在且芯片是否已经有程序
# 更严谨可以读取芯片 ID 和 ELF 的 Flash 地址比对，简单起见先判断 ELF 是否存在

def is_flash_matched():
    # 实际可以读取芯片某固定地址的标志，这里返回 False 强制先烧录一次
    # 假设烧录后创建一个 .flash_success 文件，存在则跳过
    marker = Path("/tmp/xdr_flash_done")
    return marker.exists() and ELF_PATH.exists()

def main():
    if len(sys.argv) > 1 and sys.argv[1] == "skip":
        print("Skip flashing by argument")
        return

    if not ELF_PATH.exists():
        print("ELF not found, need to build first.")
        sys.exit(1)

    if is_flash_matched():
        print("Flash already up-to-date, skipping...")
        return

    print("Flashing firmware...")
    # 调用 build.py flash
    subprocess.run([sys.executable, "tools/build.py", "flash"], cwd=Path(__file__).parent.parent, check=True)
    # 烧录成功后创建标记文件
    Path("/tmp/xdr_flash_done").touch()
    print("Flashing done.")

if __name__ == "__main__":
    main()