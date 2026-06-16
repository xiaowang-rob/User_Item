
#include "bsp_adc.h"
#include "bsp_usb.h"        // [DEBUG] BSP_USB_CDC_Transmit_FS
#include "usr_config.h"
#include "main.h"

#include "foc_main.h"
#include "status_feedback.h"
#include "log.h"
#include "protection_manager.h"
#include "port_mapping.h"
#include "usb_port.h"        // [DEBUG] fUSB_SendFrame
#include "device.h"

#ifdef __DEBUG__ //***********调试************

u32 time_while_zero = 0;
u32 time_while_T = 0;
#endif

void BSP_Init_Front(void)
{
    BSP_SetVectorTableOffset(VECT_TABLE_OFFSET);
    BSP_enable_irq(); // 使能全局中断,bl中关断了
}
void BSP_Init_Back(void)
{
    /*
这里的顺序不能乱，因为里面有一些初始化函数，如果顺序不对，会导致一些变量没有初始化，导致程序出错
参数必须尽早出现 flash在其之前，初始化的时候最好不要有其他中断干涉，所有adc得往后放，但是foc初始化必须得有adc值，故最后
正确的顺序应该是：
flash和参数一起-通讯-保护-日志-adc -foc初始化
*/

    fFLASH_Init();
    if (!fParamInit())
        BSP_Error_Handler();
    fCommunicateInit();
    fProManagerInit(&g_Param);
    fLogInit();
    BSP_AdcInit(); // 这里就启动了foc的定时器
    fFocInit();
}
void BSP_AppMain(void)
{
    // [DEBUG] USB 通信测试：每隔 2 秒主动发送一帧测试数据
    static u32 _dbg_last_send = 0;
    static u8 _dbg_counter = 0;

    while (1)
    {
        // [DEBUG] USB 发送测试
        if (BSP_GetTick() - _dbg_last_send > 2000)
        {
            _dbg_last_send = BSP_GetTick();
            _dbg_counter++;
            u8 test_data[5] = {0xDE, 0xAD, _dbg_counter, 0x00, 0x00};
            bool ok = fUSB_SendFrame(0xF0, test_data, sizeof(test_data));
            // 也尝试裸 CDC 发送
            u8 raw_msg[] = "HELLO_USB_TEST\r\n";
            BSP_USB_CDC_Transmit_FS(raw_msg, sizeof(raw_msg) - 1);
        }

        //	编码器主循环
        fEncoderMainLoopTask();
        // 通讯层运行
        fCommunicateMainLoop();
        // 控制层由定时器驱动
        // 服务层运行
        fProManagerMainLoop();
        fStatusFeedbackMainLoop();
#ifdef __DEBUG__ //***********调试************

        time_while_T = BSP_GetTick_us() - time_while_zero;
        time_while_zero = BSP_GetTick_us();
#endif
    }
}

void BSP_Error_Handler()
{
    BSP_disable_irq();
    while (1)
    {
        fSystemFaultFeedback();
    }
}
