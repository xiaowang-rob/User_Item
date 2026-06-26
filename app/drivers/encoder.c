// ============================================================
// 编码器抽象驱动 (支持 MT6816 / AS5047 / MT6835)
// 使用全局实例 g_encoder，初始化时选择芯片型号
// ============================================================

#include "device.h"
#include "bsp_base.h"
#include "string.h"
#include "math_fast.h"

#ifdef __DEBUG__
volatile u32 encoder_ts_us = 0;
u32 time_zero;
#endif

// ----------------------- 类型定义 -------------------------
typedef enum
{
    ENCODER_STATE_START_READ,
    ENCODER_STATE_WAIT_HIGH,
    ENCODER_STATE_WAIT_LOW,
    ENCODER_STATE_PROCESS_DATA,
    ENCODER_STATE_START_LOW, // MT6816 专用：准备读低字节
    ENCODER_STATE_START_NOP  // AS5047 专用：准备发 NOP 命令
} eEncoderState_DMA;

// ----------------------- 全局实例 -------------------------
static tEncoderInstance enc = {.chip_type = 0xff};

// ----------------------- 常量定义 -------------------------
#define NO_RESPONSE_MAX_TIC 1000 // 1000 * 10us = 10ms 超时
#define MT6816_NO_MAG_WARNING (1 << 1)
#define MT6816_PARITY_CHECK (1 << 0)

// ----------------------- 芯片描述表 -----------------------
static bool mt6816_parse_and_check(uint16_t high, uint16_t low, uint16_t *angle);
static bool as5047_parse_and_check(uint16_t high, uint16_t low, uint16_t *angle);
static bool mt6835_parse_and_check(uint16_t high, uint16_t low, uint16_t *angle);
static void mt6816_main_loop(void *arg);
static void as5047_main_loop(void *arg);
static void mt6835_main_loop(void *arg);

// MT6835: 连续读角度 5-byte 命令 + 接收缓冲
static uint8_t mt6835_rx_buf[5] __attribute__((aligned(4)));
static const uint8_t mt6835_cmd_buf[5] __attribute__((aligned(4))) = {0xA0, 0x03, 0x00, 0x00, 0x00};

static const tEncoderChipDesc chip_descs[CHIP_COUNT] = {
    [MT6816] = {
        .resolution = 16384,
        .deg_per_lsb = 360.0f / 16384.0f,
        .cmd_high = 0x83FF,
        .cmd_low = 0x84FF,
        .cmd_read_angle = 0,
        .spi_CPOL = 1,
        .spi_CPHA = 1,
        .spi_data_size = 16,
        .parse_and_check = mt6816_parse_and_check,
        .use_dma_state_machine = true,
        .dma_post_high_state = ENCODER_STATE_START_LOW,
        .dma_state_entry = mt6816_main_loop,
    },
    [AS5047] = {
        .resolution = 16384,
        .deg_per_lsb = 360.0f / 16384.0f,
        .cmd_high = 0,
        .cmd_low = 0,
        .cmd_read_angle = 0x7FFF, // 读角度寄存器命令
        .spi_CPOL = 0,
        .spi_CPHA = 1,
        .spi_data_size = 16,
        .parse_and_check = as5047_parse_and_check,
        .use_dma_state_machine = true,
        .dma_post_high_state = ENCODER_STATE_START_NOP,
        .dma_state_entry = as5047_main_loop,
    },
    [MT6835] = {
         .resolution = 16384, // 14-bit 输出 (21-bit 原始 >> 7)
        .deg_per_lsb = 360.0f / 2097152.0f,
        .cmd_high = 0,
        .cmd_low = 0,
        .cmd_read_angle = 0,
        .spi_CPOL = 1, // Mode 3
        .spi_CPHA = 1,
         .spi_data_size = 8, // 8-bit 模式
        .parse_and_check = mt6835_parse_and_check,
        .use_dma_state_machine = true,
         .dma_post_high_state = ENCODER_STATE_PROCESS_DATA, // 单次交易完成即处理
        .dma_state_entry = mt6835_main_loop,
    },
};

// ------------------ 内部函数前向声明 ---------------------
static void encoder_recover_from_error(void);
static void common_angle_velocity_update(void);

static void mt6816_start_high_read(void);
static void mt6816_start_low_read(void);
static void mt6816_process_data(void);

static void as5047_start_command(void);
static void as5047_start_nop(void);
static void as5047_process_data(void);

// ========== 初始化 ==========
bool encoder_init(eEncoderChip type)
{
    if (type >= CHIP_COUNT)
        return false;

    if (type == enc.chip_type)
    {
        g_device_status.encoder_state = ONLINE;
        return true;
    }

    // 清空全局实例并赋初值
    memset((void *)&enc, 0, sizeof(enc));
    enc.chip_desc = &chip_descs[type];
    enc.chip_type = type;
    enc.state = ENCODER_STATE_START_READ;
    enc.first_run = true;
    enc.valid = 0;
    enc.rubbish_data_tic = 0;

    // 根据型号动态配置 SPI3 模式 (CPOL/CPHA)
    if (false == bsp_set_encoder_spi_config(enc.chip_desc->spi_CPOL, enc.chip_desc->spi_CPHA, enc.chip_desc->spi_data_size))
    {
        g_device_status.encoder_state = OFFLINE;
        return false;
    }
    g_device_status.encoder_state = ONLINE;

    return true;
}

// ========== 错误恢复 ==========
static void encoder_recover_from_error(void)
{
    bsp_encoder_cs(BSP_SPI_EXTERNAL, true);
    if (!bsp_encoder_spi_is_ready())
    {
        bsp_encoder_spi_abort();
        g_device_status.encoder_state = RUN_ERROR;
    }
    bsp_encoder_spi_clear_dma_error_flags();
    g_device_status.encoder_state = ONLINE;
    enc.state = ENCODER_STATE_START_READ;
}

void encode_clear_error_flag(void)
{
    g_device_status.encoder_state = ONLINE;
}
// ========== DMA 完成回调（不再启动任何 SPI 传输） ==========
void bsp_encoder_spi_txrx_cplt_callback(void)
{
    bsp_encoder_cs(BSP_SPI_EXTERNAL, true);

    if (enc.state == ENCODER_STATE_WAIT_HIGH)
    {
        enc.state = enc.chip_desc->dma_post_high_state;
        enc.no_resp_tic = 0;
    }
    else if (enc.state == ENCODER_STATE_WAIT_LOW)
    {
        bsp_disable_irq();
        enc.data_raw[0] = enc.shadow_raw[0];
        enc.data_raw[1] = enc.shadow_raw[1];
        bsp_enable_irq();
        enc.state = ENCODER_STATE_PROCESS_DATA;
        enc.no_resp_tic = 0;
    }
}

// ========== DMA 错误回调 ==========
void bsp_encoder_spi_error_callback(void)
{
    encoder_recover_from_error();
}

// PLL 更新：给定观测角度 (deg)，更新 PLL 估计角度和速度
void encoder_pll_update(float dt)
{
    enc.pll_theta_delta = enc.angle_abs - enc.pll_theta;
    // 角度归一化到 [-180, 180)
    enc.pll_theta_delta = normalize_angle_pi(enc.pll_theta_delta);
#define INTEG_LIMIT 10.0f
    enc.pll_integ += enc.pll_theta_delta * dt;
     enc.pll_integ = CLAMP(enc.pll_integ, -INTEG_LIMIT, INTEG_LIMIT); // 限幅

    float estimated_speed_deg_s = enc.pll_kp * enc.pll_theta_delta + enc.pll_ki * enc.pll_integ;

    enc.pll_theta += estimated_speed_deg_s * dt;
    enc.pll_theta = normalize_angle_360(enc.pll_theta);

    enc.pll_omega_rpm = estimated_speed_deg_s / 6.0f;
    if (FABSF(enc.pll_omega_rpm) <= 0.5f)
        enc.pll_omega_rpm = 0.0f;
}

// ========== 通用角度/速度更新 (所有芯片共用) ==========
static void common_angle_velocity_update()
{
    g_device_status.encoder_state = RUNNING;
    enc.angle_abs = enc.angle_raw * enc.chip_desc->deg_per_lsb;

#ifdef __DEBUG__
    volatile u32 current_time_us = bsp_get_tick_us();
    encoder_ts_us = current_time_us - time_zero;
    time_zero = current_time_us;
#endif

    if (enc.first_run)
    {
        enc.angle_raw_last = enc.angle_raw;
        enc.num_turns = 0;
        enc.pos_offset = enc.angle_raw;
        enc.omega_rpm = 0;
        enc.pos = 0;
        enc.pos_last = 0;
        // PLL 初始化
        enc.pll_theta = enc.angle_abs;
        enc.pll_omega_rpm = 0;
        // Kp = 2 * ζ * ω_n，Ki = ω_n² (ω_n取决于 采样频率 电机最大速度 速度环带宽）
        enc.pll_kp = 80.0f;
        enc.pll_ki = 2000.0f;
        enc.pll_integ = 0.0f;

        if (enc.rubbish_data_tic++ > 3)
        {
            enc.rubbish_data_tic = 0;
            enc.first_run = false;
        }
    }
    else
    {
        // PLL 发散检测：速度饱和时重置PLL
        if (enc.pll_omega_rpm >= 99999.0f || enc.pll_omega_rpm <= -99999.0f)
        {
            enc.pll_integ = 0.0f;
            enc.pll_omega_rpm = 0.0f;
            enc.pll_theta = enc.angle_abs;
        }

        int32_t angle_raw_delta = (int32_t)enc.angle_raw - enc.angle_raw_last;

        // 不需要高转速的检测 位置模式下不可能高转速
        if (angle_raw_delta < -8192)
            enc.num_turns++;
        else if (angle_raw_delta > 8192)
            enc.num_turns--;

        enc.pos = enc.chip_desc->deg_per_lsb * (int32_t)(enc.angle_raw - enc.pos_offset) + enc.num_turns * 360.0f;
        enc.angle_raw_last = enc.angle_raw;
    }
}

// ====== MT6816 驱动 ======
static bool mt6816_parse_and_check(uint16_t high, uint16_t low, uint16_t *angle)
{
    bool parity_check = (low & MT6816_PARITY_CHECK) ? true : false;
    uint16_t data_bits = (high & 0x00FF) << 7 | ((low & 0x00FE) >> 1);
    uint8_t bit_count = __builtin_popcount(data_bits);
    bool expected_parity = (bit_count % 2 == 1);

    if (expected_parity != parity_check)
        return false;
    *angle = data_bits >> 1; // 14-bit 原始角度
    return true;
}

static void mt6816_start_high_read(void)
{
    if (!bsp_encoder_spi_is_ready())
    {
        encoder_recover_from_error();
        return;
    }
    bsp_encoder_cs(BSP_SPI_EXTERNAL, false);

    enc.cmd_reg = enc.chip_desc->cmd_high;
    if (!bsp_encoder_spi_transmit_receive_dma((u8 *)&enc.cmd_reg,
                                             (u8 *)&enc.shadow_raw[0], 2))
    {
        encoder_recover_from_error();
        return;
    }
    enc.state = ENCODER_STATE_WAIT_HIGH;
}

static void mt6816_start_low_read(void)
{
    if (!bsp_encoder_spi_is_ready())
    {
        encoder_recover_from_error();
        return;
    }
    bsp_encoder_cs(BSP_SPI_EXTERNAL, false);

    enc.cmd_reg = enc.chip_desc->cmd_low;
    if (!bsp_encoder_spi_transmit_receive_dma((u8 *)&enc.cmd_reg,
                                             (u8 *)&enc.shadow_raw[1], 2))
    {
        encoder_recover_from_error();
        return;
    }
    enc.state = ENCODER_STATE_WAIT_LOW;
}

static void mt6816_process_data(void)
{
    uint16_t angle_raw;

    // 弱磁检测
    if (enc.data_raw[1] & MT6816_NO_MAG_WARNING)
    {
        g_device_status.encoder_state = OFFLINE;
        enc.state = ENCODER_STATE_START_READ;
        return;
    }

    // 奇偶校验及数据提取
    if (!enc.chip_desc->parse_and_check(enc.data_raw[0], enc.data_raw[1], &angle_raw))
    {
        enc.valid = CLAMP(enc.valid + 10, 0, 110);
        enc.spi_error_rate += (1.0f - enc.spi_error_rate) * 0.01f; // EMA
        if (enc.valid > 100)
        {
            g_device_status.encoder_state = RUN_ERROR;
            enc.valid = 0;
        }
        enc.state = ENCODER_STATE_START_READ;
        return;
    }

    enc.valid = CLAMP(enc.valid - 1, 0, 110);
     enc.spi_error_rate *= 0.999f; // 成功时衰减
    enc.angle_raw = angle_raw;
    common_angle_velocity_update();
    enc.state = ENCODER_STATE_START_READ;
}

static void mt6816_main_loop(void *arg)
{
    tEncoderInstance *enc = (tEncoderInstance *)arg;
    switch (enc->state)
    {
    case ENCODER_STATE_START_READ:
        enc->no_resp_tic = 0;
        mt6816_start_high_read();
        break;

    case ENCODER_STATE_START_LOW:
        // 启动低字节读取 (高字节已就绪)
        mt6816_start_low_read();
        break;

    case ENCODER_STATE_WAIT_HIGH:
    case ENCODER_STATE_WAIT_LOW:
        enc->no_resp_tic++;
        if (enc->no_resp_tic > NO_RESPONSE_MAX_TIC)
            enc->state = ENCODER_STATE_START_READ;
        break;

    case ENCODER_STATE_PROCESS_DATA:
        mt6816_process_data();
        break;

    default:
        encoder_recover_from_error();
        break;
    }
}

// ====== AS5047 驱动 ======
static bool as5047_parse_and_check(uint16_t raw, uint16_t unused, uint16_t *angle)
{
    (void)unused;
    // 检查错误标志 (EF, bit14)
    if (raw & 0x4000)
        return false;

    // 提取14位角度数据
    *angle = raw & 0x3FFF;
    return true;
}

static void as5047_start_command(void)
{
    if (!bsp_encoder_spi_is_ready())
    {
        encoder_recover_from_error();
        return;
    }
    bsp_encoder_cs(BSP_SPI_EXTERNAL, false);

    enc.cmd_reg = enc.chip_desc->cmd_read_angle;
    if (!bsp_encoder_spi_transmit_receive_dma((u8 *)&enc.cmd_reg,
                                             (u8 *)&enc.shadow_raw[0], 2))
    {
        encoder_recover_from_error();
        return;
    }
    enc.state = ENCODER_STATE_WAIT_HIGH;
}

static void as5047_start_nop(void)
{
    if (!bsp_encoder_spi_is_ready())
    {
        encoder_recover_from_error();
        return;
    }
    bsp_encoder_cs(BSP_SPI_EXTERNAL, false);

     enc.cmd_reg = 0x0000; // NOP 命令
    if (!bsp_encoder_spi_transmit_receive_dma((u8 *)&enc.cmd_reg,
                                             (u8 *)&enc.shadow_raw[0], 2))
    {
        encoder_recover_from_error();
        return;
    }
    enc.state = ENCODER_STATE_WAIT_LOW;
}

static void as5047_process_data(void)
{

    uint16_t angle_raw;

    if (!enc.chip_desc->parse_and_check(enc.data_raw[0], 0, &angle_raw))
    {
        enc.valid = CLAMP(enc.valid + 10, 0, 110);
        enc.spi_error_rate += (1.0f - enc.spi_error_rate) * 0.01f; // EMA
        if (enc.valid > 100)
        {
            g_device_status.encoder_state = RUN_ERROR;
            enc.valid = 0;
        }
        enc.state = ENCODER_STATE_START_READ;
        return;
    }

    enc.valid = CLAMP(enc.valid - 1, 0, 110);
     enc.spi_error_rate *= 0.999f; // 成功时衰减

    enc.angle_raw = angle_raw;
    common_angle_velocity_update();
    enc.state = ENCODER_STATE_START_READ;
}

static void as5047_main_loop(void *arg)
{
    tEncoderInstance *enc = (tEncoderInstance *)arg;
    switch (enc->state)
    {
    case ENCODER_STATE_START_READ:
        enc->no_resp_tic = 0;
        as5047_start_command();
        break;

    case ENCODER_STATE_START_NOP:
        // 命令帧已发送，现在发送 NOP 接收数据
        as5047_start_nop();
        break;

    case ENCODER_STATE_WAIT_HIGH:
    case ENCODER_STATE_WAIT_LOW:
        enc->no_resp_tic++;
        if (enc->no_resp_tic > NO_RESPONSE_MAX_TIC)
            enc->state = ENCODER_STATE_START_READ;
        break;

    case ENCODER_STATE_PROCESS_DATA:
        as5047_process_data();
        break;

    default:
        encoder_recover_from_error();
        break;
    }
}

// ====== MT6835 驱动 (SPI Mode 3, 8-bit, 连续读角度) ======
static bool mt6835_parse_and_check(uint16_t raw, uint16_t unused, uint16_t *angle)
{
    (void)unused;
    // mt6835_rx_buf[2] = ANGLE[20:13], [3] = ANGLE[12:5], [4] = ANGLE[4:0] + STATUS[2:0]
    uint32_t angle_21 = ((uint32_t)mt6835_rx_buf[2] << 13) | ((uint32_t)mt6835_rx_buf[3] << 5) | (mt6835_rx_buf[4] >> 3);

    // STATUS 检查
    uint8_t status = mt6835_rx_buf[4] & 0x07;
    if (status & 0x02)
    { // STATUS[1]: 磁场太弱
        g_device_status.encoder_state = OFFLINE;
        return false;
    }

    *angle = (uint16_t)(angle_21 >> 7); // 21-bit → 14-bit (16384)
    return true;
}

static void mt6835_start_read(void)
{
    if (!bsp_encoder_spi_is_ready())
    {
        encoder_recover_from_error();
        return;
    }
    bsp_encoder_cs(BSP_SPI_EXTERNAL, false);
    if (!bsp_encoder_spi_transmit_receive_dma((u8 *)mt6835_cmd_buf,
                                             (u8 *)mt6835_rx_buf, 5))
    {
        encoder_recover_from_error();
        return;
    }
    enc.state = ENCODER_STATE_WAIT_HIGH;
}

static void mt6835_main_loop(void *arg)
{
    tEncoderInstance *enc = (tEncoderInstance *)arg;

    switch (enc->state)
    {
    case ENCODER_STATE_START_READ:
        enc->no_resp_tic = 0;
        mt6835_start_read();
        break;
    case ENCODER_STATE_WAIT_HIGH:
        enc->no_resp_tic++;
        if (enc->no_resp_tic > NO_RESPONSE_MAX_TIC)
            enc->state = ENCODER_STATE_START_READ;
        break;
    case ENCODER_STATE_PROCESS_DATA:
    {
        uint16_t angle_raw;
        if (!enc->chip_desc->parse_and_check(0, 0, &angle_raw))
        {
            enc->valid = CLAMP(enc->valid + 10, 0, 110);
            enc->spi_error_rate += (1.0f - enc->spi_error_rate) * 0.01f; // EMA
            if (enc->valid > 100)
            {
                g_device_status.encoder_state = RUN_ERROR;
                enc->valid = 0;
            }
            enc->state = ENCODER_STATE_START_READ;
            return;
        }
        enc->valid = CLAMP(enc->valid - 1, 0, 110);
        enc->spi_error_rate *= 0.999f;
        enc->angle_raw = angle_raw;
        common_angle_velocity_update();
        enc->state = ENCODER_STATE_START_READ;
        break;
    }
    default:
        encoder_recover_from_error();
        break;
    }
}

// ========== 主循环任务 ==========
void encoder_main_loop_task(void)
{
    if (enc.chip_desc && enc.chip_desc->dma_state_entry)
    {
        enc.chip_desc->dma_state_entry(&enc);
    }
}

// ========== 对外数据接口 ==========
float encoder_get_angle_abs(void) { return enc.angle_abs; }
float encoder_get_angle_inc(void) { return enc.pos; }
float encoder_get_rpm(void) { return enc.pll_omega_rpm; }
int encoder_get_num_turns(void) { return enc.num_turns; }

void encoder_set_angle_zero(void)
{
    enc.pos = 0;
    enc.pos_offset = enc.angle_raw;
    enc.num_turns = 0;
    enc.pos_last = 0;
}
