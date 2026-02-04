#include "usb_port.h"
#include "string.h"
#include "usbd_cdc_if.h"

USB_frame_t UsbTxFrame = {.head = USB_PACKET_HEAD, .tail = USB_PACKET_TAIL};
USB_frame_t UsbRxFrame = {.head = USB_PACKET_HEAD, .tail = USB_PACKET_TAIL};

static u8 trans_fault_tic = 0;
bool usbSendData(u8 *data, u8 len)
{
    u8 Trans_state = CDC_Transmit_FS(data, len);
    if (Trans_state == 0) // 成功返回0
    {
        return true;
        trans_fault_tic = 0;
    }
    else // 忙 重发
    {
        if (trans_fault_tic > 5)
        {
            trans_fault_tic = 0;
            return false;
        }
        trans_fault_tic++;
        usbSendData(data, len);
    }
    return false;
}
bool usb_Frame_send(u8 id, u8 *data, u8 len)
{
    if (len == 0)
        return false;
    u16 check = 0;
    for (int i = 0; i < len; i++)
        check += data[i];
    UsbTxFrame.id = id;
    UsbTxFrame.len = len;
    memcpy(UsbTxFrame.data, data, len);
    UsbTxFrame.check = (u8)check & 0x01;
    UsbTxFrame.data[len] = UsbTxFrame.check;
    UsbTxFrame.data[len + 1] = UsbTxFrame.tail;
    return usbSendData((u8 *)&UsbTxFrame, len + 5); // 5是head,id,len,check,tail的长度
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
