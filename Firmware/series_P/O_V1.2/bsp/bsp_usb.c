#include "bsp_usb.h"
#include "usbd_cdc_if.h"
#include "gpio.h"
#include "config.h"

/* ============================================
 * USB - USB虚拟串口
 * ============================================ */
void BSP_USB_CS(bool level)
{
    if (level)
    {
        HAL_GPIO_WritePin(USB_CS_GPIOx, USB_CS_GPIOx_PIN, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(USB_CS_GPIOx, USB_CS_GPIOx_PIN, GPIO_PIN_RESET);
    }
}
bool BSP_USB_CDC_Transmit_FS(uint8_t *data, uint16_t len)
{
    return CDC_Transmit_FS(data, len) == USBD_OK;
}

__weak bool BSP_USB_RecvByte(u8 *data, u8 *len)
{
    return false;
}
void USB_RecvByte(uint8_t *Buf, uint32_t *Len)
{
    BSP_USB_RecvByte(Buf, (u8 *)Len);
}