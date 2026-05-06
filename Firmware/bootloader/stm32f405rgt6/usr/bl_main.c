
#include "bsp.h"
#include "bsp_flash.h"
#include "bsp_led.h"
#include "bsp_usb.h"
#include "usr_config.h"
#include "string.h"
typedef enum
{
    BSP_BL_LED_IDLE,    /* 交替慢闪：等待升级命令 */
    BSP_BL_LED_WRITING, /* 同步快闪：正在擦除/写入 Flash */
    BSP_BL_LED_SUCCESS, /* 同步慢闪：升级完成，准备跳转 */
    BSP_BL_LED_ERROR    /* 交替快闪：升级失败/校验错误 */
} BSP_BL_LED_State_t;

/* ========== 私有变量 ========== */
static u8 rxbuf[280] = {0};
static u32 g_firmware_total_size = 0;
static u32 upgrade_addr;
static u8 g_fw_info_str[24] = {0};
static u16 fw_info_len;
static bool g_in_upgrade_mode = false;

/* 升级命令变量 */
static u8 g_upgrade_cmd;
static u16 g_upgrade_len;
static u8 g_upgrade_data[256];
static u8 g_cmd_received;

/* USB 发送缓冲区 */
static u8 firmware_info[36] = {USB_PACKET_HEAD, UC_CONNECT};
static u8 response[6] = {USB_PACKET_HEAD, 0x00, 0x01, 0x00, 0x00, USB_PACKET_TAIL};

/* LED 状态变量 */
static BSP_BL_LED_State_t g_bl_led_state = BSP_BL_LED_IDLE;
static u32 g_bl_led_timer = 0;
static bool g_bl_led_toggle = true;

/* ========== 内部函数声明 ========== */
static u8 Process_Upgrade_Cmd(u8 cmd, u8 *data, u16 len);

/* ========== BL 专用的 BSP 初始化函数 ========== */

/* ========== USB 接收处理 ========== */
bool BSP_USB_RecvByte(u8 *data, u8 *len)
{
    if (*len == 0 || *len > sizeof(rxbuf))
        return false;

    memcpy(rxbuf, data, *len);
    if (*len >= 5 && data[0] == USB_PACKET_HEAD && data[*len - 1] == USB_PACKET_TAIL)
    {
        u16 check = 0;
        for (u8 i = 3; i < *len - 2; i++)
        {
            check += data[i];
        }
        check &= 0xFF;
        if (data[*len - 2] == (u8)check)
        {
            g_upgrade_cmd = data[1];
            g_upgrade_len = data[2];
            if (g_upgrade_len > 0 && g_upgrade_len <= 256)
            {
                memcpy(g_upgrade_data, &data[3], g_upgrade_len);
            }
            g_cmd_received = 1;
        }
    }
    return true;
}

/* ========== 命令处理 ========== */
static u8 Process_Upgrade_Cmd(u8 cmd, u8 *data, u16 len)
{
    u8 result = FEEDBACK_EXECUTE;

    switch (cmd)
    {
    case UC_CONNECT:
        firmware_info[2] = (u8)strlen((char *)g_fw_info_str);
        memcpy(&firmware_info[3], g_fw_info_str, firmware_info[2]);
        {
            u16 checksum = 0;
            for (u8 i = 0; i < firmware_info[2]; i++)
                checksum += firmware_info[i + 3];
            firmware_info[3 + firmware_info[2]] = (u8)(checksum & 0xFF);
        }
        firmware_info[4 + firmware_info[2]] = USB_PACKET_TAIL;
        BSP_USB_CDC_Transmit_FS(firmware_info, 5 + firmware_info[2]);
        return FEEDBACK_EXECUTE;

    case CMD_IAP_ENTER:
        if (len >= 4)
        {
            g_firmware_total_size = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
            result = FEEDBACK_EXECUTE;
        }
        else
        {
            result = FEEDBACK_FAILURE;
        }
        break;

    case CMD_IAP_ERASE_FLASH:
        result = BSP_Flash_EraseApp() ? FEEDBACK_EXECUTE : FEEDBACK_FAILURE;
        break;

    case CMD_IAP_WRITE_FLASH:
        if (len < 4)
        {
            result = FEEDBACK_FAILURE;
            break;
        }
        upgrade_addr = (u32)data[0] | ((u32)data[1] << 8) |
                       ((u32)data[2] << 16) | ((u32)data[3] << 24);
        if (upgrade_addr % 4 != 0)
        {
            result = FEEDBACK_FAILURE;
        }
        else
        {
            result = BSP_Flash_WriteApp(upgrade_addr, &data[4], len - 4)
                         ? FEEDBACK_EXECUTE
                         : FEEDBACK_FAILURE;
        }
        break;

    case CMD_IAP_VERIFY_FLASH:
        if (len < 4)
        {
            result = FEEDBACK_FAILURE;
            break;
        }
        upgrade_addr = (u32)data[0] | ((u32)data[1] << 8) |
                       ((u32)data[2] << 16) | ((u32)data[3] << 24);
        result = BSP_Flash_Verify(upgrade_addr, &data[4], len - 4)
                     ? FEEDBACK_EXECUTE
                     : FEEDBACK_FAILURE;
        break;

    case CMD_IAP_EXIT:
        result = BSP_ClearUpgradeFlag() ? FEEDBACK_EXECUTE : FEEDBACK_FAILURE;
        break;

    default:
        result = FEEDBACK_FAILURE;
        break;
    }

    response[1] = cmd;
    response[3] = result;
    response[4] = result;
    BSP_USB_CDC_Transmit_FS(response, 6);

    return result;
}

/* ========== LED 接口 ========== */
void BSP_BL_LED_Init(void)
{
    BSP_LED_CanSetPin(false);
    BSP_LED_EncoderSetPin(false);
    g_bl_led_state = BSP_BL_LED_IDLE;
    g_bl_led_timer = BSP_GetTick();
    g_bl_led_toggle = true;
}

void BSP_BL_LED_SetState(BSP_BL_LED_State_t state)
{
    if (state == g_bl_led_state)
        return;
    g_bl_led_state = state;
    g_bl_led_timer = BSP_GetTick();
    g_bl_led_toggle = true;

    BSP_LED_CanSetPin(false);
    BSP_LED_EncoderSetPin(false);
}

void BSP_BL_LED_Process(void)
{
    u32 now = BSP_GetTick();
    u32 interval = 0;

    switch (g_bl_led_state)
    {
    case BSP_BL_LED_IDLE:
    case BSP_BL_LED_SUCCESS:
        interval = 500;
        break;
    case BSP_BL_LED_WRITING:
        interval = 100;
        break;
    case BSP_BL_LED_ERROR:
        interval = 100;
        break;
    default:
        return;
    }

    if (now - g_bl_led_timer >= interval)
    {
        g_bl_led_toggle = !g_bl_led_toggle;
        g_bl_led_timer = now;

        switch (g_bl_led_state)
        {
        case BSP_BL_LED_IDLE:
        case BSP_BL_LED_ERROR:
            BSP_LED_CanSetPin(!g_bl_led_toggle);
            BSP_LED_EncoderSetPin(g_bl_led_toggle);
            break;
        case BSP_BL_LED_SUCCESS:
        case BSP_BL_LED_WRITING:
            BSP_LED_CanSetPin(g_bl_led_toggle);
            BSP_LED_EncoderSetPin(g_bl_led_toggle);
            break;
        }
    }
}

void BSP_Init(void)
{
    BSP_BL_LED_Init();
}

void BSP_BlMain(void)
{

    BSP_BL_LED_SetState(BSP_BL_LED_IDLE);

    if (BSP_GetUpgradeFlag(g_fw_info_str, &fw_info_len))
    {
        g_in_upgrade_mode = true;
    }

    if (g_in_upgrade_mode)
    {
        while (1)
        {
            BSP_BL_LED_Process();

            if (g_cmd_received)
            {
                g_cmd_received = 0;

                switch (g_upgrade_cmd)
                {
                case UC_CONNECT:
                case CMD_IAP_ENTER:
                    BSP_BL_LED_SetState(BSP_BL_LED_IDLE);
                    break;
                case CMD_IAP_ERASE_FLASH:
                case CMD_IAP_WRITE_FLASH:
                case CMD_IAP_VERIFY_FLASH:
                    BSP_BL_LED_SetState(BSP_BL_LED_WRITING);
                    break;
                case CMD_IAP_EXIT:
                    BSP_BL_LED_SetState(BSP_BL_LED_SUCCESS);
                    break;
                default:
                    BSP_BL_LED_SetState(BSP_BL_LED_ERROR);
                    break;
                }

                BSP_disable_irq();
                u8 cmd = g_upgrade_cmd;
                u16 len = g_upgrade_len;
                u8 data_buf[256];
                if (len > 0 && len <= 256)
                    memcpy(data_buf, g_upgrade_data, len);
                g_cmd_received = 0;
                BSP_enable_irq();

                u8 result = Process_Upgrade_Cmd(cmd, data_buf, len);

                if (cmd == CMD_IAP_EXIT)
                {
                    if (result == FEEDBACK_EXECUTE)
                    {
                        for (int i = 0; i < 200; i++)
                        {
                            BSP_Delay(10);
                            BSP_BL_LED_Process();
                        }
                        break; // 跳转到 App
                    }
                    else
                    {
                        BSP_BL_LED_SetState(BSP_BL_LED_ERROR);
                    }
                }
                else if (result == FEEDBACK_FAILURE)
                {
                    BSP_BL_LED_SetState(BSP_BL_LED_ERROR);
                }
            }
            BSP_Delay(1);
        }
    }

    if (!BSP_JumpToApp())
    {
        BSP_BL_LED_SetState(BSP_BL_LED_ERROR);
    }

    while (1)
    {
        BSP_BL_LED_Process();
        BSP_Delay(1);
    }
}

void BSP_Error_Handler(void)
{
    BSP_BL_LED_SetState(BSP_BL_LED_ERROR);
    BSP_disable_irq();
    while (1)
    {
        BSP_BL_LED_Process();
        BSP_Delay(10);
    }
}