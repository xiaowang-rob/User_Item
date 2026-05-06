"""修改 CubeMX 生成的链接脚本，根据 bsp/config.h 调整 FLASH 起始地址和长度"""
import re
import sys
from pathlib import Path

def modify_linker_script(config_h_path, ld_path):
    """读取 config.h，直接修改 ld_path 中的 FLASH 区域定义"""
    # ── 解析 config.h ──
    text = config_h_path.read_text(encoding="utf-8")

    def extract_hex(key):
        m = re.search(rf'#define\s+{key}\s+(0x[0-9a-fA-F]+)U?', text)
        return int(m.group(1), 16) if m else 0

    def extract_num(key):
        m = re.search(rf'#define\s+{key}\s+(\d+)', text)
        return int(m.group(1)) if m else 0

    def extract_str(key):
        m = re.search(rf'#define\s+{key}\s+"?([\w.]+)"?', text)
        return m.group(1) if m else ""

    fw_type = extract_str("FIRMWARE_TYPE").upper()
    bl_start = extract_hex("BL_START_ADDR")
    bl_size_kb = extract_num("BL_SIZE_KB")
    app_start = extract_hex("APP_START_ADDR")
    flash_end = extract_hex("FLASH_END_ADDR")

    if fw_type == "BL":
        origin = bl_start
        length_kb = bl_size_kb
    else:
        app_size_kb = (flash_end - app_start + 1) // 1024 if flash_end >= app_start else 0
        origin = app_start
        length_kb = app_size_kb

    origin_hex = f"0x{origin:08X}"
    length_str = f"{length_kb}K"

    # ── 读取 ld 文件 ──
    ld_content = ld_path.read_text(encoding="utf-8")

    # ── 定位 MEMORY 块并替换 FLASH 行 ──
    # 匹配整个 MEMORY { ... } 块（支持 /* */ 注释和嵌套大括号）
    memory_pattern = r'(MEMORY\s*\{)(.*?)(\})'
    match = re.search(memory_pattern, ld_content, re.DOTALL | re.IGNORECASE)
    if not match:
        print("错误: 未找到 MEMORY 块，ld 文件无法修改")
        sys.exit(1)

    memory_block = match.group(2)  # MEMORY 内部的区域定义

    # 在 memory_block 中查找并替换 FLASH 行
    flash_pattern = r'(FLASH\s*\([^)]*\)\s*:\s*ORIGIN\s*=\s*)(0x[0-9a-fA-F]+)(\s*,\s*LENGTH\s*=\s*)([0-9]+[Kk]?)'
    new_flash_line, count = re.subn(flash_pattern,
                                    lambda m: f"{m.group(1)}{origin_hex}{m.group(3)}{length_str}",
                                    memory_block, flags=re.IGNORECASE)

    if count == 0:
        print("错误: 在 MEMORY 块中未找到 FLASH 定义行，当前 MEMORY 块内容：")
        print(memory_block)
        sys.exit(1)

    # 重建整个 ld 内容
    new_ld_content = ld_content[:match.start(2)] + new_flash_line + ld_content[match.end(2):]

    # ── 备份原文件 ──
    backup_path = ld_path.with_suffix(".ld.bak")
    backup_path.write_text(ld_content, encoding="utf-8")
    print(f"[备份] 原 ld 已备份至 {backup_path}")

    # ── 写入修改后的内容 ──
    ld_path.write_text(new_ld_content, encoding="utf-8")
    print(f"[OK] 已修改 {ld_path.name}: FLASH ORIGIN={origin_hex}, LENGTH={length_str}  ({fw_type}模式)")