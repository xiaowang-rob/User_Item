/* ============================================================
 * 编码器抽象驱动 (支持 MT6816 / AS5047 / MT6835)
 * 使用全局实例 g_encoder，初始化时选择芯片型号
 * ============================================================ */

#include "device.h"
#include "string.h"
#include "math_fast.h"

#ifdef __DEBUG__
volatile u32 encoder_ts_us = 0;
u32 time_zero;
#endif

/* ----------------------- 类型定义 ------------------------- */
typedef enum
{
    ENCODER_STATE_START_READ,
    ENCODER_STATE_WAIT_HIGH,
    ENCODER_STATE_WAIT_LOW,
    ENCODER_STATE_PROCESS_DATA,
    ENCODER_STATE_START_LOW, /* MT6816 专用：准备读低字节 */
    ENCODER_STATE_START_NOP  /* AS5047 专用：准备发 NOP 命令 */
} eEncoderState_DMA;

/* ----------------------- 全局实例 ------------------------- */
tEncoderInstance g_encoder = {.chip_type = 0xff};

/* ----------------------- 常量定义 ------------------------- */
#define NO_RESPONSE_MAX_TIC 1000 /* 1000 * 10us = 10ms 超时 */
#define MT6816_NO_MAG_WARNING (1 << 1)
#define MT6816_PARITY_CHECK (1 << 0)

/* ----------------------- 芯片描述表 ----------------------- */
static bool MT6816_ParseAndCheck(uint16_t high, uint16_t low, uint16_t *angle);
static bool AS5047_ParseAndCheck(uint16_t high, uint16_t low, uint16_t *angle);
static bool MT6835_ParseAndCheck(uint16_t high, uint16_t low, uint16_t *angle);
static void MT6816_MainLoop(void *arg);
static void AS5047_MainLoop(void *arg);
static void MT6835_MainLoop(void *arg);

/* MT6835: 连续读角度 5-byte 命令 + 接收缓冲 */
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
        .parse_and_check = MT6816_ParseAndCheck,
        .use_dma_state_machine = true,
        .dma_post_high_state = ENCODER_STATE_START_LOW,
        .dma_state_entry = MT6816_MainLoop,
    },
    [AS5047] = {
        .resolution = 16384,
        .deg_per_lsb = 360.0f / 16384.0f,
        .cmd_high = 0,
        .cmd_low = 0,
        .cmd_read_angle = 0x7FFF, /* 读角度寄存器命令 */
        .spi_CPOL = 0,
        .spi_CPHA = 1,
        .spi_data_size = 16,
        .parse_and_check = AS5047_ParseAndCheck,
        .use_dma_state_machine = true,
        .dma_post_high_state = ENCODER_STATE_START_NOP,
        .dma_state_entry = AS5047_MainLoop,
    },
    [MT6835] = {
        .resolution = 16384,       /* 14-bit 输出 (21-bit 原始 >> 7) */
        .deg_per_lsb = 360.0f / 2097152.0f,
        .cmd_high = 0,
        .cmd_low = 0,
        .cmd_read_angle = 0,
        .spi_CPOL = 1,               /* Mode 3 */
        .spi_CPHA = 1,
        .spi_data_size = 8,          /* 8-bit 模式 */
        .parse_and_check = MT6835_ParseAndCheck,
        .use_dma_state_machine = true,
        .dma_post_high_state = ENCODER_STATE_PROCESS_DATA,  /* 单次交易完成即处理 */
        .dma_state_entry = MT6835_MainLoop,
    },
};

/* ------------------ 内部函数前向声明 --------------------- */
static void Encoder_RecoverFromError(void);
static void Common_AngleVelocityUpdate(tEncoderInstance *enc);

static void MT6816_StartHighRead(void);
static void MT6816_StartLowRead(void);
static void MT6816_ProcessData(void);

static void AS5047_StartCommand(void);
static void AS5047_StartNOP(void);
static void AS5047_ProcessData(void);

/* ========== 初始化 ========== */
bool encoder_init(eEncoderChip type)
{
    if (type >= CHIP_COUNT)
        return false;

    if (type == g_encoder.chip_type)
    {
        g_device_status.encoder_state = ONLINE;
        return true;
    }

    /* 清空全局实例并赋初值 */
    memset((void *)&g_encoder, 0, sizeof(g_encoder));
    g_encoder.chip_desc = &chip_descs[type];
    g_encoder.chip_type = type;
    g_encoder.state = ENCODER_STATE_START_READ;
    g_encoder.first_run = true;
    g_encoder.valid = 0;
    g_encoder.rubbish_data_tic = 0;

    /* 根据型号动态配置 SPI3 模式 (CPOL/CPHA) */
    if (false == BSP_SetEncoder_SPI_Config(g_encoder.chip_desc->spi_CPOL, g_encoder.chip_desc->spi_CPHA, g_encoder.chip_desc->spi_data_size))
    {
        g_device_status.encoder_state = OFFLINE;
        return false;
    }
    g_device_status.encoder_state = ONLINE;

    return true;
}

/* ========== 错误恢复 ========== */
static void Encoder_RecoverFromError(void)
{
    BSP_Encoder_CS(EXTERNAL, true);
    if (!BSP_Encoder_SPI_IS_READY())
    {
        BSP_Encoder_SPI_Abort();
    }
    BSP_Encoder_SPI_CLEAR_DMA_error_flags();
    g_device_status.encoder_state = OFFLINE;
    g_encoder.state = ENCODER_STATE_START_READ;
}

/* ========== DMA 完成回调（不再启动任何 SPI 传输） ========== */
void bsp_encoder_spi_txrx_cplt_callback(void)
{
    BSP_Encoder_CS(EXTERNAL, true);
    tEncoderInstance *enc = &g_encoder;

    if (enc->state == ENCODER_STATE_WAIT_HIGH) {
        enc->state = enc->chip_desc->dma_post_high_state;
        enc->no_resp_tic = 0;
    } else if (enc->state == ENCODER_STATE_WAIT_LOW) {
        BSP_disable_irq();
        enc->data_raw[0] = enc->shadow_raw[0];
        enc->data_raw[1] = enc->shadow_raw[1];
        BSP_enable_irq();
        enc->state = ENCODER_STATE_PROCESS_DATA;
        enc->no_resp_tic = 0;
    }
}

/* ========== DMA 错误回调 ========== */
void bsp_encoder_spi_error_callback(void)
{
    Encoder_RecoverFromError();
}

/* PLL 更新：给定观测角度 (deg)，更新 PLL 估计角度和速度 */
static void Encoder_PllUpdate(tEncoderInstance *enc, float angle_obs_deg, float dt)
{
    float delta = angle_obs_deg - enc->pll_theta;
    /* 角度归一化到 [-180, 180) */
    if (delta > 180.0f)  delta -= 360.0f;
    if (delta < -180.0f) delta += 360.0f;

    enc->pll_theta += (enc->pll_omega_rpm * 6.0f * dt + enc->pll_kp * delta) * dt;
    enc->pll_integ += enc->pll_ki * delta * dt;
    enc->pll_omega_rpm += enc->pll_integ * dt;

    /* 限幅防止发散 */
    if (enc->pll_omega_rpm > 100000.0f) enc->pll_omega_rpm = 100000.0f;
    if (enc->pll_omega_rpm < -100000.0f) enc->pll_omega_rpm = -100000.0f;

    /* 角度归一化 */
    if (enc->pll_theta > 360.0f)  enc->pll_theta -= 360.0f;
    if (enc->pll_theta < 0.0f)    enc->pll_theta += 360.0f;
}

/* ========== 通用角度/速度更新 (所有芯片共用) ========== */
static void Common_AngleVelocityUpdate(tEncoderInstance *enc)
{
    g_device_status.encoder_state = RUNNING;

    enc->angle_abs = enc->angle_raw * enc->chip_desc->deg_per_lsb;

#ifdef __DEBUG__
    volatile u32 current_time_us = BSP_GetTick_us();
    encoder_ts_us = current_time_us - time_zero;
    time_zero = current_time_us;
#endif

    if (enc->first_run)
    {
        enc->angle_raw_last = enc->angle_raw;
        enc->num_turns = 0;
        enc->pos_offset = enc->angle_raw;
        enc->omega_rpm = 0;
        enc->pos = 0;
        enc->pos_last = 0;
        /* PLL 初始化 */
        enc->pll_theta = enc->angle_abs;
        enc->pll_omega_rpm = 0;
        enc->pll_kp = 160.0f;   /* PLL 比例增益 (对应带宽 ~80 rad/s) */
        enc->pll_ki = 6400.0f;  /* PLL 积分增益 (临界阻尼: ki = kp²/4) */
        enc->pll_integ = 0.0f;

        if (enc->rubbish_data_tic++ > 3)
        {
            enc->rubbish_data_tic = 0;
            enc->first_run = false;
        }
    }
    else
    {
        /* PLL 发散检测：速度饱和时重置积分器 */
        if (enc->pll_omega_rpm >= 99999.0f || enc->pll_omega_rpm <= -99999.0f ||
            enc->pll_integ > 10000.0f || enc->pll_integ < -10000.0f)
        {
            enc->pll_integ = 0.0f;
            enc->pll_omega_rpm = 0.0f;
            enc->pll_theta = enc->angle_abs;
        }

        int32_t angle_raw_delta = (int32_t)enc->angle_raw - enc->angle_raw_last;

        /* 根据当前转速选择过零判断阈值（保留累计圈数） */
        float speed_ref = FABSF(enc->pll_omega_rpm > 1.0f ? enc->pll_omega_rpm : enc->omega_rpm);
        if (speed_ref > 2900)
        {
            if (angle_raw_delta < -4096)
                enc->num_turns++;
            else if (angle_raw_delta > 4096)
                enc->num_turns--;
        }
        else
        {
            if (angle_raw_delta < -8192)
                enc->num_turns++;
            else if (angle_raw_delta > 8192)
                enc->num_turns--;
        }

        enc->pos = enc->chip_desc->deg_per_lsb * (int32_t)(enc->angle_raw - enc->pos_offset) + enc->num_turns * 360.0f;
        enc->angle_raw_last = enc->angle_raw;

        /* PLL 角度/速度估计 (dt ≈ 100us, 对应 10kHz 编码器更新率) */
        Encoder_PllUpdate(enc, enc->angle_abs, 0.0001f);
    }
}

/* ====== MT6816 驱动 ====== */
static bool MT6816_ParseAndCheck(uint16_t high, uint16_t low, uint16_t *angle)
{
    bool parity_check = (low & MT6816_PARITY_CHECK) ? true : false;
    uint16_t data_bits = (high & 0x00FF) << 7 | ((low & 0x00FE) >> 1);
    uint8_t bit_count = __builtin_popcount(data_bits);
    bool expected_parity = (bit_count % 2 == 1);

    if (expected_parity != parity_check)
        return false;
    *angle = data_bits >> 1; /* 14-bit 原始角度 */
    return true;
}

static void MT6816_StartHighRead(void)
{
    if (!BSP_Encoder_SPI_IS_READY())
    {
        Encoder_RecoverFromError();
        return;
    }
    BSP_Encoder_CS(EXTERNAL, false);

    tEncoderInstance *enc = &g_encoder;
    enc->cmd_reg = enc->chip_desc->cmd_high;
    if (!BSP_Encoder_SPI_TransmitReceive_DMA((u8 *)&enc->cmd_reg,
                                             (u8 *)&enc->shadow_raw[0], 2))
    {
        Encoder_RecoverFromError();
        return;
    }
    enc->state = ENCODER_STATE_WAIT_HIGH;
}

static void MT6816_StartLowRead(void)
{
    if (!BSP_Encoder_SPI_IS_READY())
    {
        Encoder_RecoverFromError();
        return;
    }
    BSP_Encoder_CS(EXTERNAL, false);

    tEncoderInstance *enc = &g_encoder;
    enc->cmd_reg = enc->chip_desc->cmd_low;
    if (!BSP_Encoder_SPI_TransmitReceive_DMA((u8 *)&enc->cmd_reg,
                                             (u8 *)&enc->shadow_raw[1], 2))
    {
        Encoder_RecoverFromError();
        return;
    }
    enc->state = ENCODER_STATE_WAIT_LOW;
}

static void MT6816_ProcessData(void)
{
    tEncoderInstance *enc = &g_encoder;
    uint16_t angle_raw;

    /* 弱磁检测 */
    if (enc->data_raw[1] & MT6816_NO_MAG_WARNING)
    {
        g_device_status.encoder_state = OFFLINE;
        enc->state = ENCODER_STATE_START_READ;
        return;
    }

    /* 奇偶校验及数据提取 */
    if (!enc->chip_desc->parse_and_check(enc->data_raw[0], enc->data_raw[1], &angle_raw))
    {
        enc->valid = CLAMP(enc->valid + 10, 0, 110);
        enc->spi_error_rate += (1.0f - enc->spi_error_rate) * 0.01f;  /* EMA */
        if (enc->valid > 100)
        {
            g_device_status.encoder_state = RUN_ERROR;
            enc->valid = 0;
        }
        enc->state = ENCODER_STATE_START_READ;
        return;
    }

    enc->valid = CLAMP(enc->valid - 1, 0, 110);
    enc->spi_error_rate *= 0.999f;  /* 成功时衰减 */
    enc->angle_raw = angle_raw;
    Common_AngleVelocityUpdate(enc);
    enc->state = ENCODER_STATE_START_READ;
}

static void MT6816_MainLoop(void *arg)
{
    tEncoderInstance *enc = (tEncoderInstance *)arg;
    switch (enc->state)
    {
    case ENCODER_STATE_START_READ:
        enc->no_resp_tic = 0;
        MT6816_StartHighRead();
        break;

    case ENCODER_STATE_START_LOW:
        /* 启动低字节读取 (高字节已就绪) */
        MT6816_StartLowRead();
        break;

    case ENCODER_STATE_WAIT_HIGH:
    case ENCODER_STATE_WAIT_LOW:
        enc->no_resp_tic++;
        if (enc->no_resp_tic > NO_RESPONSE_MAX_TIC)
            enc->state = ENCODER_STATE_START_READ;
        break;

    case ENCODER_STATE_PROCESS_DATA:
        MT6816_ProcessData();
        break;

    default:
        Encoder_RecoverFromError();
        break;
    }
}

/* ====== AS5047 驱动 ====== */
static bool AS5047_ParseAndCheck(uint16_t raw, uint16_t unused, uint16_t *angle)
{
    (void)unused;
    /* 检查错误标志 (EF, bit14) */
    if (raw & 0x4000)
        return false;

    /* 提取14位角度数据 */
    *angle = raw & 0x3FFF;
    return true;
}

static void AS5047_StartCommand(void)
{
    if (!BSP_Encoder_SPI_IS_READY())
    {
        Encoder_RecoverFromError();
        return;
    }
    BSP_Encoder_CS(EXTERNAL, false);

    tEncoderInstance *enc = &g_encoder;
    enc->cmd_reg = enc->chip_desc->cmd_read_angle;
    if (!BSP_Encoder_SPI_TransmitReceive_DMA((u8 *)&enc->cmd_reg,
                                             (u8 *)&enc->shadow_raw[0], 2))
    {
        Encoder_RecoverFromError();
        return;
    }
    enc->state = ENCODER_STATE_WAIT_HIGH;
}

static void AS5047_StartNOP(void)
{
    if (!BSP_Encoder_SPI_IS_READY())
    {
        Encoder_RecoverFromError();
        return;
    }
    BSP_Encoder_CS(EXTERNAL, false);

    tEncoderInstance *enc = &g_encoder;
    enc->cmd_reg = 0x0000; /* NOP 命令 */
    if (!BSP_Encoder_SPI_TransmitReceive_DMA((u8 *)&enc->cmd_reg,
                                             (u8 *)&enc->shadow_raw[0], 2))
    {
        Encoder_RecoverFromError();
        return;
    }
    enc->state = ENCODER_STATE_WAIT_LOW;
}

static void AS5047_ProcessData(void)
{
    tEncoderInstance *enc = &g_encoder;
    uint16_t angle_raw;

    if (!enc->chip_desc->parse_and_check(enc->data_raw[0], 0, &angle_raw))
    {
        enc->valid = CLAMP(enc->valid + 10, 0, 110);
        enc->spi_error_rate += (1.0f - enc->spi_error_rate) * 0.01f;  /* EMA */
        if (enc->valid > 100)
        {
            g_device_status.encoder_state = RUN_ERROR;
            enc->valid = 0;
        }
        enc->state = ENCODER_STATE_START_READ;
        return;
    }

    enc->valid = CLAMP(enc->valid - 1, 0, 110);
    enc->spi_error_rate *= 0.999f;  /* 成功时衰减 */

    enc->angle_raw = angle_raw;
    Common_AngleVelocityUpdate(enc);
    enc->state = ENCODER_STATE_START_READ;
}

static void AS5047_MainLoop(void *arg)
{
    tEncoderInstance *enc = (tEncoderInstance *)arg;
    switch (enc->state)
    {
    case ENCODER_STATE_START_READ:
        enc->no_resp_tic = 0;
        AS5047_StartCommand();
        break;

    case ENCODER_STATE_START_NOP:
        /* 命令帧已发送，现在发送 NOP 接收数据 */
        AS5047_StartNOP();
        break;

    case ENCODER_STATE_WAIT_HIGH:
    case ENCODER_STATE_WAIT_LOW:
        enc->no_resp_tic++;
        if (enc->no_resp_tic > NO_RESPONSE_MAX_TIC)
            enc->state = ENCODER_STATE_START_READ;
        break;

    case ENCODER_STATE_PROCESS_DATA:
        AS5047_ProcessData();
        break;

    default:
        Encoder_RecoverFromError();
        break;
    }
}

/* ====== MT6835 驱动 (SPI Mode 3, 8-bit, 连续读角度) ====== */
static bool MT6835_ParseAndCheck(uint16_t raw, uint16_t unused, uint16_t *angle)
{
    (void)unused;
    /* mt6835_rx_buf[2] = ANGLE[20:13], [3] = ANGLE[12:5], [4] = ANGLE[4:0] + STATUS[2:0] */
    uint32_t angle_21 = ((uint32_t)mt6835_rx_buf[2] << 13)
                      | ((uint32_t)mt6835_rx_buf[3] << 5)
                      | (mt6835_rx_buf[4] >> 3);

    /* STATUS 检查 */
    uint8_t status = mt6835_rx_buf[4] & 0x07;
    if (status & 0x02) {  /* STATUS[1]: 磁场太弱 */
        g_device_status.encoder_state = OFFLINE;
        return false;
    }

    *angle = (uint16_t)(angle_21 >> 7);  /* 21-bit → 14-bit (16384) */
    return true;
}

static void MT6835_StartRead(void)
{
    if (!BSP_Encoder_SPI_IS_READY()) {
        Encoder_RecoverFromError();
        return;
    }
    BSP_Encoder_CS(EXTERNAL, false);
    if (!BSP_Encoder_SPI_TransmitReceive_DMA((u8 *)mt6835_cmd_buf,
                                             (u8 *)mt6835_rx_buf, 5)) {
        Encoder_RecoverFromError();
        return;
    }
    g_encoder.state = ENCODER_STATE_WAIT_HIGH;
}

static void MT6835_MainLoop(void *arg)
{
    (void)arg;
    tEncoderInstance *enc = &g_encoder;
    switch (enc->state) {
    case ENCODER_STATE_START_READ:
        enc->no_resp_tic = 0;
        MT6835_StartRead();
        break;
    case ENCODER_STATE_WAIT_HIGH:
        enc->no_resp_tic++;
        if (enc->no_resp_tic > NO_RESPONSE_MAX_TIC)
            enc->state = ENCODER_STATE_START_READ;
        break;
    case ENCODER_STATE_PROCESS_DATA: {
        uint16_t angle_raw;
        if (!enc->chip_desc->parse_and_check(0, 0, &angle_raw)) {
            enc->valid = CLAMP(enc->valid + 10, 0, 110);
            enc->spi_error_rate += (1.0f - enc->spi_error_rate) * 0.01f;  /* EMA */
            if (enc->valid > 100) {
                g_device_status.encoder_state = RUN_ERROR;
                enc->valid = 0;
            }
            enc->state = ENCODER_STATE_START_READ;
            return;
        }
        enc->valid = CLAMP(enc->valid - 1, 0, 110);
        enc->spi_error_rate *= 0.999f;
        enc->angle_raw = angle_raw;
        Common_AngleVelocityUpdate(enc);
        enc->state = ENCODER_STATE_START_READ;
        break;
    }
    default:
        Encoder_RecoverFromError();
        break;
    }
}

/* ========== 主循环任务 ========== */
void encoder_main_loop_task(void)
{
    if (g_encoder.chip_desc && g_encoder.chip_desc->dma_state_entry) {
        g_encoder.chip_desc->dma_state_entry(&g_encoder);
    }
}

/* ========== 对外数据接口 ========== */
float encoder_get_angle_abs(void) { return g_encoder.angle_abs; }
float encoder_get_angle_inc(void) { return g_encoder.pos; }
float encoder_get_rpm(float f_speed)
{
    /* PLL 已收敛时优先使用 PLL 输出 */
    if (g_encoder.pll_omega_rpm > 1.0f || g_encoder.pll_omega_rpm < -1.0f) {
        g_encoder.omega_rpm = g_encoder.pll_omega_rpm;
    } else {
        float pos_delta = g_encoder.pos - g_encoder.pos_last;
        if (FABSF(pos_delta) <= g_encoder.chip_desc->deg_per_lsb)
        {
            g_encoder.omega_rpm = 0;
        }
        else
        {
            g_encoder.omega_rpm = pos_delta * f_speed * 0.16666666667f;
        }
        g_encoder.pos_last = g_encoder.pos;
    }
    return g_encoder.omega_rpm;
}
int encoder_get_num_turns(void) { return g_encoder.num_turns; }

void encoder_set_angle_zero(void)
{
    g_encoder.pos = 0;
    g_encoder.pos_offset = g_encoder.angle_raw;
    g_encoder.num_turns = 0;
    g_encoder.pos_last = 0;
}
