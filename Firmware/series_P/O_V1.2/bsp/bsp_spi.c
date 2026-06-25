
#include "bsp_spi.h"
#include "spi.h"
#include "config.h"

__weak void bsp_encoder_spi_txrx_cplt_callback(void)
{
    return;
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &ENCODER_SPI_CH)
    {
        bsp_encoder_spi_txrx_cplt_callback();
    }
}

__weak void bsp_encoder_spi_error_callback(void)
{
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &ENCODER_SPI_CH)
    {
        bsp_encoder_spi_error_callback();
    }
}

// ============================================
// SPI - 编码器接口
// ============================================
bool bsp_set_encoder_spi_config(u8 CPOL, u8 CPHA, u8 datasize)
{
    ENCODER_SPI_CH.Instance = ENCODER_SPI;
    ENCODER_SPI_CH.Init.Mode = SPI_MODE_MASTER;
    ENCODER_SPI_CH.Init.Direction = SPI_DIRECTION_2LINES;

    ENCODER_SPI_CH.Init.NSS = SPI_NSS_SOFT;
    ENCODER_SPI_CH.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    ENCODER_SPI_CH.Init.FirstBit = SPI_FIRSTBIT_MSB;
    ENCODER_SPI_CH.Init.TIMode = SPI_TIMODE_DISABLE;
    ENCODER_SPI_CH.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    ENCODER_SPI_CH.Init.CRCPolynomial = 10;

    ENCODER_SPI_CH.Init.CLKPolarity = CPOL ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
    ENCODER_SPI_CH.Init.CLKPhase = CPHA ? SPI_PHASE_2EDGE : SPI_PHASE_1EDGE;
    if (datasize == 8)
        ENCODER_SPI_CH.Init.DataSize = SPI_DATASIZE_8BIT;
    else if (datasize == 16)
        ENCODER_SPI_CH.Init.DataSize = SPI_DATASIZE_16BIT;
    else
        return false; // 不支持的数据位长度

    if (HAL_SPI_Init(&ENCODER_SPI_CH) != HAL_OK)
        return false;

    return true;
}

// 内部接口CS
void bsp_encoder_cs(eEncoderType type, bool level)
{
    if (type == INTERNAL)
    {
        if (level)
        {
            HAL_GPIO_WritePin(ENCODER_INT_CS_GPIOx, ENCODER_INT_CS_GPIOx_PIN, GPIO_PIN_SET);
        }
        else
        {
            HAL_GPIO_WritePin(ENCODER_INT_CS_GPIOx, ENCODER_INT_CS_GPIOx_PIN, GPIO_PIN_RESET);
        }
    }
    else // EXTERNAL
    {
        if (level)
        {
            HAL_GPIO_WritePin(ENCODER_EXT_CS_GPIOx, ENCODER_EXT_CS_GPIOx_PIN, GPIO_PIN_SET);
        }
        else
        {
            HAL_GPIO_WritePin(ENCODER_EXT_CS_GPIOx, ENCODER_EXT_CS_GPIOx_PIN, GPIO_PIN_RESET);
        }
    }
}

bool bsp_encoder_spi_is_ready()
{
    return HAL_SPI_GetState(&ENCODER_SPI_CH) == HAL_SPI_STATE_READY;
}

bool bsp_encoder_spi_transmit_receive_dma(u8 *tx, u8 *rx, u16 len)
{
    return HAL_SPI_TransmitReceive_DMA(&ENCODER_SPI_CH, tx, rx, len) == HAL_OK;
}

void bsp_encoder_spi_abort()
{
    HAL_SPI_Abort(&ENCODER_SPI_CH);
}

void bsp_encoder_spi_clear_dma_error_flags()
{
    __HAL_SPI_CLEAR_OVRFLAG(&ENCODER_SPI_CH);
    __HAL_SPI_CLEAR_FREFLAG(&ENCODER_SPI_CH);
}

// ============================================
// SPI - flash接口
// ============================================
void bsp_flash_cs(bool level)
{
    if (level)
    {
        HAL_GPIO_WritePin(FLASH_CS_GPIOx, FLASH_CS_GPIOx_PIN, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(FLASH_CS_GPIOx, FLASH_CS_GPIOx_PIN, GPIO_PIN_RESET);
    }
}
bool bsp_flash_spi_transmit(u8 *tx, u16 len, u32 timeout)
{
    return HAL_SPI_Transmit(&FLASH_SPI_CH, tx, len, timeout) == HAL_OK;
}
bool bsp_flash_spi_receive(u8 *rx, u16 len, u32 timeout)
{
    return HAL_SPI_TransmitReceive(&FLASH_SPI_CH, rx, rx, len, timeout) == HAL_OK;
}