import numpy as np

def svpwm_inverter_terminal_voltages(ud_cmd, uq_cmd, theta_e, Vdc):
    """
    输入 ud, uq, 电角度, 母线电压，
    输出：
      - 实际施加的 ud, uq（考虑限幅）
      - 相电压（对中性点）
      - 逆变器端对地电压 va, vb, vc
      - SVPWM 扇区（1~6）
    """
    # 1. Park 反变换: dq -> αβ
    cos_t = np.cos(theta_e)
    sin_t = np.sin(theta_e)
    
    u_alpha_cmd = cos_t * ud_cmd - sin_t * uq_cmd
    u_beta_cmd  = sin_t * ud_cmd + cos_t * uq_cmd

    # 2. 限幅（SVPWM 线性区最大幅值）
    U_cmd = np.sqrt(u_alpha_cmd**2 + u_beta_cmd**2)
    U_max = Vdc / np.sqrt(3)  # 线性调制上限

    if U_cmd > U_max:
        scale = U_max / U_cmd
        u_alpha = u_alpha_cmd * scale
        u_beta  = u_beta_cmd  * scale
        # 反推实际 ud, uq
        ud_actual = cos_t * u_alpha + sin_t * u_beta
        uq_actual = -sin_t * u_alpha + cos_t * u_beta
    else:
        u_alpha = u_alpha_cmd
        u_beta  = u_beta_cmd
        ud_actual = ud_cmd
        uq_actual = uq_cmd

    # 3. 计算扇区（基于限幅后的 u_alpha, u_beta）
    # 定义三个判别式
    A = u_beta
    B = np.sqrt(3)/2 * u_alpha - 0.5 * u_beta
    C = -np.sqrt(3)/2 * u_alpha - 0.5 * u_beta

    print(f"A={A:.2f}, B={B:.2f}, C={C:.2f}")
    # 判断正负（>0 为 True -> 1，否则 0）
    a = 1 if A > 0 else 0
    b = 1 if B > 0 else 0
    c = 1 if C > 0 else 0

    # 组合成三位二进制数（注意顺序：CBA 或 ABC？）
    # 标准查表法使用 N = 4*c + 2*b + a
    N = c * 4 + b * 2 + a

    # 扇区映射表（N -> sector）
    sector_map = {
        1: 2,
        2: 6,
        3: 1,
        4: 4,
        5: 3,
        6: 5,
        7: 1  # 理论上不会出现，但防止数值误差
    }

    # 特殊情况：电压矢量为零（原点）
    if U_cmd == 0:
        sector = 0  # 或设为1，但通常零矢量不区分扇区
    else:
        sector = sector_map.get(N, 1)  # 默认扇区1

    print(f"{u_alpha:.2f}, {u_beta:.2f} -> {N} -> {sector}")
    # 4. Clarke 反变换: αβ -> abc (相电压，对中性点)
    ua = u_alpha
    ub = -0.5 * u_alpha + (np.sqrt(3)/2) * u_beta
    uc = -0.5 * u_alpha - (np.sqrt(3)/2) * u_beta

    print(f"ua={ua:.2f}, ub={ub:.2f}, uc={uc:.2f}")
    # 5. 逆变器端对地电压（对母线负端）
    v_cm = Vdc / 2.0
    va = ua + v_cm
    vb = ub + v_cm
    vc = uc + v_cm

    return {
        'ud_actual': ud_actual,
        'uq_actual': uq_actual,
        'ua_neutral': ua,
        'ub_neutral': ub,
        'uc_neutral': uc,
        'va_terminal': va,   # 你要的：电机端子对母线负电压
        'vb_terminal': vb,
        'vc_terminal': vc,
        'u_alpha': u_alpha,
        'u_beta': u_beta,
        'sector': sector,    # SVPWM 扇区 (1~6)
        'voltage_magnitude': np.sqrt(u_alpha**2 + u_beta**2),
        'is_saturated': U_cmd > U_max,
        'common_mode_voltage': v_cm
    }

# 示例测试
if __name__ == "__main__":
    Vdc = 24.0
    theta = 1.67970896
    ud_in = 0.0
    uq_in = 2       # 纯 q 轴，应在扇区1附近

    res = svpwm_inverter_terminal_voltages(ud_in, uq_in, theta, Vdc)

    print("=== SVPWM 输出结果 ===")
    print(f"指令: ud={ud_in:.2f}, uq={uq_in:.2f}, θ={np.degrees(theta):.1f}°")
    print(f"实际: ud={res['ud_actual']:.2f}, uq={res['uq_actual']:.2f}")
    print(f"αβ 电压: uα={res['u_alpha']:.2f}, uβ={res['u_beta']:.2f}")
    print(f"扇区: {res['sector']}")  # 👈 新增
    print(f"端电压: va={res['va_terminal']:.2f} V, vb={res['vb_terminal']:.2f} V, vc={res['vc_terminal']:.2f} V")
    print(f"是否饱和: {res['is_saturated']}")
    print(f"电压幅值: {res['voltage_magnitude']:.2f} V (max={Vdc/np.sqrt(3):.2f} V)")