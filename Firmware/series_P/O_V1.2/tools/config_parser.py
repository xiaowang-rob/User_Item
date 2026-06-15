"""解析 bsp/config.h 中的硬件配置及编译类型"""
import re
import sys

def parse_bsp_config(config_h_path):
    """从 config.h 解析所有所需参数，返回字典"""
    if not config_h_path.exists():
        print(f"错误: config.h 不存在: {config_h_path}")
        sys.exit(1)

    text = config_h_path.read_text(encoding="utf-8")

    def extract_str(key):
        m = re.search(rf'#define\s+{key}\s+"?([\w.]+)"?', text)
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
        "T_DEADTIME_us":       extract_float("T_DEADTIME_us"),
        "T_NOISE_us":       extract_float("T_NOISE_us"),
        "BL_START_ADDR":    extract_hex("BL_START_ADDR"),
        "BL_SIZE_KB":       extract_num("BL_SIZE_KB"),
        "APP_START_ADDR":   extract_hex("APP_START_ADDR"),
        "FLASH_END_ADDR":   extract_hex("FLASH_END_ADDR"),
        "FIRMWARE_TYPE":    extract_str("FIRMWARE_TYPE").upper(),  # APP 或 BL
    }