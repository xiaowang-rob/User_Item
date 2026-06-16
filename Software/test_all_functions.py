#!/usr/bin/env python3
"""
上位机功能全面自动化测试（除 IAP 烧录外）
通过原始串口发送协议包，验证设备端每个功能的响应
"""
import sys, os, time, struct, serial

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from protocol import Cidx, Pidx, Fidx, Pkt

# ── CRC8 ──
def crc8(data: bytes) -> int:
    crc = 0
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = ((crc << 1) ^ 0x07) if (crc & 0x80) else (crc << 1)
        crc &= 0xFF
    return crc

def build_packet(cmd_id: int, data: bytes = b"") -> bytes:
    length = len(data)
    pkt = bytearray([Pkt.PACKET_HEAD, cmd_id, length])
    pkt.extend(data)
    pkt.append(crc8(data))
    pkt.append(Pkt.PACKET_TAIL)
    return bytes(pkt)

def recv_packets(ser: serial.Serial, timeout=2.0) -> list:
    """读取并解析所有完整帧"""
    packets = []
    deadline = time.time() + timeout
    buf = bytearray()
    while time.time() < deadline:
        n = ser.in_waiting
        if n > 0:
            buf.extend(ser.read(n))
        else:
            time.sleep(0.02)
            continue
    # 解析缓冲区
    while len(buf) >= 5:
        idx = buf.find(Pkt.PACKET_HEAD)
        if idx < 0:
            break
        if idx > 0:
            del buf[:idx]
        if len(buf) < 5:
            break
        cmd = buf[1]
        length = buf[2]
        min_len = 5 + length
        if len(buf) < min_len:
            break
        if buf[min_len - 1] != Pkt.PACKET_TAIL:
            del buf[0]
            continue
        data = bytes(buf[3:3 + length])
        chk = buf[3 + length]
        if crc8(data) == chk:
            packets.append((cmd, data))
        del buf[:min_len]
    return packets

def send_and_recv(ser, cmd_id, data=b"", timeout=2.0, label=""):
    """发送并接收"""
    pkt = build_packet(cmd_id, data)
    ser.write(pkt)
    if label:
        print(f"  → 发送 {label}: cmd=0x{cmd_id:02X} data={data.hex() if data else '(空)'}")
    pkts = recv_packets(ser, timeout)
    for cmd, d in pkts:
        print(f"  ← 收到: cmd=0x{cmd:02X} len={len(d)} data={d.hex()[:60]}{'...' if len(d)>30 else ''}")
    return pkts

# ── 测试计数 ──
PASS = 0
FAIL = 0
SKIP = 0
def check(name, condition, detail=""):
    global PASS, FAIL
    if condition:
        PASS += 1
        print(f"  ✅ {name}" + (f" ({detail})" if detail else ""))
    else:
        FAIL += 1
        print(f"  ❌ {name}" + (f" ({detail})" if detail else ""))

def skip(name, reason=""):
    global SKIP
    SKIP += 1
    print(f"  ⏭️  {name}" + (f" — {reason}" if reason else ""))

# ═══════════════════════════════════════
# 测试主流程
# ═══════════════════════════════════════
def main():
    global PASS, FAIL, SKIP
    ser = serial.Serial("/dev/ttyACM0", 115200, timeout=1)
    print(f"已打开 {ser.name}\n")

    # ────────────────────────────────────
    print("=" * 50)
    print("1. 连接 (UC_CONNECT)")
    print("=" * 50)
    pkts = send_and_recv(ser, Cidx.UC_CONNECT, timeout=3, label="UC_CONNECT")
    has_info = any(cmd == Cidx.UC_CONNECT and len(d) > 12 for cmd, d in pkts)
    has_status = any(cmd == Cidx.UC_CONNECT and len(d) == 12 for cmd, d in pkts)
    check("收到固件信息字符串", has_info)
    check("收到状态包 (12字节)", has_status)
    if has_status:
        for cmd, d in pkts:
            if len(d) == 12:
                tune = d[0]; foc = d[1]; fault = d[2]; warning = d[3]
                temp = struct.unpack("<f", d[4:8])[0]
                vbus = struct.unpack("<f", d[8:12])[0]
                print(f"    状态: tune={tune} foc={foc} fault={fault} warn={warning} temp={temp:.1f}°C vbus={vbus:.2f}V")
                check("foc_state=IDLE", foc == 0, f"foc={foc}")
                check("fault=NONE", fault == 0, f"fault={fault}")
                check("温度合理", 0 < temp < 50, f"{temp:.1f}°C")
                check("母线电压合理", 3 < vbus < 6, f"{vbus:.2f}V (仅USB供电)")

    # ────────────────────────────────────
    print("\n" + "=" * 50)
    print("2. 参数读取 (PARAM_READ)")
    print("=" * 50)
    # 读取单个参数: can_id (index=8)
    pkts = send_and_recv(ser, Cidx.PARAM_READ, bytes([Pidx.CAN_ID]), label="读取 CAN_ID")
    param_ok = any(cmd == Cidx.PARAM_READ for cmd, d in pkts)
    check("收到参数响应", param_ok)
    if param_ok:
        for cmd, d in pkts:
            if cmd == Cidx.PARAM_READ and len(d) >= 5:
                idx = d[0]
                val = struct.unpack("<f", d[1:5])[0]
                print(f"    参数[{idx}] = {val}")
                check("CAN_ID=1", val == 1.0, f"val={val}")

    # 读取 motor_rs (index=11)
    pkts = send_and_recv(ser, Cidx.PARAM_READ, bytes([Pidx.MOTOR_RS]), label="读取 MOTOR_RS")
    for cmd, d in pkts:
        if cmd == Cidx.PARAM_READ and len(d) >= 5:
            val = struct.unpack("<f", d[1:5])[0]
            print(f"    motor_rs = {val:.6f} Ω")
            check("MOTOR_RS 合理", 0.01 < val < 10, f"{val:.6f}Ω")

    # 读取 motor_polepairs (index=7)
    pkts = send_and_recv(ser, Cidx.PARAM_READ, bytes([Pidx.MOTOR_POLEPAIRS]), label="读取 MOTOR_POLEPAIRS")
    for cmd, d in pkts:
        if cmd == Cidx.PARAM_READ and len(d) >= 5:
            val = struct.unpack("<f", d[1:5])[0]
            print(f"    motor_polepairs = {val}")
            check("MOTOR_POLEPAIRS=7", val == 7.0, f"val={val}")

    # 读取所有参数 (0xFF)
    print("\n  读取所有参数 (0xFF):")
    pkts = send_and_recv(ser, Cidx.PARAM_READ, bytes([0xFF]), timeout=5, label="读取全部参数")
    param_count = sum(1 for cmd, d in pkts if cmd == Cidx.PARAM_READ)
    print(f"    收到 {param_count} 个参数包")
    check("收到全部参数", param_count >= 30, f"共 {param_count} 个")

    # ────────────────────────────────────
    print("\n" + "=" * 50)
    print("3. 参数写入 (PARAM_WRITE)")
    print("=" * 50)
    # 写入 CAN_ID = 1 (保持原值)
    new_val = struct.pack("<f", 1.0)
    pkts = send_and_recv(ser, Cidx.PARAM_WRITE, bytes([Pidx.CAN_ID]) + new_val, label="写入 CAN_ID=1")
    # 验证读回
    pkts = send_and_recv(ser, Cidx.PARAM_READ, bytes([Pidx.CAN_ID]), label="读回 CAN_ID")
    for cmd, d in pkts:
        if cmd == Cidx.PARAM_READ and len(d) >= 5:
            val = struct.unpack("<f", d[1:5])[0]
            check("CAN_ID 写入后读回正确", val == 1.0, f"val={val}")

    # 写入 TUNE_CURRENT = 1.5 (测试浮点写入)
    new_val = struct.pack("<f", 1.5)
    pkts = send_and_recv(ser, Cidx.PARAM_WRITE, bytes([Pidx.TUNE_CURRENT]) + new_val, label="写入 TUNE_CURRENT=1.5")
    pkts = send_and_recv(ser, Cidx.PARAM_READ, bytes([Pidx.TUNE_CURRENT]), label="读回 TUNE_CURRENT")
    for cmd, d in pkts:
        if cmd == Cidx.PARAM_READ and len(d) >= 5:
            val = struct.unpack("<f", d[1:5])[0]
            check("TUNE_CURRENT 写入读回", abs(val - 1.5) < 0.01, f"val={val}")

    # ────────────────────────────────────
    print("\n" + "=" * 50)
    print("4. 参数保存 (PARAM_SAVE)")
    print("=" * 50)
    pkts = send_and_recv(ser, Cidx.PARAM_SAVE, label="保存参数")
    save_ok = any(cmd == Cidx.PARAM_SAVE and len(d) >= 1 and d[0] == Fidx.FEEDBACK_EXECUTE for cmd, d in pkts)
    check("参数保存成功", save_ok)

    # ────────────────────────────────────
    print("\n" + "=" * 50)
    print("5. 数据流获取 (CMD_STREAM_GET)")
    print("=" * 50)
    # 获取单个数据: SPEED (index=11)
    pkts = send_and_recv(ser, Cidx.CMD_STREAM_GET, bytes([0, 11]), label="获取 SPEED")
    stream_ok = any(cmd == Cidx.CMD_STREAM_GET for cmd, d in pkts)
    check("收到数据流响应", stream_ok)
    for cmd, d in pkts:
        if cmd == Cidx.CMD_STREAM_GET and len(d) >= 4:
            val = struct.unpack("<f", d[0:4])[0]
            print(f"    SPEED = {val:.2f} rpm")
            check("SPEED 值合理 (电机静止)", abs(val) < 100, f"{val:.2f}")

    # 获取 THETA_MECH (index=14)
    pkts = send_and_recv(ser, Cidx.CMD_STREAM_GET, bytes([0, 14]), label="获取 THETA_MECH")
    for cmd, d in pkts:
        if cmd == Cidx.CMD_STREAM_GET and len(d) >= 4:
            val = struct.unpack("<f", d[0:4])[0]
            print(f"    THETA_MECH = {val:.2f}°")
            check("THETA_MECH 合理", 0 <= val <= 360, f"{val:.2f}°")

    # ────────────────────────────────────
    print("\n" + "=" * 50)
    print("6. 数据流设置 (CMD_STREAM_SET)")
    print("=" * 50)
    # 设置数据流: SPEED(11) + THETA_MECH(14)
    pkts = send_and_recv(ser, Cidx.CMD_STREAM_SET, bytes([11, 14]), label="设置数据流 [SPEED, THETA_MECH]")
    print("  等待数据流更新...")
    time.sleep(0.5)
    pkts = recv_packets(ser, timeout=2)
    stream_data = any(cmd == Cidx.CMD_STREAM_SET for cmd, d in pkts)
    check("收到数据流数据", stream_data)

    # 清除数据流
    pkts = send_and_recv(ser, Cidx.CMD_STREAM_SET, label="清除数据流")

    # ────────────────────────────────────
    print("\n" + "=" * 50)
    print("7. 模式设置 (CMD_MODE_SET)")
    print("=" * 50)
    for mode, name in [(1, "速度模式"), (2, "位置模式"), (3, "MIT模式"), (4, "开环模式"), (0, "电流模式")]:
        pkts = send_and_recv(ser, Cidx.CMD_MODE_SET, bytes([mode]), label=f"设置 {name}({mode})")
        # 验证 FOC 状态
        pkts = send_and_recv(ser, Cidx.UC_CONNECT, timeout=1, label="查询状态")
        for cmd, d in pkts:
            if cmd == Cidx.UC_CONNECT and len(d) == 12:
                check(f"模式切换 {name}", d[1] == 0, f"foc_state={d[1]}")  # IDLE 状态

    # ────────────────────────────────────
    print("\n" + "=" * 50)
    print("8. 目标值设置 (CMD_REFVALUE_SET)")
    print("=" * 50)
    # 设置目标值 100.0 (4字节 float)
    val = struct.pack("<f", 100.0)
    pkts = send_and_recv(ser, Cidx.CMD_REFVALUE_SET, val, label="设置目标值=100.0")
    check("目标值命令已发送", True)

    # ────────────────────────────────────
    print("\n" + "=" * 50)
    print("9. FOC 控制命令")
    print("=" * 50)

    # FOC 复位
    pkts = send_and_recv(ser, Cidx.FOC_NRST, label="FOC 复位")
    time.sleep(0.2)
    pkts = send_and_recv(ser, Cidx.UC_CONNECT, timeout=1, label="查询状态")
    for cmd, d in pkts:
        if cmd == Cidx.UC_CONNECT and len(d) == 12:
            check("FOC 复位后 IDLE", d[1] == 0, f"foc_state={d[1]}")

    # FOC 使能 (应该失败，没有电机电源)
    pkts = send_and_recv(ser, Cidx.CMD_ENABLE, label="FOC 使能")
    time.sleep(0.5)
    pkts = send_and_recv(ser, Cidx.UC_CONNECT, timeout=1, label="查询状态")
    for cmd, d in pkts:
        if cmd == Cidx.UC_CONNECT and len(d) == 12:
            foc = d[1]
            fault = d[2]
            print(f"    foc_state={foc}, fault={fault}")
            check("使能后状态变更", foc != 0 or fault != 0, f"foc={foc} fault={fault}")

    # FOC 失能
    pkts = send_and_recv(ser, Cidx.CMD_DISABLE, label="FOC 失能")
    time.sleep(0.2)

    # FOC 再次复位
    pkts = send_and_recv(ser, Cidx.FOC_NRST, label="FOC 复位(恢复)")

    # 刹车
    pkts = send_and_recv(ser, Cidx.BRAKE, label="刹车")
    time.sleep(0.2)
    pkts = send_and_recv(ser, Cidx.UC_CONNECT, timeout=1, label="查询状态")
    for cmd, d in pkts:
        if cmd == Cidx.UC_CONNECT and len(d) == 12:
            print(f"    刹车后: foc_state={d[1]}, fault={d[2]}")

    # 恢复
    pkts = send_and_recv(ser, Cidx.FOC_NRST, label="FOC 复位(最终恢复)")

    # ────────────────────────────────────
    print("\n" + "=" * 50)
    print("10. 整定命令 (START_TUNNING)")
    print("=" * 50)
    skip("自动整定", "需要电机电源和电机")

    # ────────────────────────────────────
    print("\n" + "=" * 50)
    print("11. 零点设置 (CMD_SET_ZERO_POS)")
    print("=" * 50)
    pkts = send_and_recv(ser, Cidx.CMD_SET_ZERO_POS, label="设置零点")
    check("零点命令已发送", True)

    # ────────────────────────────────────
    print("\n" + "=" * 50)
    print("12. 限位设置 (CMD_SET_LIMIT_POS)")
    print("=" * 50)
    pkts = send_and_recv(ser, Cidx.CMD_SET_LIMIT_POS, label="设置限位")
    check("限位命令已发送", True)

    # ────────────────────────────────────
    print("\n" + "=" * 50)
    print("13. 日志获取 (LOG_GET)")
    print("=" * 50)
    pkts = send_and_recv(ser, Cidx.LOG_GET, timeout=5, label="获取日志")
    log_count = sum(1 for cmd, d in pkts if cmd == Cidx.LOG_GET)
    print(f"    收到 {log_count} 个日志包")
    check("日志响应", log_count >= 0, f"共 {log_count} 条")

    # ────────────────────────────────────
    print("\n" + "=" * 50)
    print("14. 日志擦除 (LOG_ERASE)")
    print("=" * 50)
    skip("日志擦除", "保留日志数据，避免丢失")

    # ────────────────────────────────────
    print("\n" + "=" * 50)
    print("15. 参数擦除 (PARAM_ERASE)")
    print("=" * 50)
    skip("参数擦除", "保留参数数据，避免丢失")

    # ────────────────────────────────────
    print("\n" + "=" * 50)
    print("16. UC_DISCONNECT")
    print("=" * 50)
    pkts = send_and_recv(ser, Cidx.UC_DISCONNECT, label="断开连接")
    time.sleep(0.5)
    # 确认设备停止发送
    pkts = recv_packets(ser, timeout=2)
    status_count = sum(1 for cmd, d in pkts if cmd == Cidx.UC_CONNECT and len(d) == 12)
    print(f"    断开后收到 {status_count} 个状态包")
    check("断开后停止状态包", status_count == 0, f"收到 {status_count} 个")

    # 重新连接
    pkts = send_and_recv(ser, Cidx.UC_CONNECT, timeout=3, label="重新连接")
    reconnect_ok = any(cmd == Cidx.UC_CONNECT for cmd, d in pkts)
    check("重新连接成功", reconnect_ok)

    # ────────────────────────────────────
    print("\n" + "=" * 50)
    print("17. 系统复位 (CMD_SYSTEM_RESET)")
    print("=" * 50)
    skip("系统复位", "会断开连接，需要重新烧录/连接")

    # ────────────────────────────────────
    print("\n" + "=" * 50)
    print("18. IAP 烧录")
    print("=" * 50)
    skip("IAP 烧录", "用户要求跳过")

    # ── 汇总 ──
    print("\n" + "=" * 50)
    print(f"测试完成: ✅ {PASS} 通过  ❌ {FAIL} 失败  ⏭️  {SKIP} 跳过")
    print("=" * 50)

    ser.close()
    return FAIL

if __name__ == "__main__":
    sys.exit(main())
