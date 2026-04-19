import numpy as np
from scipy import signal

# ================= 系统参数 =================
FS = 20000          # 采样频率 20kHz
F_INJ = 1000        # 注入频率 1kHz
F_CUTOFF_LPF = 200  # 解调后低通截止 200Hz
ORDER = 2           # 滤波器阶数 (必须为 2，适配你的 C 代码 struct coeffs[5])
# ===========================================

def get_cmsis_coeffs(order, Wn, btype, fs):
    """生成 CMSIS-DSP 格式的 5 个系数 {b0, b1, b2, a1, a2}"""
    sos = signal.butter(order, Wn, btype=btype, output='sos', fs=fs)
    section = sos[0] 
    b0, b1, b2, a0, a1, a2 = section
    b0, b1, b2, a1, a2 = b0/a0, b1/a0, b2/a0, a1/a0, a2/a0
    return [b0, b1, b2, a1, a2]

def print_c_array(name, coeffs):
    print(f"/* {name} (2nd Order, 1 Stage) */")
    print(f"float32_t {name}_coeffs[5] = {{")
    print(f"    {coeffs[0]:.10f}f, {coeffs[1]:.10f}f, {coeffs[2]:.10f}f, {coeffs[3]:.10f}f, {coeffs[4]:.10f}f")
    print("};")
    print(f"/* 状态缓冲区：float32_t {name}_state[4] = {{0}}; */")
    print(f"/* 初始化：arm_biquad_cascade_df1_init_f32(&inst, 1, {name}_coeffs, {name}_state); */")
    print("")

# 1. 带通滤波器 (提取 1kHz 高频电流)
bp_coeffs = get_cmsis_coeffs(ORDER, [F_INJ*0.85, F_INJ*1.15], 'band', FS)
print_c_array("lpf_coeffs", bp_coeffs)

# 2. 低通滤波器 (解调后误差滤波)
lp_coeffs = get_cmsis_coeffs(ORDER, F_CUTOFF_LPF, 'low', FS)
print_c_array("lpf_coeffs", lp_coeffs)

# 3. 相位延迟计算 (兼容不同 scipy 版本)
sos_bpf = signal.butter(ORDER, [F_INJ*0.85, F_INJ*1.15], 'band', output='sos', fs=FS)
try:
    # 新版 scipy (>=1.9)
    w, h = signal.sosfreqz(sos_bpf, worN=[F_INJ], fs=FS)
except TypeError:
    # 旧版 scipy
    w, h = signal.sosfreqz(sos_bpf, worN=[F_INJ], fs=FS)
phase_deg = np.angle(h[0], deg=True)
print(f"/* ⚠️ 1kHz 处相位延迟：{phase_deg:.2f} 度 (需在位置估算中补偿) */")