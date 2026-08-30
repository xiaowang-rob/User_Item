#include "bsp_usb.h"
#include "usbd_cdc_if.h"
#include "gpio.h"
#include "board_config.h"

// ============================================
// USB - USB虚拟串口
// ============================================
void bsp_usb_cs(bool level)
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
bool bsp_usb_cdc_transmit_fs(uint8_t *data, uint16_t len)
{
    return CDC_Transmit_FS(data, len) == USBD_OK;
}

__weak bool bsp_usb_recv_byte(u8 *data, u8 *len)
{
    return false;
}
void USB_RecvByte(uint8_t *Buf, uint32_t *Len)
{
    bsp_usb_recv_byte(Buf, (u8 *)Len);
}