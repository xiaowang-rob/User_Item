# XDr_project

## 根目录

- `README.md` — 项目说明
- `core.drawio` — 项目架构图
- `XDrive.ioc` — CubeMX 生成的工程文件
- `USB_DEVICE` — CubeMX 生成的 USB 设备文件
- `tools` — 上位机工具
- `MDK-ARM` — MDK Keil 工程文件
- `Drivers` — 硬件抽象层（STM32CubeMX 生成）
- `Core` — 包含`main.c`核心代码（STM32CubeMX 生成）

## src/ — 源文件

### application/ — 应用层：用户交互与指令解析

- `app_main.c` — 应用主逻辑（调度控制/服务）
- `usb_interface.c` — USB CDC：调参、命令、DFU 触发、状态反馈、动态控制
- `can_interface.c` — CAN 动态控制
- `wireless_interface.c` — 串口无线监控（蓝牙/WiFi）
- `command_parser.c` — 统一命令解析器（支持多通道）

### services/ — 服务层：系统级功能

- `stream_transmission.c` — 流式传输服务（USB/CAN/串口）
- `protection_manager.c` — 过流/过压/过热/堵转等保护 分级故障处理
- `system_logger.c` — 日志管理（RAM 缓存 + FLASH 持久化）
- `status_feedback.c` — RGB LED 状态反馈
- `system_statemachine.c` — 系统级状态机,系统运行状态监测

### control/ — 控制层：FOC 与外环算法

- `foc_core.c` — 有感/无感 多模 FOC 主逻辑
- `mode_manager.c` — 多模式管理（电流/速度/位置）
- `auto_calibration.c` — 整定 + 编码器零位校准
- `adaptive_control.c` — 自适应参数（温度补偿、负载自适应 PID、自适应参数更新、前馈控制、弱磁控制）
- `loop_control.c` — 三环分频控制

### drivers/ — 驱动层：直接调用 HAL 操作外设

- `svpwm.c` — SVPWM 生成（调用 HAL_TIM）
- `adcDr.c` — 电流、电压、温度采样（调用 HAL_ADC + DMA）
- `encoder.c` — 编码器（spi 接收 磁编）
- `canDr.c` — CAN 通信（调用 HAL_CAN）
- `usbDr.c` — USB CDC（调用 USBD_LL / CDC 类）
- `usartDr.c` — 串口（调用 HAL_UART）
- `flashDr.c` — FLASH 擦写（W25Q128 和 STM32 芯片）
- `rgb.c` — RGB LED（WS2812b，led灯）

### utils/ — 工具层：通用函数，无硬件依赖

- `math_fast.c` — 快速函数（查表 / CORDIC/DSP库）
- `ring_buffer.c` — 环形缓冲区
- `crc.c` — CRC 校验
- `filter.c` — 低通/陷波滤波器
- `state_machine.c` — 通用状态机框架（可选）
- `smo` — 无感smo观测器和smo整定算法

## inc/ — 头文件（结构与 src/ 一致）

- `application/`
- `control/`
- `services/`
- `drivers/`
- `utils/`

## config/ — 配置集中管理

- `base_parameters.h` — 基础外设配置
- `system_parameters.h` — 基于硬件平台的系统级参数配置

## protocol/ — 通信协议

- `usb_protocol.h`
- `can_protocol.h`
- `usart_protocol.h`

