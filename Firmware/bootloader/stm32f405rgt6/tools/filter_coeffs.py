"""巴特沃斯滤波器系数计算（需要 scipy）"""
import sys

def compute_filter_coeffs(cfg):
    try:
        from scipy import signal
    except ImportError:
        print("错误: 需要 scipy 库。请运行: pip install scipy numpy")
        sys.exit(1)

    FS = 20000
    ORDER = 2
    filt = cfg["filter"]

    fc_speed = filt["pll_bandwidth_hz"] * filt["speed_lpf_factor"]
    sos_w = signal.butter(ORDER, fc_speed, btype="low", output="sos", fs=FS)
    b0, b1, b2, a0, a1, a2 = sos_w[0]
    lpf_w = [b0/a0, b1/a0, b2/a0, -a1/a0, -a2/a0]

    fc_curr = filt["curr_bw_target_hz"] * filt["curr_filter_factor"]
    sos_i = signal.butter(ORDER, fc_curr, btype="low", output="sos", fs=FS)
    b0, b1, b2, a0, a1, a2 = sos_i[0]
    lpf_i = [b0/a0, b1/a0, b2/a0, -a1/a0, -a2/a0]

    return {
        "lpf_w": lpf_w,
        "lpf_i": lpf_i,
        "fc_speed": fc_speed,
        "fc_curr": fc_curr,
        "FS": FS,
    }