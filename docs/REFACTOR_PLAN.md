# X-motor-Drive 工程重构 — 最终状态

## 架构

```
app/ (算法)     bl/ (BL逻辑)
  ↓               ↓
usr_config.h (版本+计算参数)
  ↓ #include
board_config.h (所有硬件参数，单点真实)
  ↓ #include
board/xdr_p/bsp/ (BSP实现 + hal_宏别名)
  ↓ 依赖
platform/stm32f4xx/ (芯片SDK)
```

## 目录

```
X-motor-Drive/
├── app/                     # 应用层 (50 files)
├── bl/                      # BL (2 files: bl_main.c, usr_config.h)
├── hal/                     # 抽象接口 (11 files, 纯头文件)
├── board/xdr_p/             # 板级 (所有硬件相关)
│   ├── bsp/                 # BSP 实现
│   ├── usb/usbd_desc.c      # USB 描述符 (VID/PID)
│   ├── board_config.h       # 硬件参数 (引脚、限制、控制分频、HFI...)
│   ├── startup_*.s          # 启动文件
│   ├── STM32F405XX_FLASH.ld # 链接脚本 (APP)
│   ├── STM32F405XX_BL.ld   # 链接脚本 (BL)
│   └── project_config.json  # 仅调试器配置
├── platform/stm32f4xx/      # 芯片 SDK (HAL/CMSIS/USB库)
├── protocol/                # 共享协议
├── tools/                   # 构建脚本
└── Firmware/                # 旧版 (保留参考)
```

## 配置分层

| 文件 | 内容 | 维护方式 |
|------|------|----------|
| `board_config.h` | 引脚映射、硬件限制、控制分频、HFI参数、Flash布局 | 手动，单点真实 |
| `usr_config.h` | 版本信息、计算参数(F_CURRENT)、滤波器系数 | 手工编写，include board_config.h |
| `project_config.json` | 调试器配置 | 手动，仅烧录用 |

## 构建

```bash
python3 tools/build.py build --board=xdr_p --type=app   # 40.6 KB
python3 tools/build.py build --board=xdr_p --type=bl    # 12.1 KB
python3 tools/build.py clean                            # 清除 build/
```

## 已完成

| 事项 | 状态 | 说明 |
|------|------|------|
| hal/ 接口头文件 | ✅ | 11个纯头文件 |
| platform SDK | ✅ | 仅芯片级代码 |
| board BSP | ✅ | 含 USB描述符/启动文件/链接脚本 |
| app 迁移 | ✅ | 50个文件 |
| BL 迁移 | ✅ | 复用 board+platform，无独立 main.c |
| **统一 main 入口** | ✅ | platform/main.c 条件编译 APP/BL |
| **USB VID/PID 条件编译** | ✅ | usbd_desc.c 中 #ifdef FW_TYPE_BL |
| **配置归一化** | ✅ | board_config.h 为单点真实 |
| **删除 build.py 自动生成** | ✅ | usr_config.h 改为手动维护 |
| **JSON 精简** | ✅ | 仅保留调试器配置 |
| CMake + build.py | ✅ | 带进度条和固件大小显示 |
| 警告数 | ✅ | APP 27 / BL 15（去除重复宏警告） |
| 清理旧工程 | ✅ | 4个Keil工程76M + codegraph 138M 已删 |

## 待完成

| 事项 | 优先级 |
|------|--------|
| S系列迁移 (series_S → platform/stm32g4xx + board/xdr_s) | ★★ |
| `build.py flash` 烧录功能 | ★★ |
| git 管理 + .gitignore | ★ |
