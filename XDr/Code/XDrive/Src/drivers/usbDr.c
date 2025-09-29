#include "usbDr.h"
#include "string.h"
#include "usbd_cdc_if.h"
USB_frame_t UsbTxFrame_g = {0};
USB_frame_t UsbRxFrame_g = {0};

void usb_cdc_init(void)
{
    memset(&UsbTxFrame_g, 0, sizeof(USB_frame_t));
    memset(&UsbRxFrame_g, 0, sizeof(USB_frame_t));
    UsbTxFrame_g.head = USB_PACKET_HEAD;
    UsbTxFrame_g.tail = USB_PACKET_TAIL;

    UsbRxFrame_g.head = USB_PACKET_HEAD;
    UsbRxFrame_g.tail = USB_PACKET_TAIL;
}

bool usbSendData(u8 *data, u16 len)
{
    if (CDC_Transmit_FS(data, len) == 0) // 成功返回0
        return true;
    else if (CDC_Transmit_FS(data, len) == 1) // 忙 重发
        usbSendData(data, len);
    return false;
}
bool usb_Frame_send(USB_MSG_ID_e msg_id, u8 *data, u16 len)
{
    u16 check;
    for (int i = 0; i < len; i++)
        check += data[i];
    UsbTxFrame_g.msg_id = msg_id;
    UsbTxFrame_g.len = len;
    memcpy(UsbTxFrame_g.data, data, len);
    UsbTxFrame_g.check = (u8)check & 0x01;
    usbSendData((u8 *)&UsbTxFrame_g, len + 3);     // 3是head,msg_id,len的长度
    if (usbSendData((u8 *)&UsbTxFrame_g.check, 2)) // 2是check,tail的长度
        return true;
    else
        return false;
}
__weak void usb_FrameData_deal(USB_MSG_ID_e msg_id, u8 *data, u16 *len)
{
    return;
}

static u16 check = 0;
bool usbRecvByte(u8 *data, u16 *len)
{
    if (data == NULL || len == NULL)
        return false;
    if ((data[0] == UsbRxFrame_g.head) && (data[*len - 1] == UsbRxFrame_g.tail))
    {
        UsbRxFrame_g.check = data[*len - 2];
        for (int i = 3; i < *len - 2; i++)
            check += data[i];
        if ((check & 0x01) != UsbRxFrame_g.check)
            return false;
        else
        {
            UsbRxFrame_g.msg_id = data[1];
            UsbRxFrame_g.len = data[2];
            memcpy(UsbRxFrame_g.data, data + 1, UsbRxFrame_g.len);
            usb_FrameData_deal(UsbRxFrame_g.msg_id, UsbRxFrame_g.data, (u16 *)&UsbRxFrame_g.len);
        }
    }
    return true;
}