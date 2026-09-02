// ============================================================
// w25qxx.c — W25Q128 串行 NOR Flash 驱动（usr/drv，同步）
//
// 芯片协议（8bit SPI，Mode0/3 均支持）：
//   - 页编程 256B / 扇区擦除 4KB / 块 64KB / 容量 16MB
//   - 操作前必须写使能(0x06)，完成后轮询状态寄存器 BUSY 位
// 本实现把原逐字节轮询改为"整段 xfer"：命令+地址+数据一次总线事务。
//
// 仅依赖 usr/if 接口表，无任何板级/厂商库依赖。
// ============================================================

#include <stdlib.h>

#include "usr/abs/device.h"
#include "usr/abs/flash.h"

#include "flash_drivers.h"

// ---- W25Q128 几何 ----
#define W25_CAPACITY_BYTES (16U * 1024U * 1024U) // 16MB
#define W25_PAGE_SIZE 256U
#define W25_SECTOR_SIZE (4U * 1024U) // 4KB
#define W25_SECTOR_COUNT (W25_CAPACITY_BYTES / W25_SECTOR_SIZE)

// ---- 命令 ----
#define FCMD_READ_ID 0x9FU
#define FCMD_WRITE_ENABLE 0x06U
#define FCMD_READ_STATUS 0x05U
#define FCMD_ERASE_SECTOR 0x20U
#define FCMD_READ_DATA 0x03U
#define FCMD_WRITE_PAGE 0x02U

#define FBIT_SR_BUSY 0x01U

#define FTIMEOUT_OP_MS 1000U // 扇区擦除/页编程典型完成时间远小于此

#define READ_CHUNK 32U // 读数据分段长度（tx 补 0xFF 提供时钟）

// W25Q128 JEDEC ID：0xEF 0x40 0x18
static const uint8_t W25_JEDEC_ID[3] = {0xEFU, 0x40U, 0x18U};

typedef struct
{
    eDeviceStatus dstate;
    const tSpiBusIf *bus;
    const tTimeIf *time;

    uint8_t tx_buf[4U + W25_PAGE_SIZE]; // 页写/命令+地址 段缓冲
    uint8_t rx_buf[4U + W25_PAGE_SIZE]; // 对应接收缓冲
    uint8_t rd_ff[READ_CHUNK];          // 读时钟填充（全 0xFF）
} tW25Qxx_ctx;

// ---- 底层原语（CS 由调用处控制） ----

static bool w25_tx(tW25Qxx_ctx *ctx, uint16_t len)
{
    return ctx->bus->xfer(ctx->bus->ctx, ctx->tx_buf, ctx->rx_buf, len);
}

static void w25_delay_ms(tW25Qxx_ctx *ctx, uint32_t ms)
{
    uint32_t t0 = ctx->time->get_ms(ctx->time->ctx);
    while ((ctx->time->get_ms(ctx->time->ctx) - t0) < ms)
    {
    }
}

// 读状态寄存器（CS 自管理）
static uint8_t w25_read_sr(tW25Qxx_ctx *ctx)
{
    ctx->tx_buf[0] = FCMD_READ_STATUS;
    ctx->tx_buf[1] = 0xFFU;
    ctx->bus->cs(ctx->bus->ctx, true);
    bool ok = ctx->bus->xfer(ctx->bus->ctx, ctx->tx_buf, ctx->rx_buf, 2U);
    ctx->bus->cs(ctx->bus->ctx, false);
    return ok ? ctx->rx_buf[1] : 0xFFU;
}

static bool w25_wait_idle(tW25Qxx_ctx *ctx, uint32_t timeout_ms)
{
    uint32_t t0 = ctx->time->get_ms(ctx->time->ctx);
    while (w25_read_sr(ctx) & FBIT_SR_BUSY)
    {
        if ((ctx->time->get_ms(ctx->time->ctx) - t0) >= timeout_ms)
            return false;
    }
    return true;
}

static bool w25_write_enable(tW25Qxx_ctx *ctx)
{
    ctx->tx_buf[0] = FCMD_WRITE_ENABLE;
    ctx->tx_buf[1] = 0xFFU; // 时钟填充（rx 忽略）
    ctx->bus->cs(ctx->bus->ctx, true);
    bool ok = ctx->bus->xfer(ctx->bus->ctx, ctx->tx_buf, ctx->rx_buf, 2U);
    ctx->bus->cs(ctx->bus->ctx, false);
    return ok;
}

// ---- ops 实现 ----

static bool w25_init(FlashChipHandle h)
{
    tW25Qxx_ctx *ctx = (tW25Qxx_ctx *)h;
    if (!ctx)
        return false;

    // W25Q 支持 Mode0/3；此处显式声明 Mode3/8bit（与板默认一致）
    if (!ctx->bus->set_mode(ctx->bus->ctx, 1U, 1U, 8U))
        return false;

    // 上电后芯片可能未就绪，重试读 JEDEC ID
    for (uint8_t attempt = 0U; attempt < 5U; attempt++)
    {
        ctx->tx_buf[0] = FCMD_READ_ID;
        ctx->tx_buf[1] = 0xFFU;
        ctx->tx_buf[2] = 0xFFU;
        ctx->tx_buf[3] = 0xFFU;

        ctx->bus->cs(ctx->bus->ctx, true);
        bool ok = ctx->bus->xfer(ctx->bus->ctx, ctx->tx_buf, ctx->rx_buf, 4U);
        ctx->bus->cs(ctx->bus->ctx, false);

        if (ok && ctx->rx_buf[1] == W25_JEDEC_ID[0] &&
            ctx->rx_buf[2] == W25_JEDEC_ID[1] &&
            ctx->rx_buf[3] == W25_JEDEC_ID[2])
        {
            ctx->dstate = DEV_ONLINE;
            return true;
        }
        w25_delay_ms(ctx, 10U);
    }

    ctx->dstate = DEV_OFFLINE;
    return false;
}

static bool w25_read(FlashChipHandle h, uint32_t addr, uint8_t *data, uint32_t len)
{
    tW25Qxx_ctx *ctx = (tW25Qxx_ctx *)h;
    if (!ctx || !data || (addr + len) > W25_CAPACITY_BYTES)
        return false;

    ctx->tx_buf[0] = FCMD_READ_DATA;
    ctx->tx_buf[1] = (uint8_t)(addr >> 16);
    ctx->tx_buf[2] = (uint8_t)(addr >> 8);
    ctx->tx_buf[3] = (uint8_t)addr;

    ctx->bus->cs(ctx->bus->ctx, true);
    if (!w25_tx(ctx, 4U)) // 命令+地址段
    {
        ctx->bus->cs(ctx->bus->ctx, false);
        ctx->dstate = DEV_RUN_ERROR;
        return false;
    }

    // 数据段：CS 保持，逐 chunk 收发（tx 全 0xFF 提供时钟）
    for (uint8_t i = 0U; i < READ_CHUNK; i++)
        ctx->rd_ff[i] = 0xFFU;

    uint32_t done = 0U;
    while (done < len)
    {
        uint32_t n = len - done;
        if (n > READ_CHUNK)
            n = READ_CHUNK;

        if (!ctx->bus->xfer(ctx->bus->ctx, ctx->rd_ff, data + done, (uint16_t)n))
        {
            ctx->bus->cs(ctx->bus->ctx, false);
            ctx->dstate = DEV_RUN_ERROR;
            return false;
        }
        done += n;
    }

    ctx->bus->cs(ctx->bus->ctx, false);
    ctx->dstate = DEV_RUNNING;
    return true;
}

// 单页编程（len <= 256），调用方保证已擦除
static bool w25_page_program(tW25Qxx_ctx *ctx, uint32_t addr, const uint8_t *data, uint16_t len)
{
    if (len == 0U || len > W25_PAGE_SIZE)
        return false;

    if (!w25_write_enable(ctx))
        return false;

    ctx->tx_buf[0] = FCMD_WRITE_PAGE;
    ctx->tx_buf[1] = (uint8_t)(addr >> 16);
    ctx->tx_buf[2] = (uint8_t)(addr >> 8);
    ctx->tx_buf[3] = (uint8_t)addr;
    for (uint16_t i = 0U; i < len; i++)
        ctx->tx_buf[4U + i] = data[i];

    ctx->bus->cs(ctx->bus->ctx, true);
    bool ok = ctx->bus->xfer(ctx->bus->ctx, ctx->tx_buf, ctx->rx_buf, (uint16_t)(4U + len));
    ctx->bus->cs(ctx->bus->ctx, false);

    if (!ok)
        return false;
    return w25_wait_idle(ctx, FTIMEOUT_OP_MS);
}

static bool w25_write(FlashChipHandle h, uint32_t addr, const uint8_t *data, uint32_t len)
{
    tW25Qxx_ctx *ctx = (tW25Qxx_ctx *)h;
    if (!ctx || !data || (addr + len) > W25_CAPACITY_BYTES)
        return false;

    // 自动跨页：每次从页边界截断写入
    uint32_t done = 0U;
    while (done < len)
    {
        uint32_t page_left = W25_PAGE_SIZE - ((addr + done) % W25_PAGE_SIZE);
        uint32_t n = len - done;
        if (n > page_left)
            n = page_left;

        if (!w25_page_program(ctx, addr + done, data + done, (uint16_t)n))
        {
            ctx->dstate = DEV_RUN_ERROR;
            return false;
        }
        done += n;
    }

    ctx->dstate = DEV_RUNNING;
    return true;
}

// 扇区擦除（4KB），地址须扇区对齐
static bool w25_erase_sector_at(tW25Qxx_ctx *ctx, uint32_t sector_addr)
{
    if (!w25_write_enable(ctx))
        return false;

    ctx->tx_buf[0] = FCMD_ERASE_SECTOR;
    ctx->tx_buf[1] = (uint8_t)(sector_addr >> 16);
    ctx->tx_buf[2] = (uint8_t)(sector_addr >> 8);
    ctx->tx_buf[3] = (uint8_t)sector_addr;

    ctx->bus->cs(ctx->bus->ctx, true);
    bool ok = ctx->bus->xfer(ctx->bus->ctx, ctx->tx_buf, ctx->rx_buf, 4U);
    ctx->bus->cs(ctx->bus->ctx, false);

    if (!ok)
        return false;
    return w25_wait_idle(ctx, FTIMEOUT_OP_MS);
}

static bool w25_erase(FlashChipHandle h, uint32_t addr, uint32_t len)
{
    tW25Qxx_ctx *ctx = (tW25Qxx_ctx *)h;
    if (!ctx || len == 0U || (addr + len) > W25_CAPACITY_BYTES)
        return false;

    // 按 4KB 扇区粒度向上取整覆盖 [addr, addr+len)
    uint32_t first = addr >> 12;
    uint32_t end = (addr + len + W25_SECTOR_SIZE - 1U) >> 12;

    for (uint32_t i = first; i < end; i++)
    {
        if (!w25_erase_sector_at(ctx, i << 12))
        {
            ctx->dstate = DEV_RUN_ERROR;
            return false;
        }
    }

    ctx->dstate = DEV_RUNNING;
    return true;
}

static uint32_t w25_get_capacity(FlashChipHandle h)
{
    tW25Qxx_ctx *ctx = (tW25Qxx_ctx *)h;
    return ctx ? W25_CAPACITY_BYTES : 0U;
}

static uint32_t w25_get_page_size(FlashChipHandle h)
{
    (void)h;
    return W25_PAGE_SIZE;
}

static uint32_t w25_get_sector_size(FlashChipHandle h)
{
    (void)h;
    return W25_SECTOR_SIZE;
}

static uint8_t w25_get_state(FlashChipHandle h)
{
    tW25Qxx_ctx *ctx = (tW25Qxx_ctx *)h;
    return (uint8_t)(ctx ? ctx->dstate : DEV_OFFLINE);
}

const tFlashDriverOps w25qxx_driver_ops = {
    .init = w25_init,
    .read = w25_read,
    .write = w25_write,
    .erase = w25_erase,
    .get_capacity = w25_get_capacity,
    .get_page_size = w25_get_page_size,
    .get_sector_size = w25_get_sector_size,
    .get_state = w25_get_state,
};

// ---- 句柄创建/销毁（资源注入点） ----

FlashChipHandle w25qxx_create(const tSpiBusIf *bus, const tTimeIf *time)
{
    if (!bus || !time)
        return NULL;

    tW25Qxx_ctx *ctx = (tW25Qxx_ctx *)calloc(1U, sizeof(tW25Qxx_ctx));
    if (!ctx)
        return NULL;

    ctx->dstate = DEV_OFFLINE;
    ctx->bus = bus;
    ctx->time = time;
    return (FlashChipHandle)ctx;
}

void w25qxx_destroy(FlashChipHandle h)
{
    free(h);
}
