#include "usbDr.h"
#include "string.h"
#include "usbd_cdc_if.h"
USB_frame_t UsbTxFrame = {0};
USB_frame_t UsbRxFrame = {0};

void usb_cdc_init(void)
{
    memset(&UsbTxFrame, 0, sizeof(USB_frame_t));
    memset(&UsbRxFrame, 0, sizeof(USB_frame_t));
    UsbTxFrame.head = USB_PACKET_HEAD;
    UsbTxFrame.tail = USB_PACKET_TAIL;

    UsbRxFrame.head = USB_PACKET_HEAD;
    UsbRxFrame.tail = USB_PACKET_TAIL;
}

bool usbSendData(u8 *data, u8 len)
{
    if (CDC_Transmit_FS(data, len) == 0) // 成功返回0
        return true;
    else if (CDC_Transmit_FS(data, len) == 1) // 忙 重发
        usbSendData(data, len);
    return false;
}
bool usb_Frame_send(u8 id, u8 *data, u8 len)
{
    u16 check = 0;
    for (int i = 0; i < len; i++)
        check += data[i];
    UsbTxFrame.id = id;
    UsbTxFrame.len = len;
    memcpy(UsbTxFrame.data, data, len);
    UsbTxFrame.check = (u8)check & 0x01;
    usbSendData((u8 *)&UsbTxFrame, len + 3);     // 3是head,id,len的长度
    if (usbSendData((u8 *)&UsbTxFrame.check, 2)) // 2是check,tail的长度
        return true;
    else
        return false;
}
__weak void usb_FrameData_deal(u8 id, u8 *data, u8 len)
{
    return;
}

 bool usbRecvByte(u8 *data, u8 *len)
{
    if (data == NULL || len == NULL)
        return false;
    if ((data[0] == UsbRxFrame.head) && (data[*len - 1] == UsbRxFrame.tail))
    {
        UsbRxFrame.check = data[*len - 2];
        u16 check = 0;
        for (int i = 3; i < *len - 2; i++)
            check += data[i];
        if ((check & 0x01) != UsbRxFrame.check)
            return false;
        else
        {
            UsbRxFrame.id = data[1];
            UsbRxFrame.len = data[2];
            memcpy(UsbRxFrame.data, data + 3, UsbRxFrame.len);
            usb_FrameData_deal(UsbRxFrame.id, UsbRxFrame.data, UsbRxFrame.len);
        }
    }
    return true;
}