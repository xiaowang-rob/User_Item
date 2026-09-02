// ============================================================
// hw_enc_spi.c — 编码器 SPI 总线接口表实现（xdr_p_o1.2 / SPI3）
//
// 把 SPI3 + 内/外 CS 引脚封装成 usr/if 的 tSpiBusIf 契约。
// 厂商库符号（hspi3、HAL_*、GPIO 宏）只出现在本文件。
//
// 说明：
//   - SPI3 CubeMX 默认 16bit / Mode3(CPOL=1,CPHA=1) / APB1÷32；
//     芯片要求不同模式（如 AS5047=Mode1）时由 drv 经 set_mode 声明
//   - xfer 的 len 以字节计；16bit 模式下按 half-word 单元执行（与既有 DMA 行为一致，
//     调用方按芯片数据宽度构造内存视图，如 uint16 命令 cast 字节）
//   - 内/外两条实例共享同一 SPI3；同一时刻只应有一个在传输（由组装层保证）
// ============================================================

#include "hw_enc_spi.h"

#include "hw_pinmap.h" // ENCODER_INT/EXT_CS_GPIOx/PIN、ENCODER_SPI_CH
#include "spi.h"       // CubeMX: extern SPI_HandleTypeDef hspi3
#include "stm32f4xx_hal.h"

// ---- 总线资源 ----
typedef struct
{
    SPI_HandleTypeDef *hspi;  // 使用的 SPI 句柄
    GPIO_TypeDef *cs_port;    // CS 引脚端口
    uint16_t cs_pin;          // CS 引脚
    uint8_t data_bits;        // 当前数据宽度(8/16)，由 set_mode 维护
} tEncSpiRes;

static tEncSpiRes g_enc_int_res = { &hspi3, ENCODER_INT_CS_GPIOx, ENCODER_INT_CS_GPIOx_PIN, 16 };
static tEncSpiRes g_enc_ext_res = { &hspi3, ENCODER_EXT_CS_GPIOx, ENCODER_EXT_CS_GPIOx_PIN, 16 };

// ---- tSpiBusIf 实现 ----

static bool enc_set_mode(void *ctx, uint8_t cpol, uint8_t cpha, uint8_t data_bits)
{
    tEncSpiRes *r = (tEncSpiRes *)ctx;
    if (!r || (data_bits != 8 && data_bits != 16))
        return false;

    SPI_HandleTypeDef *h = r->hspi;
    h->Init.Mode = SPI_MODE_MASTER;
    h->Init.Direction = SPI_DIRECTION_2LINES;
    h->Init.DataSize = (data_bits == 16) ? SPI_DATASIZE_16BIT : SPI_DATASIZE_8BIT;
    h->Init.CLKPolarity = cpol ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
    h->Init.CLKPhase = cpha ? SPI_PHASE_2EDGE : SPI_PHASE_1EDGE;
    h->Init.NSS = SPI_NSS_SOFT;
    h->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    h->Init.FirstBit = SPI_FIRSTBIT_MSB;
    h->Init.TIMode = SPI_TIMODE_DISABLE;
    h->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    h->Init.CRCPolynomial = 10;

    if (HAL_SPI_Init(h) != HAL_OK)
        return false;

    r->data_bits = data_bits;
    return true;
}

static bool enc_xfer(void *ctx, const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    tEncSpiRes *r = (tEncSpiRes *)ctx;
    if (!r || !rx)
        return false;

    // 16bit 模式下 HAL 的 Size 以 half-word 单元计
    uint16_t units = (r->data_bits == 16) ? (len / 2) : len;
    if (units == 0)
        return false;

    // 同步调用前确认总线空闲，避免上一次异常遗留造成挂死
    if (HAL_SPI_GetState(r->hspi) != HAL_SPI_STATE_READY)
        return false;

    // HAL 不修改 tx 内容，此处仅去掉 const 以满足 API 签名
    return HAL_SPI_TransmitReceive(r->hspi, (uint8_t *)tx, rx, units, 100U) == HAL_OK;
}

static void enc_cs(void *ctx, bool active)
{
    tEncSpiRes *r = (tEncSpiRes *)ctx;
    if (!r)
        return;
    HAL_GPIO_WritePin(r->cs_port, r->cs_pin, active ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

// ---- 导出的接口表实例 ----

static const tSpiBusIf g_enc_int_bus = {
    .ctx = &g_enc_int_res,
    .set_mode = enc_set_mode,
    .xfer = enc_xfer,
    .cs = enc_cs,
};

static const tSpiBusIf g_enc_ext_bus = {
    .ctx = &g_enc_ext_res,
    .set_mode = enc_set_mode,
    .xfer = enc_xfer,
    .cs = enc_cs,
};

const tSpiBusIf *hw_enc_bus_get(bool is_internal)
{
    return is_internal ? &g_enc_int_bus : &g_enc_ext_bus;
}
