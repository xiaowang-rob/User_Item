#ifndef __ENC_SPI_ENGINE_H
#define __ENC_SPI_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

#include "usr/if/spi_if.h"
#include "usr/if/time_if.h"

// ============================================================
// enc_spi_engine.h — 同步磁编码器 SPI 读角时序引擎（usr/drv）
//
// 收敛 AS5047 / MT6816 / MT6835 共有的"片选周期内同步收发帧"行为：
//   CS 拉低 → 依次执行各传输段（段间 CS 保持）→ CS 拉高。
// 芯片差异（SPI 模式、段内容、字节宽度、数据解析）留在各芯片文件。
//
// 纯逻辑，只依赖 usr/if 接口表 —— 可在 host 上用 stub 单测。
// ============================================================

// 一次读角用到的引擎实例（每芯片 ctx 内嵌一份）
typedef struct
{
    const tSpiBusIf *bus;  // 注入：编码器总线（hw 实现）
    const tTimeIf *time;   // 注入：时间基准（打时间戳）
} tEncSpiEngine;

// 一段全双工传输：tx/rx 为连续内存视图，len 以字节计
// （16bit 模式由 bus 按 half-word 单元解释，调用方按芯片位宽构造内存视图）
typedef struct
{
    const uint8_t *tx;
    uint8_t *rx;
    uint16_t len;
} tEncXferSeg;

// 执行一次读角序列：任一段失败立即抬 CS 并返回 false
bool enc_engine_run(tEncSpiEngine *e, const tEncXferSeg *segs, uint8_t n);

// 中止并复位总线（抬 CS），用于错误恢复
void enc_engine_abort(tEncSpiEngine *e);

#endif // __ENC_SPI_ENGINE_H
