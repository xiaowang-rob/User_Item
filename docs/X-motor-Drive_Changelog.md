# X-motor-Drive (XDr) 改进日志

> **版本**: series_P/O_V1.2  
> **日期**: 2026-06-16  
> **提交**: `f171e0a` ~ `253857c` 阶段 1~5, `8a152dd` P0, `55cfabe` P1, `2213997` P2

---

## 阶段 1：快速清理

### 🔸 删除旧版本
- 删除 `XDr-P O_V1.0/1.1/1.2`、`XDr-S O_V1.0`（~1200 文件，CubeMX+HAL+编译产物）
- 保留 `series_P/O_V1.2`、`series_S/O_V1.1`

### 🔸 拼写修正
| 错误 | 改为 |
|------|------|
| `cur_fiter_alpha` | `cur_filter_alpha` |
| `speed_fiter_alpha` | `speed_filter_alpha` |
| `mech_offect` | `mech_offset` |
| `ClearFalg` | `ClearFlag` |
| `T_DEATH_us` | `T_DEADTIME_us` |

### 🔸 命名规范 TODO
在 5 个头文件添加 VESC 风格命名说明（后续直接改名）

---

## 阶段 2：固件架构优化

### 🔸 2.1 参数管理偏移量表
- `parameter_manager.c` 删除 `fParamSet`(120行)/`fParamGet`(160行) switch-case → 6 行 `PARAM_ENTRY` 表查（`offsetof`+`memcpy`）
- 固件 -816 bytes

### 🔸 2.2 ~ 2.3 模块变量 static 化
- `smo.h/c`: `g_smo` → static，新增 `smo_get_theta()`/`smo_get_omega()` getter
- `hfi.h/c`: `g_hfi` → static
- `svpwm.h/c`: `g_svpwm` → static
- `foc_main.h/c`: FOC_t 移除未使用的 `.hfi`/`.svpwm`/`.loop_con` 指针

### 🔸 2.4 多端口接收缓冲独立
- `port_mapping.c`：删除单 `is_busy` 全局锁，新增 CAN/USB/UART 各 128B 独立接收缓冲
- 回调仅做 `memcpy`+`pending` 标记，主循环轮询处理

---

## 阶段 3：通信改进

### 🔸 3.1 USB 递归重发改循环
- `usb_port.c`：`fUSB_SendData` 递归重发 → `for` 循环重试（最多 5 次，间隔 1ms）
- 修复清零语句在 return 之后不执行的 bug

### 🔸 3.2 状态推送改时间基准
- `port_mapping.c`: `no_response_tic++` → `BSP_GetTick()` 时间基准，5s 无心跳断开
- `com_port.py`: 移除收到状态后的冗余 `UC_CONNECT` 请求

### 🔸 3.3 上位机命令分发器
- `com_port.py`: `handle_received_data` 70 行 `match-case` → 20 行分发表派发
- 新增 `register_handler(cmd_id, callback)` 接口

### 🔸 3.4 USB/UART 帧格式自动切换
- `com_port.py`: 连接时根据端口类型自动选择 HEAD/TAIL

### 🔸 3.5 CRC8 校验
- `math_fast.h`: 新增 `crc8()`/`crc8_update()`（多项式 0x07）
- 固件 USB/UART 收发、上位机打包/解包均改用 CRC8

---

## 阶段 4：无感改进

### 🔸 4.1 SMO + PLL
- `smo.h/c`: 新增 `tSmoPll` + `smo_pll_init()`/`smo_pll_update()`
- Type-1 PLL：角度速度联合估计，无附加 LPF
- `SMO_USE_PLL 1/0` 宏切换

### 🔸 4.2 增益自适应改基于电压
- `smo.c`: 原基于估计速度 → 基于电压模长（BEMF 信号强度）
- `SMO_GAIN_BY_DUTY 1/0` 宏切换

### 🔸 4.3 HFI + SMO 融合
- `foc_core.c`: 三段式融合：<300RPM HFI / 300~450 线性 / >450 SMO

### 🔸 4.4 开环启动状态机
- `foc_main.h/c`: 新增 `FOC_OPENLOOP` 状态
- 锁定(200ms)→斜坡(500ms)→匀速→SMO 收敛后切闭环

### 🔸 4.5 滤波 Hz 单位配置
- `usr_config.h`: 新增 `CUR_LPF_HZ`(800Hz)/`SPEED_LPF_HZ`(50Hz)
- `_FilterInit` 从 Hz 自动计算 alpha

---

## 阶段 5：工程化提升

### 🔸 5.1 MIT 控制模式
- 新增 `mit.c/h`: 刚度Kp + 阻尼Kd + 前馈扭矩 (MIT 控制律)
- 协议枚举 `MIT_MODE`, 参数 `MIT_KP/KD/TFF/TMAX`

### 🔸 5.2 目录 `functons` → `functions`
- 重命名 + 5 个 Python import 更新

### 🔸 5.3 编码器芯片描述表
- `device.h`: `tEncoderChipDesc` 扩展 `use_dma_state_machine`/`dma_state_entry`/`dma_post_high_state`

---

## 阶段 6：批量重命名 + 帧格式统一

### 🔸 6.1 函数重命名 (VESC 风格)

| 旧名 | 新名 |
|------|------|
| `PI_init()`/`PI_update()`/`PID_init()` | `loop_pi_init()`/`loop_pi_update()`/`loop_pid_init()` |
| `fFocCoreInit()` | `foc_core_init()` |
| `fFocParamUpdate()` | `foc_param_update()` |
| `fFocCoreReset()` | `foc_core_reset()` |
| `fFocValueUpdate()` | `foc_value_update()` |
| `fFocMainLoopTask()` | `foc_main_loop_task()` |
| `fFocShutdown()` | `foc_shutdown()` |
| `fFocSetTargetValue()` | `foc_set_target()` |
| `fFocSetSensorMode()` | `foc_set_sensor_mode()` |
| `fFocSetRunMode()` | `foc_set_run_mode()` |
| `fFocSetUalphaBeta()` | `foc_set_ualpha_beta()` |
| `fFocSetIdIq()` | `foc_set_id_iq()` |
| `fFocSetZeroPos()` | `foc_set_zero_pos()` |
| `fFocSetLimitPos()` | `foc_set_limit_pos()` |
| `fSetThetaOffset()` | `foc_set_theta_offset()` |
| `fFilterReset()` | `filter_reset()` |

删除命名 TODO 注释块 6 处。

### 🔸 6.2 帧格式统一 0x55/0xAA
- `protocol/protocol.h/json/py`, `Software/protocol.py`, `com_port.py`: 全局统一
- `com_port.py`: 移除端口类型切换逻辑
- Bootloader 同步更新
- **编译修复**: loop_control.h 补回函数声明, device.h use_dma_state_machine 类型修正, foc_main.c 加 `#include smo.h`

---

## 阶段 P0：协议同步 + 编码器函数指针 + MT6835 支持

### 🔸 P0.1 协议生成同步
- `protocol.json`: 添加 `FOC_OPENLOOP` 枚举，`FOC_SHUTDOWN` 值=9
- 重新运行 `gen_protocol.py`，所有 `.h`/`.py` 文件一致

### 🔸 P0.2 编码器 switch/DMA 改函数指针
- `device.h`: 新增 `dma_post_high_state` 字段
- `encoder.c`: `fEncoderMainLoopTask` 的 `switch(chip_type)` → `chip_desc->dma_state_entry(&g_encoder)`
- `encoder.c`: `BSP_Encoder_SPI_TxRxCpltCallback` 移除 `if(chip_type==MT6816)/else(AS5047)`
  → 通用 `dma_post_high_state` 查表 + 通用 WAIT_LOW 处理
- `MT6816_MainLoop`/`AS5047_MainLoop` 签名改为 `void *arg`（函数指针兼容）

### 🔸 P0.3 MT6835 支持
- 根据数据手册添加 MT6835（Magntek AMR, 21-bit, SPI Mode3, 8-bit 连续读角度）
- `encoder.c`: 新增 `MT6835_ParseAndCheck`/`MT6835_StartRead`/`MT6835_MainLoop`
- 连续读角度：5-byte DMA `[0xA0, 0x03, 0x00, 0x00, 0x00]`，接收 21-bit 角度
- 21-bit → 14-bit 输出，检测磁场报警状态 `STATUS[1]`
- 新芯片增加只需在 `chip_descs[]` 填表，无需改路由

---

## 文件改动统计

```
阶段 1:  1910 文件变更（1870 删除 + 40 修改），-1026125 行
阶段 2:  ~15 文件变更，+153/-340 行
阶段 3:  9 文件变更，+131/-100 行
阶段 4:  8 文件变更，+172/-62 行
阶段 5:  目录改名 + 5 文件
阶段 6:  20 文件变更，+190/-228 行
阶段 P0: 7 文件变更，+85/-69 行
```

## 当前固件体积

```
RAM:   13112 B (10.00%)
FLASH: 73448 B (7.23%)
```

---

## 阶段 P1：电流采样改进

### 🔸 P1.1 去中值滤波
- `bsp_adc.c`: 删除 `vBubbleSort`/`fMedianFilter`/`fMedianFilterInit` 整段代码及缓冲区
- `config.h`: 删除 `MED_FILTER_SIZE`

### 🔸 P1.2 VESC 式零点校准 + 空闲跟踪
- `bsp_adc.c`: `BSP_AdcCalibrateCurrent` 改用一阶 LPF 递推 (k=0.01)，收敛 300 次
- 新增 `BSP_AdcIdleTrack()`: 电机 IDLE 时慢速 LPF (k=0.002) 跟踪零漂
- `foc_main.c` FOC_IDLE 状态中调用 `BSP_AdcIdleTrack()`

### 🔸 P1.3 2-shunt 上溢中断采样 (省一路 ADC + 重构查表)
- `bsp_adc.h/c`: 新增 `BSP_SampleCurrent2Shunt(sector, *ialpha, *ibeta)`
  - 根据扇区确定最短导通相，两相直采 + 第三相推导 (Ia+Ib+Ic=0)
  - 直接做 Clarke 变换输出 Ialpha/Ibeta
- `bsp_pwm.c`: 上溢 ISR 调用弱函数 `BSP_CurrentSampleISR()`
- `foc_main.c`: 实现 `BSP_CurrentSampleISR` → 调用 `BSP_SampleCurrent2Shunt`
- `foc_core.c`: `_CurrentReconstruction` 简化——直接用 ISR 预计算的 Ialpha/Ibeta

### 🔸 P1.4 注释采样点调整
- `foc_core.c`: 两处 `fSamplePointCalibration()` 调用注释掉，函数保留待用

---

## 阶段 P2：编码器 PLL + SPI error rate

### 🔸 P2.1 PLL 角度/速度联合估计
- `device.h`: `tEncoderInstance` 新增 PLL 字段 (`pll_theta`/`pll_omega_rpm`/`pll_kp`/`pll_ki`/`pll_integ`)
- `encoder.c`: 新增 `Encoder_PllUpdate()` — Type-1 PLL，临界阻尼增益 (kp=160, ki=kp²/4=6400)
- `Common_AngleVelocityUpdate`: PLL 初始化 + 每周期更新
- `fGetEncoderRPM`: 优先返回 PLL 速度 (>1rpm 时)，否则 fallback 位置差分
- 保留累计圈数 (`num_turns`) 不变

### 🔸 P2.2 SPI error rate 统计
- `device.h`: `tEncoderInstance` 新增 `spi_error_rate` (指数移动平均)
- 校验失败: `spi_error_rate += (1 - rate) * 0.01`
- 成功: `spi_error_rate *= 0.999`

## 文件改动统计

```
阶段 1:  1910 文件变更（1870 删除 + 40 修改），-1026125 行
阶段 2:  ~15 文件变更，+153/-340 行
阶段 3:  9 文件变更，+131/-100 行
阶段 4:  8 文件变更，+172/-62 行
阶段 5:  目录改名 + 5 文件
阶段 6:  20 文件变更，+190/-228 行
阶段 P0: 7 文件变更，+85/-69 行
阶段 P1: 6 文件变更，+74/-142 行
阶段 P2: 2 文件变更，+63/-11 行
```
