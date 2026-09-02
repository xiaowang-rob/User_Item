# 分层架构总结（refactor 落地版 · 简洁）

> 这是 X-motor-Drive 重构目标架构的精简总结，以 `refactor/` 中**已落地代码**为基准。
> 目标一句话：**上层（业务）永远不碰厂商库（HAL/SPL/寄存器）与板级资源；换板只换 `board/<b>/hw` + 组装层。**

---

## 1. 分层

```
usr/ctl · app · srv        业务层（未来）：只 include usr/abs 头与 dev_board.h
        │
usr/abs                    业务对象：tEncoder / tLed / tRgb / 存储单元（纯逻辑，零硬件）
        │
usr/drv                    芯片协议驱动：编码器×3+引擎 / w25qxx / ws28xx（只认识接口表）
        │
usr/if                     接口契约：tSpiBusIf / tTimeIf / tPwmDmaIf（纯头，唯一跨库契约）
        ▲   （依赖倒置：接口归 usr，实现来自板）
        │
usr/app/dev_board.*        组装层（唯一同时 include hw+drv+abs），装配出全局对象 g_dev
        │
board/<b>/hw               板级适配：资源表 + 接口表实现（厂商库符号终结于此）
        │
board/<b>/Core + 厂商库    外设初始化（当前板 = CubeMX + HAL）
```

## 2. 目录（refactor/ 实际）

```
refactor/
├── CMakeLists.txt               构建入口（5 个库目标）
├── usr/
│   ├── if/      time_if.h  spi_if.h  pwm_dma_if.h        [纯头契约]
│   ├── abs/     device.h  encoder  led  flash(.h/.c)      [usr_abs 库]
│   ├── drv/     enc_spi_engine · as5047 · mt6816 · mt6835
│   │            w25qxx · ws28xx (+ *_drivers.h)            [usr_drv 库]
│   └── app/     dev_board.h/.c                             [usr_app 库]
└── board/xdr_p_o1.2/
    ├── hw/      hw_base · hw_enc_spi · hw_flash_spi
    │            hw_rgb_pwm · hw_led · hw_pinmap.h          [board_hw 库]
    └── Core/Drivers/Middlewares/…        CubeMX 原样副本
```

## 3. 每层职责

| 层 | 内容 | 关键约束 |
|---|---|---|
| usr/if | 接口结构体（函数指针表 + 不透明 `void* ctx`），参数只用 C 基础类型 | 头文件禁止 include 非标准头 |
| usr/abs | 电机控制业务语义：多圈/零位/PLL、闪烁/呼吸、日志式存储单元 | 无任何厂商符号 |
| usr/drv | 芯片协议：命令字、时序、校验、解析；**同步**读为主 | 只 include usr/if + usr/abs 类型头 |
| usr/app/dev_board | 选芯片 + 选板级资源 → create(bus,time) → abs init → 全局 `g_dev` | 唯一同时见 hw/drv/abs |
| board/hw | 把 HAL/CMSIS 装进接口表；引脚映射、中断回调、句柄只在此 | 唯一允许厂商库符号；反向 include usr/if 是实现者 |

## 4. 依赖铁律

1. `usr/abs`、`usr/drv`：禁止厂商库符号与板级头（用 grep 可查）。
2. drv 的硬件能力全部来自 **create 时注入的接口表**——drv 不 include 板级头，想误调也没声明。
3. `board/hw` 是厂商库符号终结层：句柄宏与 `HAL_*Callback` 只出现在 `hw/*.c`。
4. 唯一反向依赖例外：hw 为实现 usr/if 而 include `usr/if/*.h`。
5. 业务层不自行 create/init；装配集中在 dev_board。

## 5. 核心机制（一句话）

- **接口 = 语义，不泄漏实现**：例如编码器 ops 是 `read_angle(&raw,&ts)` 一次带时间戳读数，不是 `start_read/is_data_ready/set_cs`（异步 DMA 时代产物）。
- **资源注入**：芯片驱动不知道"内部/外部 CS、SPI3、PA15"——`create` 只收接口表，hw 为每条 CS 造一个接口实例。
- **模式是协议事实**：SPI 的 CPOL/CPHA/位宽由芯片驱动在 init 时经 `set_mode` 声明，hw 执行——同总线换芯片也能切回正确模式。

## 6. 关键设计决策

| 决策 | 结果 |
|---|---|
| 取消 bsp | 原协议封装层消解；资源/适配进 `board/<b>/hw` |
| 同步为主 | 编码器等轮询读数同步 SPI；ADC/WS2812 作异步例外（能力收敛在各自接口表，中断 hw 自持） |
| 跨厂商库 | usr/if 是唯一跨库契约；未来标准库(SPL)板只需为接口表新写一套 hw 实现 |
| 组装层 | `dev_board_init()`：`hw_base_init` → 时间 → 灯/闪存（容错）→ 编码器（关键） |

## 7. 落地状态

构建（`cmake --build refactor/build`）零 error；`usr/drv`、`usr/abs` 还能用 host gcc 直接编译（零厂商依赖的证据）。

```
usr_if   (INTERFACE, 纯头)   usr_abs(业务)   usr_drv(协议)   usr_app(装配 g_dev)   board_hw(适配)
```

设备装配：`g_dev.enc`（MT6816 默认，可切 AS5047/MT6835）、`g_dev.led_can/led_enc`、
`g_dev.rgb`（WS2812）、`g_dev.ext_flash`（W25Q128 存储单元，芯片缺失时容错为不可用）。

## 8. 尚未完成（后续方向）

- 高层（foc/服务/通讯）迁移到 `g_dev` —— 当前 refactor 只到装配层；
- 编码器业务接入控制环（`encoder_update` 的调用点）；
- `board_config.h` 瘦身拆分（HAL 宏 → hw_pinmap 已建，原文件待同步）；
- 通讯接口后置重构；
- host 单测框架（方向已确认可做，暂缓）。

## 9. 反模式速查

- [ ] abs/drv 出现厂商库符号或 `hw_/bsp_` → 库泄漏
- [ ] ops 出现实现词（DMA、set_cs、is_data_ready）→ 接口未语义化
- [ ] drv 里有"内部/外部"或引脚选择 → 资源没注入
- [ ] hw 出现芯片协议逻辑（命令字/校验）→ 职责应上移 drv
- [ ] 业务层 include `hw_*` 或自行 create → 缺组装层
