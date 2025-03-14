#include "I2S_MC.h"
#include "usb_buffer_part.h"
void MicroPhone_Init()
{
USB_Audio_Port_Init();
I2S_MC_DMA_Init();	
}