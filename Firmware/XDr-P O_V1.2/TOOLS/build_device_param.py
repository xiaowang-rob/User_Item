#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
驱动参数生成 & Bin 文件归档脚本
工作流：
1. 编译前：生成 drive_parameters.h（含自动日期和预拼接字符串）
2. 编译后：从 Keil 输出目录提取固定名称的 .bin，按版本重命名并归档
"""
import os
import sys
import shutil
from datetime import datetime

# ================= 🔧 用户配置区域 =================
HEADER_PATH = "../FOC/Config/drive_parameters.h"
TARGET_DIR  = "../firmware"      # 归档目标文件夹

PROD_NAME   = "XDr"
PROD_SERIES = "P"
FUN_V       = "O"
FIRM_V      = "V1.2"
AUTHOR      = "wxd"

# 硬件参数 (仅用于生成头文件)
PARAMS = {
    "F_PWM":         (20000,    "20kHz"),
    "FREQ_CURRENT":  (1,        "电流环分频系数"),
    "FREQ_SPEED":    (10,       "速度环分频系数"),
    "FREQ_POSTION":  (10,       "位置环分频系数"),
    "T_PWM":         (0.00005,  "50us", "f"),
    "TIC_PWM":       (2099,     ""),
    "T_CON":         ("T_PWM",  ""),
    "T_SAMPLE_us":   (7,        "采样 4-7us"),
    "T_DEATH_us":    (0.5,      "死区时间", "f"),
    "T_NOISE_us":    (0.5,      "开关噪声时间", "f"),
    "RATE_CURRENT_SAMPLE": (100.0, "电流采样比率", "f"),
    "RATE_VOLTAGE_SAMPLE": (16,   "电压采样比率"),
    "MAX_CURRENT":   (100,   "MOS管最大电流 100A"),
    "MAX_VOLTAGE":   (34,    "最大电压 34V"),
    "MIN_VOLTAGE":   (20,    "最小电压 20V"),
    "MAX_TEMPERATURE": (80,  "最大工作温度"),
    "T_DATA_STREAM":    (1,    "监测数据截取周期 ms"),
    "T_STATE_STREAM":   (500,  "状态数据发送周期 ms"),
    "TEMP_VBUS_TS_MS":  (300,  "温度、电压采样周期 ms"),
}
# ===========================================

def get_date_str():
    return datetime.now().strftime("%y%m%d")

def get_full_version_str():
    return f"{PROD_NAME}-{PROD_SERIES} {FUN_V}_{FIRM_V}_{get_date_str()}"

def format_value(val, suffix=""):
    if isinstance(val, str): return val
    if suffix: return f"{val}{suffix}"
    return str(int(val) if isinstance(val, float) and val.is_integer() else val)

def generate_header():
    date_str = get_date_str()
    date_num = (int(date_str[:2]) * 10000) + (int(date_str[2:4]) * 100) + int(date_str[4:])
    
    f_current=PARAMS["F_PWM"][0]/PARAMS["FREQ_CURRENT"][0]
    f_speed  =f_current/PARAMS["FREQ_SPEED"][0]
    f_pos    =f_speed/PARAMS["FREQ_POSTION"][0]

    drive_message = (
        f'{PROD_NAME}-{PROD_SERIES},'
        f'{FUN_V}_{FIRM_V}_{date_str},'
        f'{AUTHOR},'
        f'{PARAMS["F_PWM"][0]},'
        f'{f_current},'
        f'{f_speed},'
        f'{f_pos},'
        f'{PARAMS["MAX_CURRENT"][0]},'
        f'{PARAMS["MIN_VOLTAGE"][0]}-{PARAMS["MAX_VOLTAGE"][0]},'
        f'{PARAMS["MAX_TEMPERATURE"][0]}'
    )
    firm_version = f"{PROD_NAME}-{PROD_SERIES} {FUN_V}_{FIRM_V}_{date_str}"
    
    param_lines = []
    for key, val_info in PARAMS.items():
        val, comment = val_info[:2]
        suffix = val_info[2] if len(val_info) > 2 else ""
        val_str = format_value(val, suffix)
        comment_str = f"// {comment}" if comment else ""
        param_lines.append(f"#define {key:<25} {val_str:<12} {comment_str}")
    
    content = f'''/* ================= drive_parameters.h ================= */
/* 此文件由 build_device_param.py 自动生成，请勿手动修改！ */
/* 生成时间: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")} */
#ifndef __DRIVE_PARAMETERS_H
#define __DRIVE_PARAMETERS_H

#include "main.h"

/* ---------- 版本与产品信息 ---------- */
#define PROD_NAME           "{PROD_NAME}"
#define PROD_SERIES         "{PROD_SERIES}"
#define FUN_V               "{FUN_V}"
#define FIRM_V              "{FIRM_V}"
#define BUILD_DATE_STR      "{date_str}"
#define BUILD_DATE_NUM      {date_num}U
#define BUILD_DATE_RAW      __DATE__
#define AUTHOR              "{AUTHOR}"

/* ---------- 基本运行参数 ---------- */
{chr(10).join(param_lines)}

#define F_CURRENT           {f_current}
#define F_SPEED             {f_speed}
#define F_POS               {f_pos}

/* ---------- 预拼接字符串 ---------- */
/* 值: "{drive_message}" */
#define DRIVE_MESSAGE       "{drive_message}"

/* 值: "{firm_version}" */
#define FIRM_VERSION        "{firm_version}"



#endif /* __DRIVE_PARAMETERS_H */
'''
    os.makedirs(os.path.dirname(HEADER_PATH) or ".", exist_ok=True)
    with open(HEADER_PATH, "w", encoding="utf-8") as f:
        f.write(content)
    print(f"[OK] Header generated: {HEADER_PATH}")
    print(f"     DRIVE_MESSAGE: \"{drive_message}\"")
    print(f"     FIRM_VERSION : \"{firm_version}\"")

def archive_bin(source_bin_path):
    """提取 Keil 生成的固定 bin，重命名并归档"""
    src = source_bin_path.strip().strip('"').rstrip('\\').rstrip('/')
    
    if not os.path.exists(src):
        print(f"[ERROR] Source bin not found: {src}")
        sys.exit(1)
        
    os.makedirs(TARGET_DIR, exist_ok=True)
    
    version_str = get_full_version_str()
    dest_name = f"{version_str}.bin"
    dest_path = os.path.join(TARGET_DIR, dest_name)
    
    # 覆盖旧文件
    if os.path.exists(dest_path):
        os.remove(dest_path)
        
    shutil.copy2(src, dest_path)
    print(f"[OK] Archived: {os.path.basename(src)} -> {dest_path}")

if __name__ == "__main__":
    # Windows 控制台编码兼容
    if sys.platform == 'win32':
        try: sys.stdout.reconfigure(encoding='utf-8')
        except: pass
        
    mode = sys.argv[1] if len(sys.argv) > 1 else "help"
    if mode == "header":
        generate_header()
    elif mode == "bin":
        if len(sys.argv) < 3:
            print("Usage: python build_device_param.py bin <source_bin_path>")
            sys.exit(1)
        archive_bin(sys.argv[2])
    else:
        print("Usage: python build_device_param.py [header|bin]")