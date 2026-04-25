import numpy as np
from scipy import signal

# ================= 系统基础参数 =================
FS = 20000              # 采样/控制频率 20kHz
ORDER = 2               # 滤波器阶数 (固定为2，适配CMSIS-DSP)

# ================= HFI/PLL 参数 =================
F_INJ = 1000            # HFI注入频率 1kHz
PLL_BANDWIDTH = 20      # PLL观测器带宽 20Hz

# ================= 电流环参数 (手动配置) =================
# 🔧 电机物理参数 (用于计算PI)
MOTOR_Rs = 0.06        # 相电阻 [Ω]
MOTOR_Lq = 0.00015      # q轴电感 [H]

# 🔧 手动指定目标带宽 (直接填你想要的值)
CURR_BW_TARGET_HZ = 800.0    # ⬅️ 电流环目标带宽 [Hz]

# 🔧 电流采样滤波器: 截止频率 = 带宽 × 倍数 (推荐3~4)
CURR_FILTER_FACTOR = 4.0     # 4 × 800Hz = 3200Hz
# ===========================================

def get_cmsis_coeffs(order, Wn, btype, fs):
    """生成 CMSIS-DSP 格式的 5 个系数 {b0, b1, b2, a1, a2}"""
    sos = signal.butter(order, Wn, btype=btype, output='sos', fs=fs)
    section = sos[0] 
    b0, b1, b2, a0, a1, a2 = section
    b0, b1, b2, a1, a2 = b0/a0, b1/a0, b2/a0, a1/a0, a2/a0
    return [b0, b1, b2, -a1, -a2]

def print_c_array(name, coeffs, fc_target, description=""):
    """打印C数组格式系数 + 验证信息"""
    print(f"/* {name} - 2nd Order Butterworth {description} */")
    print(f"/* 目标截止频率: {fc_target} Hz, 采样率: {FS} Hz */")
    print(f"float32_t {name}[5] = {{")
    print(f"    {coeffs[0]:.10f}f,  /* b0 */")
    print(f"    {coeffs[1]:.10f}f,  /* b1 */")
    print(f"    {coeffs[2]:.10f}f,  /* b2 */")
    print(f"    {coeffs[3]:.10f}f,  /* a1 */")
    print(f"    {coeffs[4]:.10f}f   /* a2 */")
    print("};")
    print(f"// 状态缓冲区: float32_t {name}_state[4] = {{0}};")
    print("")
    
    # 验证: 仅当 fc_target 是数值类型时执行
    b = coeffs[:3]
    a = [1, coeffs[3], coeffs[4]]
    w, h = signal.freqz(b, a, worN=8000, fs=FS)
    mag_db = 20 * np.log10(np.abs(h))
    cutoff_idx = np.where(mag_db <= -3)[0]
    if len(cutoff_idx) > 0:
        actual_fc = w[cutoff_idx[0]]
        print(f"/* ✓ 实际截止频率(-3dB): {actual_fc:.2f} Hz */")
    print(f"/* ✓ DC增益: {np.abs(h[0]):.6f} (理想=1.0) */")
    
    # ✅ 修复：只对数值型截止频率计算相位
    if isinstance(fc_target, (int, float)) and fc_target < FS/2:
        _, h_target = signal.freqz(b, a, worN=[fc_target], fs=FS)
        phase_deg = np.angle(h_target[0], deg=True)
        print(f"/* ✓ @{fc_target:.0f}Hz 相位滞后: {phase_deg:.2f}° */")
    print("")


# ================= 1. PLL速度滤波器 =================
fc_speed_lpf = PLL_BANDWIDTH * 2.0  # 40 Hz
lp_coeffs = get_cmsis_coeffs(ORDER, fc_speed_lpf, 'low', FS)
print_c_array("lpf_w_coeffs", lp_coeffs, fc_speed_lpf, "LPF for PLL speed")

# ================= 2. HFI电流带通滤波器 =================
if F_INJ > 0:
    bp_low = F_INJ * 0.85
    bp_high = F_INJ * 1.15
    bp_coeffs = get_cmsis_coeffs(ORDER, [bp_low, bp_high], 'band', FS)
    print_c_array("hfi_current_bpf_coeffs", bp_coeffs, f"{bp_low}-{bp_high}", "BPF for HFI current")
    
    sos_bpf = signal.butter(ORDER, [bp_low, bp_high], 'band', output='sos', fs=FS)
    w, h = signal.sosfreqz(sos_bpf, worN=[F_INJ], fs=FS)
    phase_deg = np.angle(h[0], deg=True)
    print(f"/* ⚠️ {F_INJ}Hz 处相位延迟: {phase_deg:.2f}° */")
    print(f"/* 需在PLL角度中补偿: theta += {phase_deg:.2f} */\n")

# ================= 3. 电流环参数计算 (手动带宽) =================
print("/* ============ 电流环参数 (手动配置) ============ */")
f_bw = CURR_BW_TARGET_HZ
omega_bw = 2 * np.pi * f_bw

# PI参数 (零极点对消法)
Kp = MOTOR_Lq * omega_bw
Ki = MOTOR_Rs * omega_bw

print(f"/* 电机参数: Rs = {MOTOR_Rs} Ω, Lq = {MOTOR_Lq*1000} mH */")
print(f"/* 目标电流环带宽: {f_bw} Hz */")
print(f"/* PI参数: Kp = {Kp:.6f}f, Ki = {Ki:.6f}f */")
print(f"/* 离散化建议: Ki_discrete = Ki * T_CON = {Ki * (1/FS):.6f}f */\n")

# ================= 4. 电流采样滤波器 =================
fc_curr_filter = f_bw * CURR_FILTER_FACTOR
curr_filter_coeffs = get_cmsis_coeffs(ORDER, fc_curr_filter, 'low', FS)
print_c_array("lpf_i_coeffs", curr_filter_coeffs, fc_curr_filter, "LPF for current sampling")

# ================= 5. 相位裕度估算 =================
print("/* ============ 相位裕度估算 ============ */")
# 滤波器在带宽处的相位滞后
b = curr_filter_coeffs[:3]
a = [1, curr_filter_coeffs[3], curr_filter_coeffs[4]]
_, h_bw = signal.freqz(b, a, worN=[f_bw], fs=FS)
phase_filt = np.angle(h_bw[0], deg=True)

# 计算延迟相位 (假设1.5个周期总延迟)
delay_cycles = 1.5
phase_delay = -360 * f_bw * delay_cycles / FS

# 总相位裕度估算
phase_margin = 180 + phase_filt + phase_delay

print(f"/* 滤波器@{f_bw}Hz相位: {phase_filt:.2f}° */")
print(f"/* 计算延迟@{f_bw}Hz相位: {phase_delay:.2f}° (假设{delay_cycles}T延迟) */")
print(f"/* ✓ 预估相位裕度: {phase_margin:.1f}° (建议 > 45°) */")
if phase_margin < 45:
    print(f"/* ⚠️ 警告: 相位裕度不足! 建议: ①降低带宽 ②提高滤波截止 ③优化计算延迟 */")
print("")

# ================= 6. 参数汇总 (方便复制) =================
print("/* ============ 参数汇总 (一键复制) ============ */")
print(f"#define CURR_LOOP_BW_HZ       {f_bw}f")
print(f"#define CURR_KP               {Kp:.6f}f")
print(f"#define CURR_KI               {Ki:.6f}f")
print(f"#define CURR_FILTER_FC_HZ     {fc_curr_filter}f")
print(f"#define PLL_SPEED_LPF_FC_HZ   {fc_speed_lpf}f")
print(f"#define HFI_INJ_FREQ_HZ       {F_INJ}U")
print(f"#define HFI_PLL_BW_HZ         {PLL_BANDWIDTH}f")