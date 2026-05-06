
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
    return HAL_SPI_Receive(&FLASH_SPI_CH, rx, len, timeout) == HAL_OK;
}