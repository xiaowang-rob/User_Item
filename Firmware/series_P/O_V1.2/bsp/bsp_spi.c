
#include "bsp_spi.h"
#include "spi.h"
#include "config.h"

__weak void BSP_Encoder_SPI_TxRxCpltCallback(void)
{
    return;
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &ENCODER_SPI_CH)
    {
        BSP_Encoder_SPI_TxRxCpltCallback();
    }
}

__weak void BSP_Encoder_SPI_ErrorCallback(void)
{
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &ENCODER_SPI_CH)
    {
        BSP_Encoder_SPI_ErrorCallback();
    }
}

/* ============================================
 * SPI - 编码器接口
 * ============================================ */
bool BSP_SetEncoder_SPI_Config(u8 CPOL, u8 CPHA, u8 datasize)
{
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
void BSP_Encoder_CS(eEncoderType type, bool level)
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

bool BSP_Encoder_SPI_IS_READY()
{
    return HAL_SPI_GetState(&ENCODER_SPI_CH) == HAL_SPI_STATE_READY;
}

bool BSP_Encoder_SPI_TransmitReceive_DMA(u8 *tx, u8 *rx, u16 len)
{
    return HAL_SPI_TransmitReceive_DMA(&ENCODER_SPI_CH, tx, rx, len) == HAL_OK;
}

void BSP_Encoder_SPI_Abort()
{
    HAL_SPI_Abort(&ENCODER_SPI_CH);
}

void BSP_Encoder_SPI_CLEAR_DMA_error_flags()
{
    __HAL_SPI_CLEAR_OVRFLAG(&ENCODER_SPI_CH);
    __HAL_SPI_CLEAR_FREFLAG(&ENCODER_SPI_CH);
}

/* ============================================
 * SPI - flash接口
 * ============================================ */
void BSP_Flash_CS(bool level)
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
bool BSP_Flash_SPI_Transmit(u8 *tx, u16 len, u32 timeout)
{
    return HAL_SPI_Transmit(&FLASH_SPI_CH, tx, len, timeout) == HAL_OK;
}
bool BSP_Flash_SPI_Receive(u8 *rx, u16 len, u32 timeout)
{
    return HAL_SPI_TransmitReceive(&FLASH_SPI_CH, rx, rx, len, timeout) == HAL_OK;
}