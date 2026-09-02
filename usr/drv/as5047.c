#include "device.h"

#include "bsp_spi.h"

// ---------- 驱动参数配置 ----------
#define RESOLUTION 16384
#define CMD_READ_ANGLE 0x7FFF // 读角度寄存器命令
#define CMD_NOP 0x0000        // NOP 命令（用于读取数据）
#define SPI_CPOL 0
#define SPI_CPHA 1
#define SPI_DATA_SIZE 16
#define NO_RESP_MAX 1000     // 超时计数
#define ERR_FLAG_MASK 0x4000 // bit14 错误标志

// ---------- 上下文结构 ----------
typedef enum
{
    ST_IDLE,
    ST_WAIT_HIGH,
    ST_WAIT_LOW,
    ST_ERR
} eAS5047_state;
typedef struct
{
    eDeviceStatus Dstatus; // 设备状态
    eEncoderType enc_type;
    volatile eAS5047_state state;
    uint16_t cmd_high; // 读角度命令
    uint16_t cmd_low;  // NOP 命令
    volatile uint16_t shadow_raw[2];
    volatile uint16_t data_raw[2];
    uint32_t timestamp_ms;
    volatile bool data_ready;
    volatile uint16_t no_resp_tic;
    void (*set_cs)(bool active);
} tAS5047_ctx;

// ---------- 驱动回调 声明----------
static void AS5047_spi_cb(void *arg);
// ---------- 驱动操作表 声明----------
static bool AS5047_init(EncoderChipHandle handle, eEncoderType type);
static bool AS5047_get_resolution(EncoderChipHandle handle, uint16_t *res);
static bool AS5047_start_read(EncoderChipHandle handle);
static bool AS5047_is_data_ready(EncoderChipHandle handle);
static bool AS5047_get_raw_data(EncoderChipHandle handle, uint16_t *raw, uint32_t *timestamp_ms);
static void AS5047_reset(EncoderChipHandle handle);
static void AS5047_set_cs(EncoderChipHandle handle, bool active);
static uint8_t AS5047_get_Dstate(EncoderChipHandle handle);

tEncoderDriverOps AS5047_driver_ops = {
    .init = AS5047_init,
    .get_resolution = AS5047_get_resolution,
    .start_read = AS5047_start_read,
    .is_data_ready = AS5047_is_data_ready,
    .get_raw_data = AS5047_get_raw_data,
    .reset = AS5047_reset,
    .set_cs = AS5047_set_cs,
    .get_Dstate = AS5047_get_Dstate};

// ---------- 句柄 创建/销毁 ----------
EncoderChipHandle AS5047_create(void)
{
    tAS5047_ctx *ctx = (tAS5047_ctx *)calloc(1, sizeof(tAS5047_ctx));
    return (EncoderChipHandle)ctx;
}

void AS5047_destroy(EncoderChipHandle handle)
{
    free(handle);
    handle = NULL;
}

// ---------- 驱动操作表 定义----------
static bool AS5047_init(EncoderChipHandle handle, eEncoderType type)
{
    tAS5047_ctx *ctx = (tAS5047_ctx *)handle;
    if (!ctx)
    {
        ctx->Dstatus = OFFLINE;
        return false;
    }

    ctx->enc_type = type;
    // 配置 SPI 模式 (Mode 1: CPOL=0, CPHA=1)
    // 如果 BSP 需要显式配置，取消注释下面一行
    if (!bsp_change_encoder_spi_config(SPI_CPOL, SPI_CPHA, SPI_DATA_SIZE))
    {
        ctx->Dstatus = OFFLINE;
        return false;
    }

    bsp_encoder_register_callback(AS5047_spi_cb, ctx);
    ctx->state = ST_IDLE;
    ctx->data_ready = false;
    ctx->cmd_high = CMD_READ_ANGLE;
    ctx->cmd_low = CMD_NOP;

    // 根据类型选择 CS 控制函数
    if (type == ENC_INTERNAL)
        ctx->set_cs = bsp_int_encoder_cs;
    else
        ctx->set_cs = bsp_ext_encoder_cs;

    ctx->Dstatus = ONLINE;
    return true;
}

static bool AS5047_get_resolution(EncoderChipHandle handle, uint16_t *res)
{
    tAS5047_ctx *ctx = (tAS5047_ctx *)handle;
    if (!ctx || !res)
        return false;
    *res = RESOLUTION;
    return true;
}

static bool AS5047_start_read(EncoderChipHandle handle)
{
    tAS5047_ctx *ctx = (tAS5047_ctx *)handle;
    if (!ctx || ctx->state != ST_IDLE)
        return false;

    if (!bsp_encoder_spi_is_ready())
    {
        bsp_encoder_spi_abort();
        return false;
    }

    ctx->set_cs(true); // 拉低 CS 开始
    ctx->no_resp_tic = 0;

    // 发送读角度命令（第一次传输）
    if (!bsp_encoder_spi_transmit_receive_dma((uint8_t *)&ctx->cmd_high,
                                              (uint8_t *)&ctx->shadow_raw[0], 2))
    {
        ctx->set_cs(false); // 失败则拉高 CS
        return false;
    }
    ctx->state = ST_WAIT_HIGH;
    return true;
}

static bool AS5047_is_data_ready(EncoderChipHandle handle)
{
    tAS5047_ctx *ctx = (tAS5047_ctx *)handle;
    return ctx ? ctx->data_ready : false;
}

static bool AS5047_get_raw_data(EncoderChipHandle handle, uint16_t *raw, uint32_t *timestamp_ms)
{
    tAS5047_ctx *ctx = (tAS5047_ctx *)handle;
    if (!ctx || !ctx->data_ready)
        return false;

    uint16_t angle = ctx->data_raw[1]; // 第二次接收的数据（NOP 响应）

    // 检查错误标志 (bit14)
    if (angle & ERR_FLAG_MASK)
        return false;

    *raw = angle & 0x3FFF; // 低14位有效角度
    *timestamp_ms = ctx->timestamp_ms;
    ctx->data_ready = false;
    return true;
}

static void AS5047_reset(EncoderChipHandle handle)
{
    tAS5047_ctx *ctx = (tAS5047_ctx *)handle;
    if (ctx)
    {
        ctx->state = ST_IDLE;
        ctx->data_ready = false;
        ctx->set_cs(false); // 拉高 CS
        bsp_encoder_spi_abort();
    }
}

static void AS5047_set_cs(EncoderChipHandle handle, bool active)
{
    tAS5047_ctx *ctx = (tAS5047_ctx *)handle;
    ctx->set_cs(active);
}

// ---------- DMA 回调（状态机） ----------
static void AS5047_spi_cb(void *arg)
{
    tAS5047_ctx *ctx = (tAS5047_ctx *)arg;
    if (!ctx)
        return;

    if (ctx->state == ST_WAIT_HIGH)
    {
        // 第一次传输完成：保持 CS 低，发送 NOP 获取有效数据
        if (bsp_encoder_spi_is_ready())
        {
            ctx->set_cs(true); // 保持 CS 低（实际已是低，显式调用确保）
            ctx->no_resp_tic = 0;
            if (bsp_encoder_spi_transmit_receive_dma((uint8_t *)&ctx->cmd_low,
                                                     (uint8_t *)&ctx->shadow_raw[1], 2))
            {
                ctx->state = ST_WAIT_LOW;
                return;
            }
        }
        ctx->state = ST_ERR;
    }
    else if (ctx->state == ST_WAIT_LOW)
    {
        ctx->Dstatus = RUNNING;
        // 第二次传输完成，数据就绪
        ctx->timestamp_ms = bsp_get_tick();
        ctx->data_raw[0] = ctx->shadow_raw[0]; // 第一次无用数据
        ctx->data_raw[1] = ctx->shadow_raw[1]; // 有效角度
        ctx->set_cs(false);                    // 拉高 CS，结束
        ctx->data_ready = true;
        ctx->state = ST_IDLE;
    }
    else
    {
        ctx->Dstatus = RUN_ERROR;
        // 异常状态恢复
        ctx->state = ST_IDLE;
        ctx->data_ready = false;
        ctx->set_cs(false);
    }
}

static uint8_t AS5047_get_Dstate(EncoderChipHandle handle)
{
    tAS5047_ctx *ctx = (tAS5047_ctx *)handle;
    if (!ctx)
        return;
    return ctx->Dstatus;
}
