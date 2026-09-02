#include "device.h"
#include "bsp_spi.h"

// ---------- 芯片参数 ----------
#define RESOLUTION 16384 // 输出 14-bit 角度（21-bit 原始 >> 7）
#define SPI_CPOL 1
#define SPI_CPHA 1
#define SPI_DATA_SIZE 8 // 8-bit 模式
#define NO_RESP_MAX 1000

// MT6835 连续读角度命令（5 字节）
static const uint8_t MT6835_CMD[5] = {0xA0, 0x03, 0x00, 0x00, 0x00};

// ---------- 上下文 ----------
typedef enum
{
    ST_IDLE,
    ST_WAIT,
    ST_ERR
} eMT6835_state;
typedef struct
{
    eDeviceStatus Dstatus; // 设备状态
    eEncoderType enc_type;
    volatile eMT6835_state state;
    volatile uint8_t rx_buf[5];  // 接收 5 字节
    volatile uint16_t raw_angle; // 解析后的 14-bit 角度
    uint32_t timestamp_ms;
    volatile bool data_ready;
    volatile uint16_t no_resp_tic;
    void (*set_cs)(bool active);
} tMT6835_ctx;

// ---------- 静态函数 ----------
static void MT6835_spi_cb(void *arg);
static bool MT6835_init(EncoderChipHandle handle, eEncoderType type);
static bool MT6835_get_resolution(EncoderChipHandle handle, uint16_t *res);
static bool MT6835_start_read(EncoderChipHandle handle);
static bool MT6835_is_data_ready(EncoderChipHandle handle);
static bool MT6835_get_raw_data(EncoderChipHandle handle, uint16_t *raw, uint32_t *timestamp_ms);
static void MT6835_reset(EncoderChipHandle handle);
static void MT6835_set_cs(EncoderChipHandle handle, bool active);
static uint8_t MT6835_get_Dstatus(EncoderChipHandle handle);
// ---------- 驱动操作表 ----------
tEncoderDriverOps MT6835_driver_ops = {
    .init = MT6835_init,
    .get_resolution = MT6835_get_resolution,
    .start_read = MT6835_start_read,
    .is_data_ready = MT6835_is_data_ready,
    .get_raw_data = MT6835_get_raw_data,
    .reset = MT6835_reset,
    .set_cs = MT6835_set_cs,
    .get_Dstate = MT6835_get_Dstatus, // 获取设备状态
};

// ---------- 创建/销毁 ----------
EncoderChipHandle MT6835_create(void)
{
    tMT6835_ctx *ctx = (tMT6835_ctx *)calloc(1, sizeof(tMT6835_ctx));
    return (EncoderChipHandle)ctx;
}

void MT6835_destroy(EncoderChipHandle handle)
{
    free(handle);
    handle = NULL;
}

// ========== 实现 ==========
static bool MT6835_init(EncoderChipHandle handle, eEncoderType type)
{
    tMT6835_ctx *ctx = (tMT6835_ctx *)handle;
    if (!ctx)
        return false;

    ctx->enc_type = type;
    // 配置 SPI 为 Mode 3 (CPOL=1, CPHA=1), 8-bit
    // if (!bsp_change_encoder_spi_config(SPI_CPOL, SPI_CPHA, SPI_DATA_SIZE)) return false;

    bsp_encoder_register_callback(MT6835_spi_cb, ctx);
    ctx->state = ST_IDLE;
    ctx->data_ready = false;
    ctx->raw_angle = 0;

    if (type == ENC_INTERNAL)
        ctx->set_cs = bsp_int_encoder_cs;
    else
        ctx->set_cs = bsp_ext_encoder_cs;

    return true;
}

static bool MT6835_get_resolution(EncoderChipHandle handle, uint16_t *res)
{
    tMT6835_ctx *ctx = (tMT6835_ctx *)handle;
    if (!ctx || !res)
        return false;
    *res = RESOLUTION;
    return true;
}

static bool MT6835_start_read(EncoderChipHandle handle)
{
    tMT6835_ctx *ctx = (tMT6835_ctx *)handle;
    if (!ctx || ctx->state != ST_IDLE)
        return false;

    if (!bsp_encoder_spi_is_ready())
    {
        bsp_encoder_spi_abort();
        return false;
    }

    ctx->set_cs(true);
    ctx->no_resp_tic = 0;
    // 发送 5 字节命令，同时接收 5 字节数据
    if (!bsp_encoder_spi_transmit_receive_dma((uint8_t *)MT6835_CMD,
                                              (uint8_t *)ctx->rx_buf, 5))
    {
        ctx->set_cs(false);
        return false;
    }
    ctx->state = ST_WAIT;
    return true;
}

static bool MT6835_is_data_ready(EncoderChipHandle handle)
{
    tMT6835_ctx *ctx = (tMT6835_ctx *)handle;
    return ctx ? ctx->data_ready : false;
}

static bool MT6835_get_raw_data(EncoderChipHandle handle, uint16_t *raw, uint32_t *timestamp_ms)
{
    tMT6835_ctx *ctx = (tMT6835_ctx *)handle;
    if (!ctx || !ctx->data_ready)
        return false;

    // 解析 21-bit 角度并转为 14-bit
    uint32_t angle_21 = ((uint32_t)ctx->rx_buf[2] << 13) |
                        ((uint32_t)ctx->rx_buf[3] << 5) |
                        (ctx->rx_buf[4] >> 3);

    // 检查 STATUS 位 (rx_buf[4] bit0-2)
    uint8_t status = ctx->rx_buf[4] & 0x07;
    if (status & 0x02)
    { // 磁场太弱
        return false;
    }

    *raw = (uint16_t)(angle_21 >> 7); // 取高 14 位
    *timestamp_ms = ctx->timestamp_ms;
    ctx->data_ready = false;
    return true;
}

static void MT6835_reset(EncoderChipHandle handle)
{
    tMT6835_ctx *ctx = (tMT6835_ctx *)handle;
    if (ctx)
    {
        ctx->state = ST_IDLE;
        ctx->data_ready = false;
        ctx->set_cs(false);
        bsp_encoder_spi_abort();
    }
}

static void MT6835_set_cs(EncoderChipHandle handle, bool active)
{
    tMT6835_ctx *ctx = (tMT6835_ctx *)handle;
    ctx->set_cs(active);
}

// ---------- 回调（单次传输完成） ----------
static void MT6835_spi_cb(void *arg)
{
    tMT6835_ctx *ctx = (tMT6835_ctx *)arg;
    if (!ctx)
        return;

    if (ctx->state == ST_WAIT)
    {
        ctx->timestamp_ms = bsp_get_tick();
        ctx->set_cs(false); // 传输结束，拉高 CS
        ctx->data_ready = true;
        ctx->state = ST_IDLE;
    }
    else
    {
        ctx->state = ST_IDLE;
        ctx->data_ready = false;
        ctx->set_cs(false);
    }
}

static uint8_t MT6835_get_Dstatus(EncoderChipHandle handle)
{
    if (NULL == handle)
        return 0;
    return ((tMT6835_ctx *)handle)->Dstatus; // 返回设备状态
}