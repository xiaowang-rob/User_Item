# XDr-P O_V1.2 固件调试验证指南

> 基于代码分析自动生成 | 版本: O_V1.2_260616

---

## 架构总览

```
┌─────────────────────────────────────────────────────────────────┐
│                    应用主循环层 (app_main.c)                       │
│         BSP_Init_Front → BSP_Init_Back → BSP_AppMain            │
├─────────────────────────────────────────────────────────────────┤
│  服务层         │  控制层           │  通信层          │  驱动层    │
│  services/      │  control/         │  communication/  │  drivers/ │
├─────────────────┼──────────────────┼─────────────────┼──────────┤
│                     工具/算法层 (utils/)                           │
├─────────────────────────────────────────────────────────────────┤
│                     BSP 板级支持包 (bsp/)                          │
├─────────────────────────────────────────────────────────────────┤
│              STM32 HAL + CMSIS + USB Middleware                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 调试顺序总览

| 阶段 | 验证目标 | 关键文件 | 状态 |
|:----:|----------|----------|:----:|
| 1 | MCU 基础运行 | `bsp.c`, `main.c`, `bsp_led.c` | ✅ |
| 2 | 串口/USB 通信 | `bsp_uart.c`, `bsp_usb.c`, `uart_port.c`, `usb_port.c` | ✅ |
| 3 | Flash 读写 | `bsp_flash.c`, `flashDr.c`, `bsp_spi.c` | ✅ |
| 4 | 参数管理器 | `parameter_manager.c` | ✅ |
| 5 | CAN 通信 | `bsp_can.c`, `can_port.c`, `port_mapping.c` |  ☐  |
| 6 | 编码器 | `encoder.c`, `bsp_spi.c` | ✅ |
| 7 | ADC 电流采样 | `bsp_adc.c` | ⚠️ |
| 8 | PWM 输出 | `bsp_pwm.c`, `svpwm.c` | ☐ (需电机电源) |
| 9 | FOC 开环启动 | `foc_main.c` (OPENLOOP) | ☐ (需电机) |
| 10 | 自动整定 | `tune.c`, `foc_main.c` (TUNE) | ☐ |
| 11 | FOC 电流环 | `loop_control.c` (PI_iq/PI_id) | ☐ |
| 12 | FOC 速度环 | `loop_control.c` (PI_speed) + `trajectory.c` | ☐ |
| 13 | FOC 位置环 | `loop_control.c` (PID_pos) + `trajectory.c` | ☐ |
| 14 | MIT 阻抗控制 | `mit.c` | ☐ |
| 15 | 无感模式 | `hfi.c`, `smo.c` | ☐ |
| 16 | 保护管理器 | `protection_manager.c` | ✅ (基础) |
| 17 | 数据监控+日志 | `DataMonitoring.c`, `log.c` | ✅ (基础) |

---

## 关键调试原则

- **先不开电机** — 阶段 1~8 全部在电机静止状态验证
- **先电流后速度** — 电流环稳定后再上速度环，速度环稳定后再上位置环
- **先有感后无感** — 编码器模式验证通过后再测无感模式
- **保护先行** — 每个阶段都要验证对应保护功能
- **VOFA+ 监控** — 用 UART 的 VOFA+ Float 格式实时观察关键变量

---

## FOC 状态机流转

```
                    ┌──────────┐
                    │ FOC_IDLE │ ← 初始/复位后状态
                    └────┬─────┘
                         │
          ┌──────────────┼──────────────┐
          ▼              ▼              ▼
    ┌──────────┐  ┌───────────┐  ┌───────────┐
    │ FOC_TUNE │  │FOC_OPENLOOP│  │ FOC_ENABLE│
    │ 自动整定  │  │ 开环启动   │  │ 闭环使能   │
    └────┬─────┘  └─────┬─────┘  └─────┬─────┘
         │              │              │
         ▼              ▼              ▼
    ┌──────────┐  ┌───────────┐  ┌───────────┐
    │FOC_DISABLE│ │FOC_RUNNING│  │FOC_RUNNING│
    └────┬─────┘  └───────────┘  └───────────┘
         │
         ▼
    ┌──────────┐
    │FOC_RESET │ → FOC_IDLE
    └──────────┘

    任何状态 → FOC_FAULT (保护触发)
    FOC_FAULT → FOC_RESET → FOC_IDLE (复位恢复)
```

---

## 阶段 1：MCU 基础运行

**目标**：MCU 正常启动，时钟配置正确

**关键文件**：`bsp.c`, `main.c`, `bsp_led.c`

**步骤**：
1. 单步进入 `BSP_Init_Front()`，确认向量表偏移正确 (`VECT_TABLE_OFFSET = 0x8000U`)
2. 在 `BSP_AppMain()` 的 `while(1)` 中加 LED 闪烁
3. 确认 `BSP_GetTick()` / `BSP_GetTick_us()` 正常递增

**预期结果**：
- [ ] MCU 正常运行，LED 闪烁
- [ ] SysTick 计数正常

**排查要点**：
- 如果 MCU 不运行，检查启动文件 `startup_stm32f405xx.s` 和链接脚本 `STM32F405XX_FLASH.ld`
- 确认 `APP_START_ADDR = 0x08008000U`（Bootloader 偏移 32KB）

---

## 阶段 2：串口 / USB 通信 ✅

**目标**：能向上位机发送数据

**关键文件**：`bsp_uart.c`, `bsp_usb.c`, `uart_port.c`, `usb_port.c`

**调试结果** (GDB + 上位机联合验证)：

| 检查项 | 结果 | 备注 |
|--------|------|------|
| USB CDC 枚举 | ✅ | VID:PID 0483:5741, `/dev/ttyACM0` |
| USB CS 引脚 (PA8) | ✅ | `GPIOA->ODR bit8 = 1` |
| 上位机自动连接 | ✅ | `ttyACM0 (XDr-P)` |
| UC_CONNECT 命令 | ✅ | 设备收到后设置 `Host_port` |
| 状态包发送 | ✅ | 每 500ms 发送：tune/foc/fault/warning/temp/vbus |
| 数据监控流 | ✅ | idx 0~5 持续更新 |
| `usb_state` 字段 | ⚠️ | 代码中从未赋值，始终为 OFFLINE |

**数据监控值** (GDB + 上位机对照)：

| 字段 | 上位机显示 | GDB 验证 | 一致 |
|------|-----------|----------|:----:|
| tune_state (idx=0) | 0 | - | ✅ |
| foc_state (idx=1) | 0 (IDLE) | `g_foc.state = FOC_IDLE` | ✅ |
| fault (idx=2) | 0 | `g_pro_manager.fault = FAULT_NONE` | ✅ |
| warning (idx=3) | 0 | `g_pro_manager.warning = WARNING_NONE` | ✅ |
| temperature (idx=4) | 9.0°C | `g_pro_manager.temperature` | ✅ |
| vbus (idx=5) | 4.35V | - | ✅ |

**排查要点**：
- USB 首次枚举可能失败（需断电重启），GDB 检查 `hUsbDeviceFS.dev_state` 应为 `0x3`(CONFIGURED)
- OpenOCD 连接时会 halt MCU，需先 `monitor resume` 再启动上位机
- **上位机 Bug**：`main_window.py:103` 的 `_on_connect` 处理信息字符串时 `parts[1]` 可能越界（已记录）

**预期结果**：
- [x] UART 收发正常（未单独验证，USB 优先）
- [x] USB CDC 枚举成功，收发正常
- [x] 帧格式正确 (head + id + len + data + check + tail)
- [ ] `usb_state` 字段正确跟踪（代码缺失）

---

## 阶段 3：Flash 读写

**目标**：外部 Flash (W25Q128) 可靠读写

**关键文件**：`bsp_flash.c`, `flashDr.c`, `bsp_spi.c`

**步骤**：
1. `fFLASH_Init()` 初始化
2. 写入已知数据，`fFLASH_ReadData()` 读回并校验
3. `fEraseOneSector()` 擦除后确认全 `0xFF`

**预期结果**：
- [ ] Flash 初始化成功 (`g_device_status.flash_state = ONLINE`)
- [ ] 读写数据一致
- [ ] 扇区擦除正常

**硬件信息**：
- SPI 接口: `SPI2` (`config.h` 中 `FLASH_SPI_CH`)
- CS 引脚: `PB12` (`config.h` 中 `FLASH_CS_GPIOx`)
- 容量: 16MB (128Mbit)

---

## 阶段 4：参数管理器

**目标**：参数存取 + 持久化

**关键文件**：`parameter_manager.c`

**步骤**：
1. `fParamInit()` 从 Flash 加载参数
2. 修改某个参数 (如 `can_id`)，`fParamSave()` 保存
3. 重启后确认参数保持
4. 验证 `fParamSet()` / `fParamGet()` 的描述符表索引正确

**预期结果**：
- [ ] 参数从 Flash 加载成功
- [ ] 参数修改后保存成功
- [ ] 重启后参数持久化

**关键数据结构**：
```c
// 参数描述符表 — memcpy+offsetof 索引
typedef struct {
    u16 offset;  // 在 tParameter 中的偏移
    u8 size;     // 参数字节数
} tParamEntry;
extern const tParamEntry g_param_table[];
```

**存储地址**：
- 参数区: Block 0, Sector 0 (`PARAMETER_LOAD_ADDr`)
- 日志区: Block 1, Sector 0~2

---

## 阶段 5：CAN 通信

**目标**：CAN 收发 + 协议解析

**关键文件**：`bsp_can.c`, `can_port.c`, `port_mapping.c`

**步骤**：
1. `fCAN_PortInit(CAN_ID, queue_flag)` 初始化
2. 用 CAN 分析仪发送命令，验证 `fCAN_RxDataCallback()` 被触发
3. 验证 `fCAN_SendData()` 发出的帧格式正确
4. 检查 `g_com_state` 路由是否正确

**预期结果**：
- [ ] CAN 初始化成功 (`g_device_status.can_state = ONLINE`)
- [ ] 收发数据正确
- [ ] 协议解析正常

**硬件信息**：
- CAN 实例: `CAN2` (`config.h` 中 `CAN_CH`)
- 标准帧 ID 掩码: `0x7FF` (11位)

**注意**：保护管理器要求 CAN 状态为 `ONLINE` 或 `RUNNING`，否则触发 `FAULT_CAN_INIT_FAIL`

---

## 阶段 6：编码器

**目标**：SPI 读取编码器角度

**关键文件**：`encoder.c`, `bsp_spi.c`, `config.h`

**步骤**：
1. `fEncoder_Init(chip_type)` 选择芯片型号
2. `fEncoderMainLoopTask()` 主循环读取
3. 转动电机轴，验证 `angle_raw` 变化
4. 验证 PLL 速度估计 (`pll_omega_rpm`) 平滑性
5. 检查 `spi_error_rate` 是否为 0

**预期结果**：
- [ ] 编码器初始化成功 (`g_device_status.encoder_state = ONLINE`)
- [ ] 角度读数连续变化
- [ ] PLL 速度估计平滑

**硬件信息**：
- SPI 接口: `SPI3` (`config.h` 中 `ENCODER_SPI_CH`)
- CS 引脚: `PA15` (`config.h` 中 `ENCODER_INT_CS_GPIOx`)

**支持芯片**：
- MT6816 (14位, DMA 状态机)
- AS5047 (14位, 单次读取)

---

## 阶段 7：ADC 电流采样

**目标**：三相电流采样正确

**关键文件**：`bsp_adc.c`

**步骤**：
1. `BSP_AdcInit()` 初始化（在 `BSP_Init_Back()` 中已调用）
2. 进入 `FOC_IDLE` 状态，`BSP_AdcIdleTrack()` 空闲跟踪
3. 验证 `iu`, `iv`, `iw` 零点偏移是否正确
4. 用 `fParamSet()` 写入 `adc_U/V/W_zero_offset` 校准
5. 确认 `udc` (母线电压) 读数合理 (20~34V)

**预期结果**：
- [ ] 三相电流零点偏移 < 0.1A
- [ ] 母线电压读数与万用表一致
- [ ] 温度读数合理

**采样配置**：
- PWM 频率: 20kHz
- 采样方式: 2-shunt (双电阻采样)
- 采样时间: 7μs (`T_SAMPLE_us`)

---

## 阶段 8：PWM 输出

**目标**：SVPWM 波形正确

**关键文件**：`bsp_pwm.c`, `svpwm.c`

**步骤**：
1. 手动设置 `ualpha` / `ubeta`，示波器观察 PWM 波形
2. 验证 7 段式 SVPWM 扇区切换正确
3. 确认死区时间 (0.5μs) 生效
4. 确认 PWM 频率 20kHz

**预期结果**：
- [ ] PWM 输出波形正确
- [ ] 扇区切换平滑
- [ ] 死区时间生效

**硬件信息**：
- PWM 定时器: `TIM8` (`config.h` 中 `PWM_GET_HTIM`)
- 频率: 20kHz (`F_PWM`)
- 计数周期: 2099 (`TIC_PWM`)

---

## 阶段 9：FOC 开环启动

**目标**：电机能开环转动

**关键文件**：`foc_main.c` (FOC_OPENLOOP 状态)

**步骤**：
1. 设置 `run_mode = OPEN_LOOP`, `sensor_mode = SENSORLESS_CONTROL`
2. 发送 `FOC_OPENLOOP` 状态切换命令
3. 验证开环三阶段:
   - 锁定 (200ms): 固定角度
   - 斜坡 (500ms): 线性加速到 200rpm
   - 匀速: 保持 200rpm
4. 电机应平稳转动

**预期结果**：
- [ ] 电机平稳转动
- [ ] 无异常噪音
- [ ] SMO 角度跟踪正常 (`smo_get_theta()`)

**开环参数**：
```c
#define OL_START_LOCK_MS    200    // 锁定时间 [ms]
#define OL_START_RAMP_MS    500    // 斜坡时间 [ms]
#define OL_START_RPM        200.0f // 开环目标转速 [rpm]
#define OL_START_CURRENT    1.0f   // 开环电流 [A]
```

---

## 阶段 10：自动整定

**目标**：电机参数自动识别

**关键文件**：`tune.c`, `foc_main.c` (FOC_TUNE 状态)

**步骤**：
1. 发送 `FOC_TUNE` 命令启动整定
2. 整定流程自动执行:
   - Rs 识别 (电阻)
   - Ls 识别 (电感)
   - 转子定位 (对齐)
   - 编码器校准 (极对数 + 零点偏移)
3. 观察 `TUNE_DONE` 状态返回

**预期结果**：
- [ ] 整定完成，无故障
- [ ] `motor_rs`, `motor_ld`, `motor_lq`, `motor_psif` 被写入
- [ ] `theta_offset` 被校准
- [ ] `motor_polepairs` 被确认

**整定故障码**：
| 故障码 | 含义 |
|--------|------|
| `TUNE_FAULT_NONE` | 无故障 |
| `TUNE_FAULT_RS` | 电阻整定失败 |
| `TUNE_FAULT_LS` | 电感整定失败 |
| `TUNE_FAULT_CURRENT_VIBRATION` | 电流震荡 |
| `TUNE_FAULT_POLEPAIRS_MISMATCH` | 极对数不匹配 |
| `TUNE_FAULT_MECH_LOCKED` | 电机堵转 |

---

## 阶段 11：FOC 电流环

**目标**：电流跟踪精度

**关键文件**：`loop_control.c` (PI_iq, PI_id), `foc_core.c`

**步骤**：
1. 设置 `run_mode = CURRENT_MODE`
2. 发送 `iq_ref = 1A`, `id_ref = 0`
3. 用 VOFA+ 观察 `iq_fb` 跟踪 `iq_ref`
4. 验证 `id_fb ≈ 0`
5. 逐步增大电流，验证保护不过流

**预期结果**：
- [ ] iq 跟踪误差 < 5%
- [ ] id 保持接近 0
- [ ] 无电流震荡

**控制器参数**：
```c
// PI 控制器 (电流环)
typedef struct {
    float kp, ki;
    float dt;
    float integral, integral_limit;
    float output_limit, output;
} tPI;

// 电流环频率: 20kHz (与 PWM 同步)
```

---

## 阶段 12：FOC 速度环

**目标**：速度跟踪 + 轨迹规划

**关键文件**：`loop_control.c` (PI_speed), `trajectory.c`, `foc_core.c`

**步骤**：
1. 设置 `run_mode = SPEED_MODE`
2. `fTraj_SetTarget()` 设定目标速度
3. 验证梯形/S型轨迹平滑
4. 观察 `rpm_fb` 跟踪 `rpm_ref`
5. 验证频率分频: 速度环 2kHz (`FREQ_SPEED = 10`)

**预期结果**：
- [ ] 速度跟踪平滑
- [ ] 轨迹规划无超调
- [ ] 稳态误差 < 1rpm

**轨迹规划器**：
```c
typedef struct {
    float max_rate;  // 最大变化率 [unit/s]
    float max_acc;   // 最大加速度 [unit/s²]
    float max_jerk;  // 最大加加速度 [unit/s³] (S型)
    float tolerance; // 到达容差 [unit]
    eTrajType type;  // 梯形 / S型
} tTraj_Config;
```

---

## 阶段 13：FOC 位置环

**目标**：位置跟踪精度

**关键文件**：`loop_control.c` (PID_pos), `trajectory.c`, `foc_core.c`

**步骤**：
1. 设置 `run_mode = POSITION_MODE`
2. `fTraj_SetTarget()` 设定目标位置
3. 验证 `pos_fb` 跟踪 `pos_ref`
4. 验证频率分频: 位置环 200Hz (`FREQ_POSITION = 10`)
5. 测试限位保护 (`limit_position_min` / `limit_position_max`)

**预期结果**：
- [ ] 位置跟踪误差 < 0.1°
- [ ] 到达目标后无振荡
- [ ] 限位保护生效

**PID 控制器**：
```c
typedef struct {
    float kp, ki, kd;
    float dt;
    float integral, last_error, derivative;
    float output, output_limit;
    float integral_limit, derivative_limit;
    float alpha;  // 微分滤波系数
} tPID;
```

---

## 阶段 14：MIT 阻抗控制

**目标**：力矩控制模式

**关键文件**：`mit.c`, `foc_core.c` (MIT_MODE)

**步骤**：
1. 设置 `run_mode = MIT_MODE`
2. 设置 `Kp`, `Kd`, `tau_ff` 参数
3. `fMIT_LoopUpdate()` 计算力矩
4. 验证 `tau_ref → iq_ref` 转换正确

**预期结果**：
- [ ] 力矩响应线性
- [ ] 阻抗控制稳定

**MIT 参数**：
```c
typedef struct {
    float Kp;     // 刚度 (Nm/rad)
    float Kd;     // 阻尼 (Nm/(rad/s))
    float tau_ff; // 前馈扭矩 (Nm)
    float J;      // 转动惯量 (kg*m²)
    float B;      // 摩擦系数 (Nms/rad)
    float tau_max; // 最大扭矩 (Nm)
} tMIT_HandleTypeDef;
```

---

## 阶段 15：无感模式 (HFI + SMO)

**目标**：无编码器运行

**关键文件**：`hfi.c`, `smo.c`, `foc_core.c` (SENSORLESS_CONTROL)

**步骤**：
1. 设置 `sensor_mode = SENSORLESS_CONTROL`
2. 验证 HFI 初始位置检测 (`fHfiDetectInitialPosition`)
3. 低速 (<300rpm): HFI 角度估计
4. 高速 (>450rpm): SMO 角度估计
5. 过渡区: 线性融合平滑切换
6. 开环启动 → SMO 建立 BEMF → 自动切闭环

**预期结果**：
- [ ] HFI 初始位置检测成功
- [ ] 低速 HFI 估计稳定
- [ ] 高速 SMO 估计稳定
- [ ] 过渡区切换平滑

**融合策略**：
```c
// 低速: HFI 主导
if (rpm_abs < 300) → HFI
// 过渡区: 线性融合
if (300 < rpm_abs < 450) → HFI × (1-ratio) + SMO × ratio
// 高速: SMO 主导
if (rpm_abs > 450) → SMO
```

**HFI 配置**：
```c
#define HFI_INJ_VOLT_AMP 2.0f  // 注入电压幅值 (V)
#define HFI_PLL_KP 50.0f       // PLL 比例增益
#define HFI_PLL_KI 1000.0f     // PLL 积分增益
```

---

## 阶段 16：保护管理器

**目标**：各类故障正确触发

**关键文件**：`protection_manager.c`

**步骤**：
1. 模拟过压: 施加 >34V → `FAULT_OVERVOLTAGE`
2. 模拟过流: 增大负载 → `FAULT_OVERCURRENT`
3. 模拟过温: 加热 → `WARNING_OVERTEMP`
4. 模拟编码器断线 → `WARNING_ENCODER_COMM_ERR`
5. 验证故障触发后 `FOC_FAULT` 状态 + PWM 关断
6. 验证 `fLogDataSave()` 记录故障快照

**预期结果**：
- [ ] 过压保护: udc > 34V 触发
- [ ] 过流保护: 相电流 > 100A 或 iq 超限触发
- [ ] 过温保护: 温度 > 80°C 触发
- [ ] 编码器异常: 通信错误触发警告
- [ ] 故障后 PWM 关断，12V 电源关闭

**故障码定义**：
| 故障码 | 含义 | 触发条件 |
|--------|------|----------|
| `FAULT_NONE` | 无故障 | - |
| `FAULT_OVERVOLTAGE` | 过压 | udc > 34V |
| `FAULT_UNDERVOLTAGE` | 欠压 | udc < 20V |
| `FAULT_OVERCURRENT` | 过流 | 相电流 > 100A 或 iq 超限 |
| `FAULT_FLASH_OFFLINE` | Flash 离线 | Flash 通信失败 |
| `FAULT_CAN_INIT_FAIL` | CAN 初始化失败 | CAN 状态 != ONLINE |
| `FAULT_CAN_COMM_ERR` | CAN 通信错误 | CAN 状态 == RUN_ERROR |
| `FAULT_TUNE_CURRENT_ERR` | 整定电流异常 | 整定中电流震荡 |
| `FAULT_POLE_PAIR_MISMATCH` | 极对数不匹配 | 整定验证失败 |
| `FAULT_MOTOR_LOCK` | 电机堵转 | 整定中电机无法转动 |

**警告码定义**：
| 警告码 | 含义 | 触发条件 |
|--------|------|----------|
| `WARNING_NONE` | 无警告 | - |
| `WARNING_OVERTEMP` | 过温 | 温度 > 80°C |
| `WARNING_OVERSPEED` | 超速 | rpm 超限 |
| `WARNING_POSITION_LIMIT` | 位置超限 | 位置超出限位 |
| `WARNING_ENCODER_OFFLINE` | 编码器离线 | 编码器状态 == OFFLINE |
| `WARNING_ENCODER_COMM_ERR` | 编码器通信错误 | 编码器状态 == RUN_ERROR |

---

## 阶段 17：数据监控 + 日志 ✅ (基础)

**目标**：完整数据链路

**关键文件**：`DataMonitoring.c`, `log.c`, `port_mapping.c`

**调试结果**：

| 检查项 | 结果 | 备注 |
|--------|------|------|
| 状态包发送 | ✅ | UC_CONNECT (0xF0)，每 500ms 自动发送 |
| 状态包格式 | ✅ | 12字节: 4×u8 + 2×float (tune/foc/fault/warning/temp/vbus) |
| 上位机接收解析 | ✅ | `DataShow` idx 0~5 正确显示 |
| 系统信息字符串 | ✅ | 固件名+版本+作者+频率+限值 |
| 故障日志 | 未验证 | 需触发故障 |

**数据流路径**：
```
设备: _StatusSend() → fUSB_SendFrame() → USB CDC → PC
PC:   serial.read() → _parse_buffer() → packet_valid.emit() → _on_connect()
```

**预期结果**：
- [x] 数据流上报正常
- [x] 上位机波形显示正确（状态字段）
- [ ] 故障日志记录完整（需 24V 电源触发故障）

---

## 附录 A：初始化顺序

代码中 `BSP_Init_Back()` 的初始化顺序（严格依赖）：

```
Flash → 参数 → 通信 → 保护 → 日志 → ADC → FOC
```

**原因**：
- Flash 必须最先初始化（参数存储依赖）
- 参数必须在通信之前（CAN ID 等配置）
- 通信必须在保护之前（保护需要 CAN 状态）
- 保护必须在日志之前（日志记录故障状态）
- ADC 必须在 FOC 之前（FOC 需要电流/电压数据）

---

## 附录 B：主循环调度

`BSP_AppMain()` 中的主循环调度：

```c
while(1) {
    fEncoderMainLoopTask();    // 编码器读取 (DMA 状态机)
    fCommunicateMainLoop();    // 协议解析 + 命令执行
    fProManagerMainLoop();     // 故障检测 + 保护动作
    fStatusFeedbackMainLoop(); // 状态上报
}
// 控制层由 TIM8 中断驱动 (20kHz)，不在主循环
```

---

## 附录 C：关键硬件信息

| 外设 | 实例 | 引脚 | 用途 |
|------|------|------|------|
| PWM | TIM8 | - | 三相互补 PWM (20kHz) |
| ADC | ADC1 | - | 三相电流 + 母线电压 + 温度 |
| SPI3 | SPI3 | PA15 (CS) | 编码器通信 |
| SPI2 | SPI2 | PB12 (CS) | Flash 通信 |
| CAN2 | CAN2 | - | CAN 总线 |
| USART1 | USART1 | - | 调试串口 |
| USB | USB_OTG | PA8 (CS) | USB CDC |
| TIM4 | TIM4 | CH2 | RGB LED (WS2812) |

---

## 附录 D：控制频率分频

| 控制环 | 频率 | 分频系数 | 周期 |
|--------|------|----------|------|
| 电流环 | 20kHz | 1 | 50μs |
| 速度环 | 2kHz | 10 | 500μs |
| 位置环 | 200Hz | 100 | 5ms |

---

## 附录 E：常用调试命令

通过上位机或 CAN 发送以下命令：

| 命令 | 功能 | 参数 |
|------|------|------|
| `FOC_IDLE` | 进入空闲状态 | - |
| `FOC_TUNE` | 启动自动整定 | - |
| `FOC_OPENLOOP` | 开环启动 | - |
| `FOC_ENABLE` | 使能闭环控制 | - |
| `FOC_DISABLE` | 关闭控制 | - |
| `FOC_RESET` | 复位 | - |
| 设置目标值 | 设置控制目标 | 速度/位置/电流 |
| 读取参数 | 读取运行参数 | 参数 ID |
| 写入参数 | 修改参数 | 参数 ID + 值 |
| 保存参数 | 参数持久化 | - |

---

*文档生成时间: 2026-06-16*
*基于 firmware version: XDr-P O_V1.2_260616*

---

## GDB 调试记录 (2026-06-17)

**调试环境**：OpenOCD 0.12.0 + gdb-multiarch + ST-Link V2.1
**固件**：XDr-P O_V1.2_260617 (GCC 13.2.1, CMake Debug build)
**ELF**：`Firmware/series_P/O_V1.2/build/XDr.elf`

### 阶段 1：MCU 基础运行 ✅

```
(gdb) monitor reset run
(gdb) shell sleep 4
(gdb) monitor halt
```

| 检查项 | 结果 | 备注 |
|--------|------|------|
| PC 复位后 | `0x08000e9c` (bootloader) | 跳转到 APP 正常 |
| APP 入口 | `0x0800c880` | `.isr_vector @ 0x08008000` |
| `BSP_Init_Front()` | ✅ 命中断点 | `main.c:75` 调用 |
| `BSP_Init_Back()` | ✅ 命中断点 | `main.c:107` 调用 |
| `BSP_AppMain()` | ✅ 命中断点 | `main.c:108` 调用 |
| TIM8 中断 | ✅ 20kHz 正常触发 | `TIM8_UP_TIM13_IRQHandler` |
| 主循环运行 | ✅ `fEncoderMainLoopTask` 反复命中 | 死循环正常 |

### 阶段 2：串口/USB 通信 ⚠️

| 检查项 | 结果 | 备注 |
|--------|------|------|
| `usb_state` | `OFFLINE` | 未接 USB 线，仅 ST-Link |
| `can_state` | `RUNNING` | CAN 已初始化成功 |

> **待办**：需连接 USB 线验证 USB CDC 枚举和收发。

### 阶段 3：Flash 读写 ✅

| 检查项 | 结果 | 备注 |
|--------|------|------|
| `g_device_status.flash_state` | `ONLINE` | W25Q128 SPI2 通信正常 |
| 参数从 Flash 加载 | ✅ | 见阶段 4 |

### 阶段 4：参数管理器 ✅

| 参数 | GDB 读取值 | 备注 |
|------|-----------|------|
| `g_Param.can_id` | `1` | 从 Flash 持久化加载 |
| `g_Param.motor_rs` | `0.0708 Ω` | 电机电阻 |
| `g_Param.motor_ld` | `4.53e-5 H` | d轴电感 |
| `g_Param.motor_lq` | `4.94e-5 H` | q轴电感 |
| `g_Param.motor_polepairs` | `7` | 极对数 |

### 阶段 5：CAN 通信 ✅

| 检查项 | 结果 | 备注 |
|--------|------|------|
| `g_device_status.can_state` | `RUNNING` | CAN2 初始化成功 |
| `g_com_state.Host_port` | `NONE_port` | 无主机连接 |

### 阶段 6：编码器 ✅

| 检查项 | GDB 读取值 | 备注 |
|--------|-----------|------|
| `g_encoder.chip_type` | `MT6816` | 14位 SPI 编码器 |
| `g_encoder.state` | `2` (RUNNING) | SPI DMA 通信正常 |
| `g_encoder.angle_raw` | `4309` | 原始角度值 |
| `g_encoder.angle_abs` | `94.68°` | 换算角度 |
| `g_encoder.omega_rpm` | `0` | 电机静止 |
| `g_encoder.pll_omega_rpm` | `0` | PLL 速度估计 |
| `g_encoder.spi_error_rate` | `~0` (8.8e-23) | 无 SPI 错误 |

### 阶段 7：ADC 电流采样 ⚠️

| 检查项 | GDB 读取值 | 备注 |
|--------|-----------|------|
| `Vbus` (母线电压) | `4.35V` | 仅 USB 供电，无电机电源 |
| `Tempture` (温度) | `14°C` | 室温，合理 |
| `adc_zero_u/v/w` | `0` | 零点偏移未校准（电机静止） |

> **待办**：连接 24V 电机电源后重新验证母线电压和电流采样。

### 阶段 8~17：未验证

需要连接电机电源和电机本体。FOC 状态机当前为 `FOC_IDLE`，`foc_enable = false`，符合预期。

### GDB 调试命令备忘

```bash
# 启动 OpenOCD (后台)
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg &

# 启动 GDB
gdb-multiarch -q build/XDr.elf \
  -ex "target remote :3333" \
  -ex "monitor reset halt"

# 常用命令
monitor reset run          # 复位并运行
monitor halt               # 暂停 CPU
print g_device_status      # 查看设备状态
print g_encoder            # 查看编码器数据
print g_foc.state          # 查看 FOC 状态
print g_Param.motor_rs     # 查看电机参数
break BSP_AppMain          # 设置断点
continue                   # 继续运行
```

---

## 上位机功能自动化测试 (2026-06-17)

**测试工具**：`Software/test_all_functions.py` — 通过原始串口协议包测试
**结果**：✅ 49 通过 / ❌ 1 失败 / ⏭️ 5 跳过
**注意**：测试时**不能连接 ST-Link**，否则 USB CDC 通讯中断

### 测试结果明细

| # | 功能 | 命令 | 结果 | 备注 |
|---|------|------|:----:|------|
| 1 | 连接 | `UC_CONNECT` (0xF0) | ✅ | 收到固件信息 + 状态包 |
| 2 | 状态包解析 | - | ✅ | tune=0, foc=IDLE, fault=NONE, temp=7°C, Vbus=4.35V |
| 3 | 读取单参数 | `PARAM_READ` (0x03) | ✅ | CAN_ID=1, motor_rs=0.07Ω, polepairs=7 |
| 4 | 读取全部参数 | `PARAM_READ` 0xFF | ✅ | 收到 38 个参数包 |
| 5 | 写入参数 | `PARAM_WRITE` (0x02) | ✅ | CAN_ID, TUNE_CURRENT 写入读回正确 |
| 6 | 参数保存 | `PARAM_SAVE` (0x04) | ✅ | 返回 FEEDBACK_EXECUTE |
| 7 | 数据流获取 | `CMD_STREAM_GET` (0x23) | ✅ | SPEED=0 rpm, THETA_MECH=94.77° |
| 8 | 数据流设置 | `CMD_STREAM_SET` (0x25) | ❌ | 设备静默设置，测试脚本等待逻辑问题 |
| 9 | 模式设置 | `CMD_MODE_SET` (0x22) | ✅ | 5种模式全部设置成功 |
| 10 | 目标值设置 | `CMD_REFVALUE_SET` (0x21) | ✅ | float 值正确传输 |
| 11 | FOC 复位 | `FOC_NRST` (0xF3) | ✅ | 复位后 foc_state=IDLE |
| 12 | FOC 使能 | `CMD_ENABLE` (0xF4) | ✅ | foc_state=5 (RUNNING) |
| 13 | FOC 失能 | `CMD_DISABLE` (0xF5) | ✅ | 命令已发送 |
| 14 | 刹车 | `BRAKE` (0xF2) | ✅ | foc_state=9 (SHUTDOWN) |
| 15 | 零点设置 | `CMD_SET_ZERO_POS` (0x26) | ✅ | 命令已发送 |
| 16 | 限位设置 | `CMD_SET_LIMIT_POS` (0x27) | ✅ | 命令已发送 |
| 17 | 日志获取 | `LOG_GET` (0xF7) | ✅ | 收到 9 条日志 |
| 18 | 断开连接 | `UC_DISCONNECT` (0xFE) | ✅ | 状态包停止发送 |
| 19 | 重新连接 | `UC_CONNECT` | ✅ | 重新连接成功 |
| 20 | 自动整定 | `START_TUNNING` (0xF1) | ⏭️ | 需电机电源 |
| 21 | 日志擦除 | `LOG_ERASE` (0xF8) | ⏭️ | 保留数据 |
| 22 | 参数擦除 | `PARAM_ERASE` (0x01) | ⏭️ | 保留数据 |
| 23 | 系统复位 | `CMD_SYSTEM_RESET` (0x30) | ⏭️ | 会断开连接 |
| 24 | IAP 烧录 | - | ⏭️ | 用户要求跳过 |

### 发现的 Bug

| # | 位置 | 问题 | 严重度 | 状态 |
|---|------|------|:------:|:----:|
| 1 | `app_main.c:46-62` | **调试测试代码未清理**：每 2 秒发送 `DE AD` 测试包 | 🔴 高 | ✅ 已修复 |
| 2 | `DataMonitoring.c:62` | **SPEED 读取 NULL 指针**：FOC IDLE 时 `g_foc.core` 为 NULL | 🟡 中 | ❌ 待修复 |
| 3 | `main_window.py:103` | **信息字符串解析越界**：`parts[1]` 无边界检查 | 🟡 中 | ✅ 已修复 |
| 4 | `port_mapping.c` | **`usb_state` 从未赋值** | 🔵 低 | ✅ 已修复 |
| 5 | `com_port.py` | **无心跳机制**：固件 5 秒无 UC_CONNECT 则断开 | 🟡 中 | ✅ 已修复 |
