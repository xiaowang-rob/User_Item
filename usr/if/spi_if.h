#ifndef __SPI_IF_H
#define __SPI_IF_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================
// spi_if.h — SPI 总线最小能力接口（usr/if 契约，同步）
//
// 用途：drv 层芯片驱动（编码器 / W25QXX 等）访问 SPI 总线的唯一通道。
//       驱动不 include 任何厂商库头，只面对本接口表。
//
// 实现者：board/<b>/hw
//   - HAL 板    : set_mode 做 HAL_SPI_Init 字段修改；xfer = HAL_SPI_TransmitReceive
//   - SPL 板    : SPI_InitTypeDef + SPI_Init(SPIx,...)；xfer = SPI_I2S_SendData + 轮询标志
//   - 寄存器板  : 直接写 SPIx->CR1/CR2/DR，读 SPIx->SR
//
// 规则：
//   - 本文件禁止 include 任何非标准头（可被 hw 干净反向 include）
//   - 接口参数只用 C 基础类型，不泄漏任何厂商库类型
//   - ctx 由 hw 持有并解释，对调用方(drv)完全不透明，drv 永不解析
//   - SPI 模式(CPOL/CPHA/位宽)是芯片协议事实 → 由 drv 调用 set_mode 声明，
//     执行留在 hw；data_bits 取值 8 或 16
//   - CS 由 hw 为"每条片选"造独立接口表实例，驱动无需感知具体引脚；
//     驱动负责成对调用 cs(true)/.../cs(false)
// ============================================================

typedef struct
{
    void *ctx; // hw 侧资源（库句柄/CS 引脚等，对 drv 不透明）

    // 配置 SPI 模式；返回 false 表示该模式本总线不支持
    bool (*set_mode)(void *ctx, uint8_t cpol, uint8_t cpha, uint8_t data_bits);

    // 同步全双工传输：发 tx、收 rx、长 len 字节；忙/失败返回 false
    bool (*xfer)(void *ctx, const uint8_t *tx, uint8_t *rx, uint16_t len);

    // 片选：active=true 拉低 CS（选中）；调用方负责成对调用
    void (*cs)(void *ctx, bool active);
} tSpiBusIf;

#endif // __SPI_IF_H
