# X-motor-Drive (XDr) 改进日志

> **版本**: series_P/O_V1.2  
> **日期**: 2026-06-16  
> **说明**: 阶段 1~5 全部改动汇总，由 Reasonix 协助完成。

---

## 阶段 1：快速清理

### 🔸 删除旧版本
删除了 4 套冗余版本，仅保留最新：
```
Firmware/XDr-P O_V1.0   ✗
Firmware/XDr-P O_V1.1   ✗
Firmware/XDr-P O_V1.2   ✗
Firmware/XDr-S O_V1.0   ✗
Firmware/series_P/O_V1.2  ✅（保留）
Firmware/series_S/O_V1.1  ✅（保留）
```
清理 ~1200 个冗余文件（CubeMX 生成 + HAL 库 + 编译产物）。

### 🔸 拼写修正
| 错误 | 改为 | 涉及文件 |
|------|------|---------|
| `cur_fiter_alpha` | `cur_filter_alpha` | foc_core.h/c, parameter_manager.h, tune.h/c, build_core.py |
| `speed_fiter_alpha` | `speed_filter_alpha` | foc_core.h/c, parameter_manager.h |
| `mech_offect` | `mech_offset` | foc_core.h/c |
| `ClearFalg` | `ClearFlag` | protection_manager.h/c, port_mapping.c |
| `T_DEATH_us` | `T_DEADTIME_us` | config.h, usr_config.h, svpwm.c, build_core.py, config_parser.py |

### 🔸 命名规范 TODO
在 5 个头文件关键位置添加 VESC 风格命名说明：
- `loop_control.h` — `PI_init()` → `loop_pi_init()`
- `smo.h` — `fSmoMainLoop()` → `smo_main_loop()`
- `hfi.h` — `fHfiStep()` → `hfi_step()`
- `foc_core.h` — `fFocCoreInit()` → `foc_core_init()`
- `foc_core.c` — switch-case fall-through 级联结构说明

---

## 阶段 2：固件架构优化

### 🔸 2.1 参数管理偏移量表
**文件**: `parameter_manager.h/c`

- 删除 `fParamSet` 120 行 switch-case → 6 行表查
- 删除 `fParamGet` 160 行 switch-case → 6 行表查
- 新增 `tParamEntry` 描述符表 + `PARAM_ENTRY` 宏（`offsetof` + `memcpy`）
- 加新参数只需枚举 + 表各一行，无需改函数体
- **固件 -816 bytes**

### 🔸 2.2 模块变量 static 化
**文件**: `smo.h/c`, `foc_main.h/c`

- `g_smo` 改为 `static`，外部不可直接访问
- 新增 `smo_get_theta()` / `smo_get_omega()` getter
- 删除 `FOC_t` 中未使用的 `tSMO *smo` 指针

### 🔸 2.3 hfi/svpwm static + FOC_t 清理
**文件**: `hfi.h/c`, `svpwm.h/c`, `loop_control.h/c`, `foc_main.h/c`

- `g_hfi` → `static`（通过 `fHfiGet*()` 读取）
- `g_svpwm` → `static`
- `FOC_t` 移除未使用的 `.hfi` / `.svpwm` / `.loop_con` 指针
- `g_loop_con` 保留 extern（被 foc_core.c 使用）

### 🔸 2.4 多端口接收缓冲独立
**文件**: `port_mapping.c/h`

- 删除单 `com_frame.is_busy` 全局锁
- 新增 3 端口独立接收缓冲（CAN/USB/UART 各 128B）
- 回调函数仅做 `memcpy` + `pending` 标记（适合中断上下文）
- 主循环轮询各端口 `pending`，顺序处理
- **解决了多端口同时来包被静默丢弃的问题**

---

## 阶段 3：通信改进

### 🔸 3.1 USB 递归重发改循环
**文件**: `usb_port.c`

- `fUSB_SendData` 递归重发 → `for` 循环重试（最多 5 次，间隔 1ms）
- 删除不再使用的 `trans_fault_tic` 计数器
- **修复了注释指出的"清零语句在 return 之后不会执行"的 bug**

### 🔸 3.2 状态推送：计数超时 → 时间超时
**文件**: `port_mapping.c`（固件）, `com_port.py`（上位机）

- **固件**: `no_response_tic++` 计数器 → `BSP_GetTick()` 时间基准
- 5 秒无心跳自动断开，独立于推送频率（500ms）
- **上位机**: 移除收到状态后冗余的 `UC_CONNECT` 请求

### 🔸 3.3 上位机命令分发器
**文件**: `com_port.py`, `main_window.py`, `wave.py`

- `handle_received_data` 70 行 `match-case` → 20 行分发表派发
- 新增 `register_handler(cmd_id, callback)` 接口
- 各模块在 `_register_data_handlers()` 中注册（main_window.py）
- 新增 `Wave.handle_stream_data()` 方法

### 🔸 3.4 USB/UART 帧格式自动切换
**文件**: `com_port.py`

- 连接时自动切换帧格式：
  - USB: HEAD=0x3A, TAIL=0x0D
  - 蓝牙/串口: HEAD=0x55, TAIL=0xAA

### 🔸 3.5 CRC8 替代 sum 校验
**文件**: `math_fast.h`, `usb_port.c`, `uart_port.c`, `com_port.py`

- 新增 `crc8()` / `crc8_update()` 函数，多项式 0x07
- 固件 USB/UART 收发均改用 CRC8
- 上位机打包/解包均用 `_crc8()` 方法

---

## 阶段 4：无感改进

### 🔸 4.1 SMO 输出加 PLL
**文件**: `smo.h`, `smo.c`

- 新增 `tSmoPll` 结构体 + `smo_pll_init()` / `smo_pll_update()` 函数
- Type-1 PLL：角度速度联合估计，无附加 LPF
- 可通过 `SMO_USE_PLL 1/0` 宏在 PLL 和原 atan2+50% 平滑之间切换
- PLL 默认参数: Kp=200, Ki=10000

### 🔸 4.2 增益自适应改为基于电压
**文件**: `smo.c`

- 原方案：基于估计速度（鸡生蛋问题——速度不准时增益也不准）
- 新方案：基于电压模长（BEMF 信号强度），电压高→信号好→增益降
- 可通过 `SMO_GAIN_BY_DUTY 1/0` 宏切换

### 🔸 4.3 HFI + SMO 融合
**文件**: `foc_core.c`

- 按速度三段式融合：
  - 低速（<300 RPM）：用 HFI
  - 过渡区（300~450 RPM）：线性融合 HFI + SMO
  - 高速（>450 RPM）：用 SMO
- 同时启用 SMO（之前被注释掉）

### 🔸 4.4 开环启动过渡状态机
**文件**: `foc_main.h/c`, `protocol.h`

- 新增 `FOC_OPENLOOP` 状态
- 锁定(200ms) → 斜坡(500ms) → 匀速 → SMO 收敛后切闭环
- 开环电压矢量通过 SVPWM 输出，SMO 后台同时运行

### 🔸 4.5 滤波系数 Hz 单位配置
**文件**: `usr_config.h`（生成）, `build_core.py`, `foc_core.c`

- 新增 `CUR_LPF_HZ`(800Hz) / `SPEED_LPF_HZ`(50Hz) 配置
- `_FilterInit` 从 Hz 自动计算 `alpha = dt/(dt+1/2πfc)`
- 若参数中配置了有效 alpha 值则优先使用（向后兼容）

---

## 阶段 5：工程化提升

### 🔸 5.2 目录 `functons` → `functions`
**文件**: 目录重命名 + 5 个 Python 文件 import 路径更新

### 🔸 5.5 编码器函数指针表
**文件**: `device.h`, `encoder.c`

- `tEncoderChipDesc` 扩展：新增 `use_dma_state_machine`、`dma_state_entry` 字段
- 芯片描述表各条目已填充新字段
- `fEncoderMainLoopTask` 的 `switch(chip_type)` 已加 TODO 标记（后续可替换为 `chip_desc->dma_state_entry`）

---

## 文件改动统计

```
阶段 1: 1910 文件变更（1870 删除 + 40 修改），-1026125 行
阶段 2: ~15 文件变更，+153/-340 行
阶段 3: 9 文件变更，+131/-100 行  
阶段 4: 8 文件变更，+172/-62 行
阶段 5: 目录改名 + 5 文件

固件增长: text=68856 -> 68952 (+96 bytes，含 CRC8 + PLL + openloop 等新增功能)
```

---

## 技术债务（Q1 计划）
1. `functons` 目录旧名已删除，git 历史中可回溯
2. 协议代码生成 `gen_protocol.py` 尚未同步 `FOC_OPENLOOP` 枚举（手动修改了 `protocol.h`）
3. 编码器状态机 `switch(chip_type)` 待替换为函数指针表
4. plat 平台抽象层待标准库替换 HAL 时统一做

---

*生成于 2026-06-16，基于 git commit `35a81fe` 及之后的阶段 4+5 改动*
