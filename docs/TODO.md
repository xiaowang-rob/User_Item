# TODO.md — 待办事项与改进计划

## CI/CD 持续集成

推荐使用 GitHub Actions，每次 push 自动执行编译检查。

### 配置模板 `.github/workflows/build.yml`

```yaml
name: Build

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Install ARM GCC
        run: |
          sudo apt-get update
          sudo apt-get install -y gcc-arm-none-eabi cmake

      - name: Build APP
        run: python3 tools/build.py build --board=xdr_p_o1.2 --type=app

      - name: Build BL
        run: python3 tools/build.py build --board=xdr_p_o1.2 --type=bl
```

### 可扩展的 CI 能力

| 功能 | 说明 |
|------|------|
| 自动构建 APP + BL | 验证代码是否可编译 |
| 固件大小报告 | 输出 text/data/bss 变化趋势 |
| 构建警告统计 | 追踪代码质量 |
| 多板构建 | xdr_p + xdr_s 同时编译 |
| Release 发布 | 打 tag 时自动生成固件产物 |

---

## 单元测试

### 推荐方案：Ceedling + CMock

Ceedling 是嵌入式 C 项目的单元测试框架，配合 CMock 可自动生成 mock 对象。

```
tests/
├── test_foc_core.c       # FOC 核心算法测试
├── test_loop_control.c   # PID 控制器测试
├── test_svpwm.c          # SVPWM 计算测试
├── test_filter.c         # 滤波器测试
├── test_trajectory.c     # 轨迹规划测试
└── test_math_fast.c      # 快速数学函数测试
```

### 测试策略

| 层级 | 测试范围 | 硬件依赖 | 优先级 |
|------|---------|---------|--------|
| **单元测试** | control/ utils/ 算法层 | ❌ 无 | ★★★★★ |
| **集成测试** | communication/ services/ | ⚠️ 模拟外设 | ★★★★ |
| **硬件在环** | 完整固件 | ✅ 需要硬件 | ★★ |

### 可测试的模块（无硬件依赖）

这些模块可以直接编写 host-side 单元测试：

- `app/control/foc_core.c` — 坐标变换（Clarke/Park）、状态机
- `app/control/svpwm.c` — SVPWM 扇区计算与占空比
- `app/control/smo.c` — 滑模观测器算法
- `app/control/loop_control.c` — PID 控制器
- `app/utils/filter.c` — 8 种滤波器实现
- `app/utils/math_fast.c` — 快速数学函数
- `app/utils/queue.c` — 环形队列
- `app/utils/trajectory.c` — 轨迹规划

---

## 待办改进项

| 优先级 | 事项 | 说明 |
|--------|------|------|
| ★★ | **S 系列移植** | board/xdr_s + platform/stm32g4xx 空骨架待填充 |
| ★★ | **`build.py flash` 烧录功能** | 已添加 flash/bf/erase 命令，需实际验证 OpenOCD 配置 |
| ★ | **README 更新** | 当前 README 仍描述旧结构，需同步更新 |
| ★ | **CI 配置** | 添加 GitHub Actions 自动构建 |
| ★ | **单元测试** | 对 control/ utils/ 算法层添加 host-side 测试 |
| ★ | **VSCode 调试** | launch.json 已创建，需安装 cortex-debug 插件并验证 |
| ☆ | **固件大小趋势追踪** | 每次构建记录 .text/.data/.bss 大小 |
| ☆ | **多语言支持** | 上位机 UI 当前仅中文，可添加英文支持 |
| ☆ | **CAN 上位机** | PC 无原生 CAN，需 CANable 等 USB-CAN 工具支持 |

---

## 已完成

| 事项 | 完成时间 |
|------|---------|
| app/ 应用层迁移 | ✅ |
| BL 迁移（共享 board + platform） | ✅ |
| BL USB 接收回调修复（`bsp_usb_recv_byte`） | ✅ |
| 统一 main 入口（条件编译 APP/BL） | ✅ |
| USB VID/PID 条件编译 | ✅ |
| board_config.h 配置归一化 | ✅ |
| CMake + build.py 构建系统 | ✅ |
| build.py 添加 flash/bf/erase 命令 | ✅ |
| .vscode/ 调试+任务配置（tasks/launch/settings） | ✅ |
| SVD 文件移至 platform/ | ✅ |
| **hal/ → board/bsp/ 重构 → 最终删除 board/bsp/** | ✅ |
| **hal_*() → bsp_*() 函数名统一** | ✅ |
| **bsp_base.h/.c 重命名（原 bsp.h/.c）** | ✅ |
| **清除所有重构引入的编译警告（31→0）** | ✅ |
| docs/ARCHITECTURE.md | ✅ |
| docs/TODO.md | ✅ |
