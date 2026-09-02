#include "device.h"
#include "bsp_spi.h"

// MT6816参数配置
#define RESOLUTION 16384    // 单圈分辨率
#define CMD_HIGH 0x83FF     // 高位命令
#define CMD_LOW 0x84FF      // 低位命令
#define SPI_CPOL 1          // SPI CPOL
#define SPI_CPHA 1          // SPI CPHA
#define SPI_DATA_SIZE 16    // SPI 数据大小
#define NO_RESP_MAX 1000    // 无响应重启次数
#define MAG_WARN (1 << 1)   // 磁场失效位
#define PARITY_BIT (1 << 0) // 校验位

// MT6816 驱动上下文
typedef enum
{
    ST_IDLE,
    ST_WAIT_HIGH,
    ST_WAIT_LOW,
    ST_ERR
} eMT6816_state;

typedef struct
{
    eDeviceStatus Dstatus; // 设备状态
    eEncoderType enc_type;
    volatile eMT6816_state state;
    uint16_t cmd_high, cmd_low;
    volatile uint16_t shadow_raw[2];
    volatile uint16_t data_raw[2];
    uint32_t timestamp_ms;
    volatile bool data_ready;
    volatile uint16_t no_resp_tic;
    void (*set_cs)(bool active);
} tMT6816_ctx;

// MT6816 SPI DMA 回调函数
static void MT6816_spi_cb(void *arg);

// MT6816 函数操作 声明
bool MT6816_init(EncoderChipHandle handle, eEncoderType type);
bool MT6816_get_resolution(EncoderChipHandle handle, uint16_t *res);
bool MT6816_start_read(EncoderChipHandle handle);
bool MT6816_is_data_ready(EncoderChipHandle handle);
bool MT6816_get_raw_data(EncoderChipHandle handle, uint16_t *raw, uint32_t *timestamp_ms);
void MT6816_reset(EncoderChipHandle handle);
void MT6816_set_cs(EncoderChipHandle handle, bool active);
uint8_t MT6816_get_Dstatus(EncoderChipHandle handle);

tEncoderDriverOps MT6816_driver_ops = {
    .init = MT6816_init,
    .get_resolution = MT6816_get_resolution,
    .start_read = MT6816_start_read,
    .is_data_ready = MT6816_is_data_ready,
    .get_raw_data = MT6816_get_raw_data,
    .reset = MT6816_reset,
    .set_cs = MT6816_set_cs,
    .get_Dstate = MT6816_get_Dstatus // 获取设备状态
};
// 驱动句柄创建 销毁

// 新建句柄
EncoderChipHandle MT6816_create(void)
{
    tMT6816_ctx *ctx = (tMT6816_ctx *)calloc(1, sizeof(tMT6816_ctx));
    return ctx;
}
// 销毁驱动句柄
void MT6816_destroy(EncoderChipHandle handle)
{
    free(handle);
    handle = NULL;
}

// MT6816 函数操作 实现
static bool MT6816_init(EncoderChipHandle handle, eEncoderType type)
{
    tMT6816_ctx *ctx = (tMT6816_ctx *)handle;
    if (!ctx)
    {
        ctx->Dstatus = OFFLINE;
        return false;
    }
    ctx->enc_type = type;
    // 配置 SPI 为 Mode 3 (CPOL=1, CPHA=1), 16bit
    // 和默认配置一样 不需要改动
    // if (!bsp_change_encoder_spi_config(SPI_CPOL, SPI_CPHA, SPI_DATA_SIZE))
    //     return false;
    bsp_encoder_register_callback(MT6816_spi_cb, ctx);
    ctx->state = ST_IDLE;
    ctx->data_ready = false;
    ctx->cmd_high = CMD_HIGH;
    ctx->cmd_low = CMD_LOW;

    if (type == ENC_INTERNAL)
        ctx->set_cs = bsp_int_encoder_cs;
    else
        ctx->set_cs = bsp_ext_encoder_cs;
    ctx->Dstatus = ONLINE;
    return true;
}

static bool MT6816_get_resolution(EncoderChipHandle handle, uint16_t *res)
{
    tMT6816_ctx *ctx = (tMT6816_ctx *)handle;
    if (!ctx || !res)
        return false;
    *res = RESOLUTION;
    return true;
}

static bool MT6816_start_read(EncoderChipHandle handle)
{
    tMT6816_ctx *ctx = (tMT6816_ctx *)handle;
    if (!ctx || ctx->state != ST_IDLE)
        return false;
    if (!bsp_encoder_spi_is_ready())
    {
        bsp_encoder_spi_abort();
        return false;
    }
    ctx->set_cs(true);
    ctx->no_resp_tic = 0;
    if (!bsp_encoder_spi_transmit_receive_dma((uint8_t *)&ctx->cmd_high, (uint8_t *)&ctx->shadow_raw[0], 2))
    {
        ctx->set_cs(false);
        return false;
    }
    ctx->state = ST_WAIT_HIGH;
    return true;
}

static bool MT6816_is_data_ready(EncoderChipHandle handle)
{
    tMT6816_ctx *ctx = (tMT6816_ctx *)handle;
    return ctx ? ctx->data_ready : false;
}

static bool MT6816_get_raw_data(EncoderChipHandle handle, uint16_t *raw, uint32_t *timestamp_ms)
{
    tMT6816_ctx *ctx = (tMT6816_ctx *)handle;
    if (!ctx || !ctx->data_ready)
        return false;

    uint16_t high = ctx->data_raw[0];
    uint16_t low = ctx->data_raw[1];

    // 奇偶校验（芯片协议）
    bool parity = (low & PARITY_BIT) ? true : false;
    uint16_t bits = (high & 0x00FF) << 7 | ((low & 0x00FE) >> 1);
    if ((__builtin_popcount(bits) % 2 == 1) != parity)
        return false;

    *raw = bits >> 1;
    *timestamp_ms = ctx->timestamp_ms;
    ctx->data_ready = false;
    return true;
}

static void MT6816_reset(EncoderChipHandle handle)
{
    tMT6816_ctx *ctx = (tMT6816_ctx *)handle;
    if (ctx)
    {
        ctx->state = ST_IDLE;
        ctx->data_ready = false;
        ctx->set_cs(false);
        bsp_encoder_spi_abort();
    }
}

static void MT6816_set_cs(EncoderChipHandle handle, bool active)
{
    tMT6816_ctx *ctx = (tMT6816_ctx *)handle;
    ctx->set_cs(active);
}
// ---------- 回调（状态机） ----------
static void MT6816_spi_cb(void *arg)
{
    tMT6816_ctx *ctx = (tMT6816_ctx *)arg;
    if (!ctx)
        return;

    if (ctx->state == ST_WAIT_HIGH)
    {
        if (bsp_encoder_spi_is_ready())
        {
            ctx->set_cs(true);
            ctx->no_resp_tic = 0;
            if (bsp_encoder_spi_transmit_receive_dma((uint8_t *)&ctx->cmd_low, (uint8_t *)&ctx->shadow_raw[1], 2))
            {
                ctx->state = ST_WAIT_LOW;
                return;
            }
        }
        ctx->state = ST_ERR;
    }
    else if (ctx->state == ST_WAIT_LOW)
    {
        ctx->timestamp_ms = bsp_get_tick();
        ctx->data_raw[0] = ctx->shadow_raw[0];
        ctx->data_raw[1] = ctx->shadow_raw[1];
        if (ctx->data_raw[1] & MAG_WARN)
        {
            ctx->state = ST_ERR;
            return;
        }
        ctx->data_ready = true;
        ctx->state = ST_IDLE;

        ctx->Dstatus = ONLINE;
    }
    else
    {
        ctx->Dstatus = RUN_ERROR;
        ctx->state = ST_IDLE;
        ctx->data_ready = false;
    }
}

uint8_t MT6816_get_Dstatus(EncoderChipHandle handle)
{
    if (NULL == handle)
        return 0;
    return ((tMT6816_ctx *)handle)->Dstatus; // 获取设备状态
}