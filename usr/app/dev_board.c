// ============================================================
// dev_board.c — 组装层：板级设备装配（usr/app，xdr_p_o1.2）
//
// 唯一同时 include hw 头 + drv 头 + abs 头的翻译单元。
// 装配顺序：hw_base_init → 时间 → 总线资源 → 芯片句柄 → abs 业务对象。
//
// 设备选择（换电机/换芯片）只改本文件顶部的"产品配置区"。
// ============================================================

#include "dev_board.h"

#include <stddef.h> // NULL

// ---- hw（板上资源） ----
#include "hw_base.h"
#include "hw_enc_spi.h"
#include "hw_flash_spi.h"
#include "hw_led.h"
#include "hw_rgb_pwm.h"

// ---- drv（芯片协议实现） ----
#include "usr/drv/encoder_drivers.h"
#include "usr/drv/flash_drivers.h"
#include "usr/drv/rgb_drivers.h"

// ============================================================
// 产品配置区
// ============================================================

// 电机用内部还是外部编码器 CS（true=内部）
#ifndef DEV_USE_INTERNAL_ENC
#define DEV_USE_INTERNAL_ENC 1
#endif

// 编码器芯片选择：默认 MT6816。
// 换芯片改为在编译定义中启用其一，如 -DDEV_ENC_CHIP_AS5047
//   #define DEV_ENC_CHIP_AS5047
//   #define DEV_ENC_CHIP_MT6835

// ============================================================

tDevBoard g_dev;

// ---- 编码器装配（按产品配置选择芯片） ----
static bool dev_assemble_encoder(const tSpiBusIf *bus, const tTimeIf *time)
{
    EncoderChipHandle chip = NULL;
    const tEncoderDriverOps *ops = NULL;

#if defined(DEV_ENC_CHIP_AS5047)
    chip = AS5047_create(bus, time);
    ops = &AS5047_driver_ops;
#elif defined(DEV_ENC_CHIP_MT6835)
    chip = MT6835_create(bus, time);
    ops = &MT6835_driver_ops;
#else // 默认 MT6816
    chip = MT6816_create(bus, time);
    ops = &MT6816_driver_ops;
#endif

    if (!chip || !ops)
        return false;
    return encoder_init(&g_dev.enc, ops, chip);
}

// ---- RGB 装配 ----
static bool dev_assemble_rgb(const tTimeIf *time)
{
    RgbHandle chip = ws28xx_create(hw_rgb_pwm_get(), hw_rgb_pixel_num());
    if (!chip)
        return false;
    return rgb_init(&g_dev.rgb, &ws28xx_driver_ops, chip, time);
}

// ---- 外部 Flash 装配（存储单元挂在介质首个扇区；芯片缺失则不可用） ----
static bool dev_assemble_flash(const tTimeIf *time)
{
    FlashChipHandle chip = w25qxx_create(hw_flash_bus_get(), time);
    if (!chip)
        return false;

    // 先初始化介质（JEDEC ID 校验），再挂载日志式存储单元
    if (!w25qxx_driver_ops.init(chip))
    {
        w25qxx_destroy(chip);
        return false;
    }
    bool ok = flash_unit_init(&g_dev.ext_flash, &w25qxx_driver_ops, chip, 0U);
    if (!ok)
        w25qxx_destroy(chip);
    return ok;
}

bool dev_board_init(void)
{
    // 1) 板级基础：DWT 周期计数（微秒时间基准）
    hw_base_init();

    // 2) 时间基准
    g_dev.time = hw_time_get();
    if (!g_dev.time)
        return false;

    // 3) 逐设备装配（灯效失败不阻塞关键设备）
    g_dev.led_can = (tLed){0};
    g_dev.led_enc = (tLed){0};
    g_dev.rgb = (tRgb){0};

    bool led_ok = led_init(&g_dev.led_can, hw_led_ops(), hw_led_handle(0), g_dev.time) &&
                  led_init(&g_dev.led_enc, hw_led_ops(), hw_led_handle(1), g_dev.time);
    (void)led_ok; // 板载 LED 为辅助指示，失败不阻塞

    g_dev.rgb_ok = dev_assemble_rgb(g_dev.time);
    g_dev.flash_ok = dev_assemble_flash(g_dev.time);

    // 4) 编码器（关键设备）：内部/外部总线由产品配置决定
    const tSpiBusIf *enc_bus = hw_enc_bus_get(DEV_USE_INTERNAL_ENC);
    g_dev.enc_ok = dev_assemble_encoder(enc_bus, g_dev.time);

    return g_dev.enc_ok;
}
