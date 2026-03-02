import scipy.signal as signal
import numpy as np

def calculate_butterworth_coeffs(fc, fs, order=2):
    """
    计算巴特沃斯低通滤波器系数，适配 CMSIS-DSP arm_biquad_cascade_df1_f32
    
    参数:
    fc: 截止频率 (Hz)
    fs: 采样频率 (Hz)
    order: 滤波器阶数 (建议 2)
    """
    # 1. 设计滤波器
    # nyq = 0.5 * fs
    # normal_cutoff = fc / nyq
    # b, a = signal.butter(order, normal_cutoff, btype='low', analog=False)
    
    # 使用双线性变换法直接计算数字滤波器系数
    b, a = signal.butter(order, fc, fs=fs, btype='low')
    
    print(f"--- 滤波器参数 ---")
    print(f"截止频率：{fc} Hz")
    print(f"采样频率：{fs} Hz")
    print(f"阶数：{order}")
    print(f"分子系数 (b): {b}")
    print(f"分母系数 (a): {a}")
    
    # 2. 转换为 CMSIS-DSP 格式
    # CMSIS 结构体 pCoeffs 布局为：{b0, b1, b2, a1, a2}
    # 注意：CMSIS 内部计算是减法： - a1 * y[n-1] - a2 * y[n-2]
    # scipy 输出的 a 是 [1, a1, a2]，对应 y + a1*y[-1] + a2*y[-2] = ...
    # 所以传入 CMSIS 的 a1, a2 应该是 scipy 输出的 a[1], a[2] (保持正号，库函数内部会减)
    # 但为了保险，我们检查 CMSIS 文档：
    # arm_biquad_cascade_df1_f32 implements: 
    # y[n] = b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] - a1 * y[n-1] - a2 * y[n-2]
    # scipy 传递函数： H(z) = (b0 + b1 z^-1 + b2 z^-2) / (1 + a1 z^-1 + a2 z^-2)
    # 对比可知：CMSIS 的 a1 对应 scipy 的 a[1]，CMSIS 的 a2 对应 scipy 的 a[2]。
    # 直接填入即可，库函数内部有负号。
    
    cmsis_coeffs = []
    # 2 阶滤波器只有一个二阶节 (numStages = 1)
    # 系数数组长度 = 5 * numStages = 5
    cmsis_coeffs.append(b[0])
    cmsis_coeffs.append(b[1])
    cmsis_coeffs.append(b[2])
    cmsis_coeffs.append(a[1])  # 注意：这里直接填 a[1]，库函数内部做减法
    cmsis_coeffs.append(a[2])  # 注意：这里直接填 a[2]，库函数内部做减法
    
    print(f"\n--- C 代码数组初始化 ---")
    print(f"float32_t hfi_speed_coeffs[5] = {{")
    print(f"    {cmsis_coeffs[0]:.10f}f,  // b0")
    print(f"    {cmsis_coeffs[1]:.10f}f,  // b1")
    print(f"    {cmsis_coeffs[2]:.10f}f,  // b2")
    print(f"    {cmsis_coeffs[3]:.10f}f,  // a1")
    print(f"    {cmsis_coeffs[4]:.10f}f   // a2")
    print(f"}};")
    
    return cmsis_coeffs

# ================= 使用示例 =================
if __name__ == "__main__":
    # 根据你的实际系统修改这里
    CONTROL_FREQ = 10000.0  # 控制中断频率 10kHz
    CUT_OFF_FREQ = 50.0     # 速度滤波截止频率 50Hz
    
    calculate_butterworth_coeffs(CUT_OFF_FREQ, CONTROL_FREQ, order=2)