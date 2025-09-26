XDr_project/
│
├── README.md                       # 项目说明
│
├── src/                            # 源文件
│   │
│   ├── main.c                      # 系统入口：调用各模块 init + 主循环
│   │
│   ├── application/                # 应用层：用户交互与指令解析
│   │   ├── app_main.c              # 应用主逻辑（调度控制/服务）
│   │   ├── usb_interface.c         # USB CDC：调参、命令、DFU 触发
│   │   ├── can_interface.c         # CAN 动态控制
│   │   ├── wireless_interface.c    # 串口无线监控（蓝牙/WiFi）
│   │   └── command_parser.c        # 统一命令解析器（支持多通道）
│   │
│   ├── control/                    # 控制层：FOC 与外环算法
│   │   ├── foc_core.c              # 有感 SVPWM FOC 主逻辑
│   │   ├── mode_manager.c          # 多模式管理（电流/速度/位置）
│   │   ├── auto_calibration.c      # 电机参数辨识 + 编码器零位校准
│   │   ├── adaptive_control.c      # 自适应参数（温度补偿、负载自适应 PID）
│   │   ├── loop_control.c          # 环控制
│   │
│   ├── services/                   # 服务层：系统级功能
│   │   ├── system_monitor.c        # 实时数据流 + 内置示波器
│   │   ├── protection_manager.c    # 过流/过压/过热/堵转保护 分级故障处理
│   │   ├── system_logger.c         # 日志管理（RAM 缓存 + FLASH 持久化）
│   │   ├── status_feedback.c       # RGB LED 状态反馈
│   │
│   ├── drivers/                    # 驱动层：直接调用 HAL 操作外设
│   │   ├── pwm_driver.c            # SVPWM 生成（调用 HAL_TIM）
│   │   ├── adc_driver.c            # 电流采样（调用 HAL_ADC + DMA）
│   │   ├── encoder_driver.c        # 编码器（调用 HAL_TIM_Encoder）
│   │   ├── can_driver.c            # CAN 通信（调用 HAL_CAN）
│   │   ├── usb_cdc_driver.c        # USB CDC（调用 USBD_LL / CDC 类）
│   │   ├── uart_driver.c           # 串口（调用 HAL_UART）
│   │   ├── flash_driver.c          # FLASH 擦写（调用 HAL_FLASH）
│   │   ├── rgb_led_driver.c        # RGB LED（如 WS2812，用 TIM PWM 或 bit-bang）
│   │   └── temperature_driver.c    # 温度采样（NTC + ADC）
│   │
│   └── utils/                      # 工具层：通用函数，无硬件依赖
│       ├── math_fast.c             # 快速三角函数（查表 / CORDIC）
│       ├── ring_buffer.c           # 环形缓冲区
│       ├── crc.c                   # CRC 校验
│       ├── filter.c                # 低通/陷波滤波器
│       └── state_machine.c         # 通用状态机框架（可选）
│
├── inc/                            # 头文件（结构与 src/ 一致）
│   ├── application/
│   ├── control/
│   ├── services/
│   ├── drivers/
│   └── utils/
│
├── config/                         # 配置集中管理
│   ├── motor_parameters.h          # 电机参数（R, L, Ke, 极对数）
│   ├── control_tuning.h            # PID 参数、限幅、自适应开关
│   ├── protection_limits.h         # 保护阈值
│   ├── pin_mapping.h               # 引脚定义（与 CubeMX 一致）
│   └── system_config.h             # 系统开关（是否启用日志、无线等）
│
├── protocol/                       # 通信协议
│   ├── usb_protocol.h
│   ├── can_protocol.h
│   └── wireless_protocol.h
│
└── third_party/                    # 第三方库（可选）
    └── stm32g4xx_hal_driver/       # STM32 HAL 库（由 CubeMX 生成或手动添加）