```
.
├── bsp/                          # 底层硬件抽象层
│   └── series_P/                 # P系列硬件平台
│       └── O_V1.2/               # O版本V1.2原理图对应固件
│           ├── cubemx/           # STM32CubeMX生成代码
│           │   ├── Core/         # 主程序、外设驱动
│           │   ├── CMSIS/        # ARM CMSIS核心库
│           │   ├── Drivers/      # STM32F4xx HAL驱动
│           │   └── USB_DEVICE/   # USB CDC设备
│           ├── bsp_*.c           # 板级支持包（ADC/Flash/LED等）
│           ├── linker_*.ld       # Flash链接脚本(App/Bootloader)
│           └── CMakeLists.txt    # BSP构建配置
│   └── series_S/                 # S系列硬件平台（预留）
│
├── docs/                         # 文档
│   └── core.drawio               # 系统架构图
│
├── Software/                     # 上位机软件
│   ├── XDr-SiliconUI/            # SiliconUI新版调试工具
│   │   ├── UI/                   # 页面组件（参数/控制/IAP等）
│   │   ├── siui/                 # SiliconUI框架源码
│   │   ├── functons/             # 功能模块（串口/波形/数据分析等）
│   │   ├── configs/              # 配置文件
│   │   └── start.py              # 启动入口
│   └── XDr-pyqt5/                # PyQt5旧版调试工具
│       ├── QTdesigner/           # Qt Designer UI文件
│       ├── functions/            # 功能模块
│       ├── ui/                   # UI逻辑代码
│       ├── configs/              # 配置文件
│       └── start.py              # 启动入口
│
├── usr/                          # 用户应用程序（主控固件）
│   ├── app_main.c              # 应用程序入口
│   ├── bootload/               # Bootloader
│   │   └── bl_main.c
│   ├── foc/                      # 电机FOC控制算法
│   │   ├── communication/        # 通信协议（CAN/UART/USB）
│   │   ├── control/              # FOC核心（SVPWM/SMO/磁场定向/HFI）
│   │   ├── drivers/              # 硬件驱动（编码器/FLASH/LED）
│   │   ├── services/             # 服务层（日志/参数管理/保护/状态反馈）
│   │   └── utils/                # 工具函数（滤波/数学/环形队列/轨迹规划）
│   ├── cmake/                    # CMake交叉编译配置
│   ├── include/                  # 公共头文件
│   ├── tools/                    # 辅助脚本
│   ├── CMakeLists.txt            # 主工程构建配置
│   ├── project_config.json       # 工程配置
│   ├── usr_config.h              # 用户配置头文件
│   └── usr_config.json           # 用户配置JSON
│
├── .claude/                      # Claude Code 配置
├── .vscode/                      # VSCode 配置
├── .gitignore
└── README.md
```
