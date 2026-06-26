# X-motor-Drive 工程架构说明

## 项目概述

X-motor-Drive 是基于 STM32 的电机驱动控制项目，支持 FOC（磁场定向控制）算法，具备 CAN/串口/USB 多通道通讯、IAP 固件升级、参数调优等功能。

---

## 目录结构

```
X-motor-Drive/
├── app/                    # 应用层 — FOC 控制算法、通讯、服务、工具
│   ├── app_main.c         # 应用入口（init + main-loop）
│   ├── control/            # FOC 核心：SVPWM, SMO, HFI, MIT, 自整定
│   ├── communication/      # 通讯：CAN/UART/USB 协议解析分发
│   ├── drivers/            # 外设驱动：编码器、Flash、LED
│   ├── services/           # 服务：参数管理、保护、故障日志、状态反馈
│   └── utils/              # 工具：滤波器、快速数学、环形队列、轨迹规划
│
├── bl/                     # Bootloader（USB CDC IAP 固件升级）
│
├── board/                  # 板级层 — 硬件参数 + BSP 实现
│   ├── xdr_p/              # XDr-P 板卡（STM32F405）
│   │   ├── bsp/            #   BSP 接口+实现：bsp_base, adc, pwm, can, uart, usb, spi, flash, led, rgb
│   │   ├── usb/            #   USB 描述符（VID/PID 条件编译）
│   │   ├── board_config.h  #   硬件引脚映射 + Flash 分区 + 控制参数（单点真实）
│   │   ├── *.ld            #   链接脚本（APP + BL）
│   │   └── startup_*.s     #   启动文件
│   └── xdr_s/              # XDr-S 板卡预留（STM32G4，待实现）
│
├── platform/               # 平台层 — 芯片 SDK
│   ├── stm32f4xx/          # STM32F4 系列
│   │   ├── Drivers/        #   CMSIS + HAL 库
│   │   ├── Middlewares/    #   USB 设备库 + ARM DSP 库
│   │   ├── core/           #   CubeMX 外设初始化（统一 main.c 入口）
│   │   ├── USB_DEVICE/     #   USB CDC 设备适配
│   │   ├── STM32F405.svd   #   SVD 调试描述文件
│   │   └── CMakeLists.txt  #   平台库构建
│   └── stm32g4xx/          # STM32G4 系列（预留）
│
├── protocol/               # 通讯协议定义（代码生成）
│   ├── protocol.json       #   单点真实来源
│   ├── gen_protocol.py     #   代码生成器
│   ├── protocol.h          #   C 头文件（app + bl 使用）
│   └── protocol.py         #   Python 模块（上位机使用）
│
├── Software/               # 上位机软件
│   ├── start.py            #   启动入口
│   ├── UI/                 #   GUI 页面
│   ├── functions/          #   业务逻辑（串口、示波器、参数、IAP）
│   └── siui/               #   第三方暗色主题组件库
│
├── tools/                  # 构建与开发工具
│   ├── build.py            #   统一构建脚本（build/flash/bf/erase/clean）
│   ├── build_core.py       #   CMake 编译核心
│   ├── flash_core.py       #   OpenOCD 烧录/擦除
│   ├── codegen.py          #   协议代码生成
│   ├── filter_coeffs.py    #   巴特沃斯滤波器系数计算
│   ├── generate_linker_script.py  # 链接脚本自动配置
│   └── config_parser.py    #   配置文件解析
│
├── docs/                   # 文档
│   ├── ARCHITECTURE.md     #   ← 本文档
│   └── TODO.md             #   待办事项
│
├── .vscode/                # VSCode 调试/任务配置
│   ├── tasks.json          #   编译/烧录/擦除快捷按钮
│   └── launch.json         #   STLink/JLink/DAPLink 调试
│
├── Firmware/               # 旧版结构（保留参考）
├── demo/                   # 示例文档
├── firmware_out/           # 编译产物输出目录
├── build/                  # CMake 构建中间文件
├── CMakeLists.txt          # 顶层构建配置
├── project.json            # 当前项目配置（板卡/固件类型/调试器）
├── README.md               # 项目简介
└── REFACTOR_PLAN.md        # 重构记录
```

---

## 三层架构

```
┌──────────────────────────────────────────┐
│         app/    应用层                    │  ← 纯算法，与硬件无关
│  FOC 控制 / 通讯 / 服务 / 工具            │     通过 bsp_* 接口访问硬件
├──────────────────────────────────────────┤
│        board/xdr_p/bsp/  板级 BSP 层      │  ← 接口声明 + 实现合一
│  bsp_adc.h/.c / bsp_pwm.h/.c / ...       │     直接调用 STM32 HAL
│  bsp_base.h/.c (系统基础: timer/irq/delay) │
├──────────────────────────────────────────┤
│       platform/stm32f4xx/  平台 SDK       │  ← ST 官方代码，不动
│  HAL / CMSIS / USB / DSP                 │
└──────────────────────────────────────────┘
```

### 依赖方向

```
app/  →  board/xdr_p/bsp/ (接口+实现)  →  platform/ (SDK)
  ↓
protocol/ (app + bl 共享类型系统)
```

- `app/` 和 `bl/` 直接包含 `board/xdr_p/bsp/bsp_*.h` 头文件
- 每个 `.c` 文件按需包含自己用到的 BSP 头文件（不需要 umbrella 头文件）
- `platform/` 只被 `board/` 层调用
- 没有跨层或循环依赖

---

## 编译构建

### 命令行

```bash
# 编译 APP
python3 tools/build.py build --board=xdr_p_o1.2 --type=app

# 编译 Bootloader
python3 tools/build.py build --board=xdr_p --type=bl

# 烧录（最新编译的固件）
python3 tools/build.py flash

# 编译并烧录
python3 tools/build.py bf

# 擦除芯片
python3 tools/build.py erase

# 清理构建目录
python3 tools/build.py clean
```

### VSCode 快捷按钮

在 `.vscode/tasks.json` 中定义了以下任务（Ctrl+Shift+P → Tasks: Run Task）：

| 按钮 | 功能 |
|------|------|
| 编译 | 编译 APP 固件 |
| 重编译 | 清理后重新编译 |
| 烧录 | 仅烧录 |
| 编译并烧录 | 编译完成后自动烧录 |
| 擦除 | 擦除芯片 |
| 清理 | 删除 build/ 目录 |

### 调试

需要安装 Cortex-Debug 插件，在 `.vscode/launch.json` 中配置了三种调试器：

| 配置 | 调试器 | 服务器 |
|------|--------|--------|
| STLink | ST-Link/V2 | OpenOCD |
| JLink | SEGGER J-Link | J-Link GDB Server |
| DAPLink | CMSIS-DAP | OpenOCD |

---

## 通讯架构

### 通道分工

| 通道 | 用途 | 方向 | 说明 |
|------|------|------|------|
| **CAN** | 驱动器间控制 | 主控制器 ↔ 驱动器 | **主要控制方式**，通过其他控制器（如 PLC/上位控制器）发送 CAN 指令控制驱动器 |
| **USB CDC** | 上位机调试/刷参 | PC ↔ 驱动器 | 参数读写、实时数据示波器、IAP 固件升级 |
| **UART** | 上位机调试 | PC ↔ 驱动器 | 与 USB CDC 功能相同，作为备用通道 |

### 协议格式

所有通道共享同一协议（定义在 `protocol/protocol.json`）：

```
帧格式: [0x55] [CMD_ID] [LEN] [DATA...] [CRC8] [0xAA]
```

| 字段 | 长度 | 说明 |
|------|------|------|
| HEAD | 1B | 0x55 帧头 |
| CMD_ID | 1B | 命令 ID（23 个命令） |
| LEN | 1B | 数据负载长度 |
| DATA | N B | 负载数据（≤128B） |
| CRC8 | 1B | CRC8/ATM (poly=0x07) 校验 |
| TAIL | 1B | 0xAA 帧尾 |

### 命令分类

| 范围 | 功能 | 示例 |
|------|------|------|
| 0x01-0x04 | 参数操作 | 参数读写/保存/擦除 |
| 0x21-0x27 | 运行控制 | 模式切换、目标值设定、数据流控制 |
| 0x30-0x35 | IAP 升级 | 进入/擦除/写入/校验/退出 |
| 0xF0-0xF8 | 连接与杂项 | 连接心跳/断开/刹车/复位/日志 |

### 协议代码生成

```
protocol/protocol.json  (单点真实)
        ↓ gen_protocol.py
┌─────────────────┬────────────────────┐
│ protocol.h (C)  │ protocol.py (Python)│
│ (app + bl 使用)  │ (上位机使用)        │
└─────────────────┴────────────────────┘
```

---

## IAP 固件升级流程

```
上位机                     驱动器
  │                         │
  ├── CMD_IAP_ENTER ──────► │ 设置升级标志
  ├── CMD_SYSTEM_RESET ───► │ 重启进入 BL
  │                         │
  │  (Bootloader 启动)      │
  │                         │
  ├── UC_CONNECT ──────────►│ 返回固件信息
  ├── CMD_IAP_ERASE ──────► │ 擦除 APP Flash
  ├── CMD_IAP_WRITE ──────► │ 逐块写入（44B/块）
  ├── CMD_IAP_VERIFY ─────► │ 校验写入数据
  ├── CMD_IAP_EXIT ───────► │ 清除升级标志 → 跳转 APP
  │                         │
  │  (APP 启动运行)         │
```

---

## BSP 层说明

BSP（板级支持包）统一在 `board/<board_name>/bsp/` 目录下，头文件和实现文件放在一起：

| 文件 | 功能 | 接口函数示例 |
|------|------|-------------|
| `bsp_base.h/.c` | 系统基础：类型定义、中断、定时器、延时、复位 | `bsp_get_tick()`, `bsp_delay()`, `bsp_enable_irq()` |
| `bsp_adc.h/.c` | 三相电流采样、母线电压、温度、偏置校准 | `bsp_adc_get_current()`, `bsp_adc_calibrate_current()` |
| `bsp_pwm.h/.c` | 三相 PWM 输出、12V 电源控制、FOC 中断回调 | `bsp_pwm_set_compare()`, `bsp_foc_it_callback()` |
| `bsp_can.h/.c` | CAN 总线初始化、发送、配置 | `bsp_can_init()`, `bsp_can_send_data()` |
| `bsp_uart.h/.c` | UART DMA 收发 | `bsp_uart_transmit_dma()`, `bsp_uart_rx_callback()` |
| `bsp_usb.h/.c` | USB CDC 虚拟串口 | `bsp_usb_cdc_transmit_fs()`, `bsp_usb_recv_byte()` |
| `bsp_spi.h/.c` | 编码器 SPI + Flash SPI | `bsp_encoder_spi_transmit_receive_dma()`, `bsp_flash_spi_transmit()` |
| `bsp_flash.h/.c` | 内部 Flash 读写、参数存储、故障日志、IAP | `bsp_read_param()`, `bsp_flash_erase_app()`, `bsp_jump_to_app()` |
| `bsp_led.h/.c` | 状态指示灯（CAN 和编码器双 LED） | `bsp_led_can_toggle_pin()`, `bsp_led_encoder_set_pin()` |
| `bsp_rgb.h/.c` | WS2812 RGB LED 呼吸灯 | `bsp_rgb_breathe()`, `bsp_rgb_set_all_color()` |

### 设计原则

1. **接口与实现合一**：头文件和实现文件在同一目录，没有单独的接口抽象层
2. **按需包含**：每个 `.c` 文件只包含自己用到的 `bsp_*.h`，没有 umbrella 头文件
3. **app 和 bl 共享**：APP 和 BL 共用同一套 BSP，通过链接时选择不同 .c 文件实现差异
4. **多板支持**：新增板卡（如 `xdr_s`）只需在 `board/xdr_s/bsp/` 下实现同名接口函数

---

## 关键设计决策

1. **单点真实（SSoT）**：`board_config.h` 是硬件参数的唯一定义，`protocol.json` 是通讯协议的唯一定义
2. **代码生成**：协议代码由 `gen_protocol.py` 自动生成，避免手工同步错误
3. **统一 main 入口**：`platform/core/Src/main.c` 通过 `FW_TYPE` 条件编译同时服务 APP 和 BL
4. **条件编译 USB PID**：APP 和 BL 使用不同的 USB PID，上位机据此判断设备模式
5. **bootloader 无寄存器操作**：所有硬件操作通过 `bsp_*` 接口，与 APP 共享 BSP 层
6. **BSP 单一目录**：接口声明与实现在同一目录（`board/xdr_p/bsp/`），没有单独的抽象接口层
