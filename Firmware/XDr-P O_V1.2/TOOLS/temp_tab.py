import math

def r_to_temp(r):
    return 1 / (1/298.15 + (1/3380)*math.log(r/10)) - 273.15

def adc_to_temp(adc, vbus=24, vref=3.3, rpull=150):
    v = (adc / 256) * vref
    if v <= 0: return 120
    if v >= vbus: return 0
    r = v * rpull / (vbus - v)
    t = r_to_temp(r)
    return max(0, min(120, t))

table = [int(round(adc_to_temp(i))) for i in range(256)]
# 打印为 C 数组
for i in range(0, 256, 16):
    print(', '.join(f'{table[i+j]:3d}' for j in range(16)) + ',')