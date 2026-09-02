// ============================================================
// hw_flash_spi.c — 外部 Flash SPI 总线接口表实现（xdr_p_o1.2 / SPI2）
//
// 把 SPI2 + FLASH_CS(PB12) 封装成 usr/if 的 tSpiBusIf 契约。
// 厂商库符号（hspi2、HAL_*、GPIO 宏）只出现在本文件。
//
// 说明：
//   - W25Q 系列支持 Mode0/3；CubeMX 已按 Mode3 初始化，此处 set_mode 仍由
//     驱动按协议声明（8bit）
//   - xfer 的 len 以字节计（8bit 模式即字节单元）；rx 不可为 NULL
// ============================================================

#include "hw_flash_spi.h"

#include "hw_pinmap.h" // FLASH_SPI_CH、FLASH_CS_GPIOx/PIN
#include "spi.h"       // CubeMX: extern SPI_HandleTypeDef hspi2
#include "stm32f4xx_hal.h"

typedef struct
{
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
    uint8_t data_bits;
} tFlashSpiRes;

static tFlashSpiRes g_flash_res = { &hspi2, FLASH_CS_GPIOx, FLASH_CS_GPIOx_PIN, 8 };

static bool flash_set_mode(void *ctx, uint8_t cpol, uint8_t cpha, uint8_t data_bits)
{
    tFlashSpiRes *r = (tFlashSpiRes *)ctx;
    if (!r || data_bits != 8U) // 外部 NOR Flash 固定 8bit
        return false;

    SPI_HandleTypeDef *h = r->hspi;
    h->Init.Mode = SPI_MODE_MASTER;
    h->Init.Direction = SPI_DIRECTION_2LINES;
    h->Init.DataSize = SPI_DATASIZE_8BIT;
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

static bool flash_xfer(void *ctx, const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    tFlashSpiRes *r = (tFlashSpiRes *)ctx;
    if (!r || !tx || !rx || len == 0U)
        return false;

    if (HAL_SPI_GetState(r->hspi) != HAL_SPI_STATE_READY)
        return false;

    return HAL_SPI_TransmitReceive(r->hspi, (uint8_t *)tx, rx, len, 100U) == HAL_OK;
}

static void flash_cs(void *ctx, bool active)
{
    tFlashSpiRes *r = (tFlashSpiRes *)ctx;
    if (!r)
        return;
    HAL_GPIO_WritePin(r->cs_port, r->cs_pin, active ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static const tSpiBusIf g_flash_bus = {
    .ctx = &g_flash_res,
    .set_mode = flash_set_mode,
    .xfer = flash_xfer,
    .cs = flash_cs,
};

const tSpiBusIf *hw_flash_bus_get(void)
{
    return &g_flash_bus;
}
