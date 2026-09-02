#ifndef __HW_ENC_SPI_H
#define __HW_ENC_SPI_H

#include <stdbool.h>

#include "usr/if/spi_if.h"

// ============================================================
// hw_enc_spi.h — 板上编码器 SPI 总线资源（board/<b>/hw）
//
// 提供两条编码器总线实例（内部/外部 CS），实现 usr/if 的 tSpiBusIf：
//   - set_mode：按芯片协议(CPOL/CPHA/位宽)重新配置 SPI3（执行在 hw，参数来自 drv）
//   - xfer    ：同步全双工，len 以字节计（16bit 模式按 half-word 单元执行）
//   - cs      ：拉低/拉高片选
//
// "这台电机用内部还是外部编码器"是产品决策，由组装层选择调用哪个 getter。
// ============================================================

// 取编码器总线实例：is_internal=true 取内部 CS 实例，false 取外部 CS 实例
const tSpiBusIf *hw_enc_bus_get(bool is_internal);

#endif // __HW_ENC_SPI_H
