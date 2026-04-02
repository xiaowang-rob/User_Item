#include "encoder.h"
#include "string.h"
#include "stdbool.h"
#include "spi.h"
#include "math_fast.h"

#ifdef __DEBUG__
volatile u32 encoder_ts_us = 0;
u32 time_zero;
#endif

/* ========== 全局变量 ========== */
static u16 reg03_cmd = 0x83ff;
volatile static u16 reg03_data = 0;
static u16 reg04_cmd = 0x84ff;
volatile static u16 reg04_data = 0;

// 1. 添加影子变量（用于主循环安全读取）
static volatile u16 reg03_data_shadow = 0;
static volatile u16 reg04_data_shadow = 0;

static bool first_run = true;
static u8 valid = 0;
static u8 rubbish_data_tic = 0;

volatile static tEncoder encoder = {0};

/* ========== CS 控制 ========== */
void ENCODER_SPI_CS_H(void)
{
    HAL_GPIO_WritePin(ENcoderCS_CPIOx, ENcoderCS_CPIOx_PIN, GPIO_PIN_SET);
}

void ENCODER_SPI_CS_L(void)
{
    HAL_GPIO_WritePin(ENcoderCS_CPIOx, ENcoderCS_CPIOx_PIN, GPIO_PIN_RESET);
}

/* ========== 内部函数声明 ========== */
static void ENCODER_StartReg3Read(void);
static void ENCODER_StartReg4Read(void);
static void ENCODER_ProcessData(void);
static void ENCODER_RecoverFromError(void);

/* ========== 启动读取高位寄存器 ========== */
static void ENCODER_StartReg3Read(void)
{
    // 检查 SPI 是否空闲
    if (HAL_SPI_GetState(&ENcoder_SPI_Get_HSPI) != HAL_SPI_STATE_READY)
    {
        ENCODER_RecoverFromError();
        return;
    }

    ENCODER_SPI_CS_L();

    // 启动 DMA 传输
    if (HAL_SPI_TransmitReceive_DMA(&ENcoder_SPI_Get_HSPI,
                                    (uint8_t *)&reg03_cmd,
                                    (uint8_t *)&reg03_data_shadow,
                                    2) != HAL_OK)
    { // 2 bytes = 16 bits
        ENCODER_RecoverFromError();
        return;
    }

    encoder.state = ENCODER_STATE_WAIT_HIGH;
}

/* ========== 启动读取低位寄存器 ========== */
static void ENCODER_StartReg4Read(void)
{
    if (HAL_SPI_GetState(&ENcoder_SPI_Get_HSPI) != HAL_SPI_STATE_READY)
    {
        ENCODER_RecoverFromError();
        return;
    }

    ENCODER_SPI_CS_L();

    if (HAL_SPI_TransmitReceive_DMA(&ENcoder_SPI_Get_HSPI,
                                    (uint8_t *)&reg04_cmd,
                                    (uint8_t *)&reg04_data_shadow,
                                    2) != HAL_OK)
    {
        ENCODER_RecoverFromError();
        return;
    }

    encoder.state = ENCODER_STATE_WAIT_LOW;
}

/* ========== 数据处理 ========== */
static void ENCODER_ProcessData(void)
{
    g_device_status.encoder_state = RUNNING;

    u32 current_time = HAL_GetTick();
    u32 time_diff = current_time - encoder.last_time;

    // 状态检查：弱磁报警
    if (reg04_data & MT6816_NO_MAG_WARNING)
    {
        g_device_status.encoder_state = OFFLINE;
        encoder.state = ENCODER_STATE_START_READ;
        return;
    }
    // 奇偶校验
    bool parity_check = (reg04_data & MT6816_PARITY_CHECK) ? true : false;

    u16 data_bits = (reg03_data & 0x00FF) << 7 | ((reg04_data & 0x00FE) >> 1);
    u8 bit_count = __builtin_popcount(data_bits);

    bool expected_parity = (bit_count % 2 == 1);

    if (expected_parity != parity_check)
    {
        valid = CLAMP(valid + 10, 0, 110);
        if (valid > 100)
        {
            g_device_status.encoder_state = RUN_ERROR;
            valid = 0;
        }
        encoder.state = ENCODER_STATE_START_READ;
        return;
    }
    else
    {
        valid = CLAMP(valid - 1, 0, 110);
    }

    // 解析 14 位角度值
    encoder.angle_raw = data_bits >> 1;
    // 角度计算
    encoder.angle_abs = (float)encoder.angle_raw * 0.02197265625f; // 2^14 = 16384

#ifdef __DEBUG__
    u32 cur_time = HAL_GetTick_us();
    encoder_ts_us = cur_time - time_zero;
    time_zero = cur_time;
#endif

    if (first_run)
    {
        encoder.angle_raw_last = encoder.angle_raw;
        encoder.num_turns = 0;
        encoder.num_turns_last = 0;
        encoder.pos_offset = encoder.angle_raw;
        encoder.omega_rpm = 0;
        encoder.pos = 0;
        encoder.last_time = current_time;
        if (rubbish_data_tic++ > 3)
        {
            rubbish_data_tic = 0;
            first_run = false;
        }
    }
    else
    { // 增量角度计算

        int angle_raw_delta = encoder.angle_raw - encoder.angle_raw_last;

        if (FABSF(encoder.omega_rpm) > 2900) // 大于2900rpm有超过180°的风险
        {
            if (angle_raw_delta < -4096) // 16384/4 90°
            {
                encoder.num_turns++;
            }
            else if (angle_raw_delta > 4096)
            {
                encoder.num_turns--;
            }
        }
        else
        {
            if (angle_raw_delta < -8192) // 16384/2 180°
            {
                encoder.num_turns++;
            }
            else if (angle_raw_delta > 8192)
            {
                encoder.num_turns--;
            }
        }
        // 过滤掉抖动
        if (angle_raw_delta > 1 || angle_raw_delta < -1)
        {
            // 角速度计算
            if (time_diff > 0)
            {
                encoder.omega_rpm = 0.16666666667f * ((encoder.num_turns - encoder.num_turns_last) * 360.0f + angle_raw_delta * 0.02197265625f) / (time_diff * 0.001f);
            }
            // 位置计算
            encoder.pos = 0.02197265625f * (int)(encoder.angle_raw - encoder.pos_offset) + encoder.num_turns * 360.0f;

            // 更新历史数据
            encoder.angle_raw_last = encoder.angle_raw;
            encoder.num_turns_last = encoder.num_turns;
            encoder.last_time = current_time;
        }
        else
            encoder.omega_rpm = 0;
    }

    // 启动下一次读取周期
    encoder.state = ENCODER_STATE_START_READ;
}

/* ========== 错误恢复 ========== */
static void ENCODER_RecoverFromError(void)
{
    // 1. 拉高 CS
    ENCODER_SPI_CS_H();

    // 2. 停止任何正在进行的 DMA
    if (HAL_SPI_GetState(&ENcoder_SPI_Get_HSPI) != HAL_SPI_STATE_READY)
    {
        HAL_SPI_Abort(&ENcoder_SPI_Get_HSPI); // 阻塞式中止
        // 或者用非阻塞: HAL_SPI_Abort_IT()
    }

    // 3. 清除 SPI/DMA 错误标志
    __HAL_SPI_CLEAR_OVRFLAG(&ENcoder_SPI_Get_HSPI);
    __HAL_SPI_CLEAR_FREFLAG(&ENcoder_SPI_Get_HSPI);

    // 4. 重置状态机
    encoder.state = ENCODER_STATE_START_READ;

    // 5. 重置数据
    encoder.omega_rpm = 0;
}

/* ========== DMA 完成回调 (中断上下文) ========== */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi != &ENcoder_SPI_Get_HSPI)
    {
        return;
    }

    // 立即拉高 CS
    ENCODER_SPI_CS_H();

    // 保存接收到的数据
    if (encoder.state == ENCODER_STATE_WAIT_HIGH)
    {
        encoder.state = ENCODER_STATE_WAIT_LOW;
        // 立即启动低位读取
        ENCODER_StartReg4Read();
    }
    else if (encoder.state == ENCODER_STATE_WAIT_LOW)
    {
        __disable_irq();
        reg03_data = reg03_data_shadow;
        reg04_data = reg04_data_shadow;
        __enable_irq();
        encoder.state = ENCODER_STATE_PROCESS_DATA;
        // 数据处理交给主循环，避免中断耗时
    }
}

/* ========== DMA 错误回调 ========== */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &ENcoder_SPI_Get_HSPI)
    {
        ENCODER_RecoverFromError();
    }
}

static u16 No_response_tic;
#define NO_RESPONSE_MAX_TIC 1000 // 1000*10us = 10ms
/* ========== 主循环任务 ========== */
void fEncoderMainLoopTask(void)
{
    switch (encoder.state)
    {
    case ENCODER_STATE_START_READ:
        No_response_tic = 0;
        ENCODER_StartReg3Read();
        break;

    case ENCODER_STATE_PROCESS_DATA:
        ENCODER_ProcessData();
        break;

    case ENCODER_STATE_WAIT_HIGH:
    case ENCODER_STATE_WAIT_LOW:
        No_response_tic++;
        if (No_response_tic > NO_RESPONSE_MAX_TIC)
            encoder.state = ENCODER_STATE_START_READ;
        break;

    default:
        ENCODER_RecoverFromError();
        break;
    }
}

/* ========== 数据获取接口 ========== */
float fGetEncoderAngle_ABS(void) { return encoder.angle_abs; }
float fGetEncoderAngle_INC(void) { return encoder.pos; }
float fGetEncoderRPM(void) { return encoder.omega_rpm; }

void fSetEncoderAngleZero(void)
{
    encoder.pos = 0;
    encoder.pos_offset = encoder.angle_raw;
    encoder.num_turns = 0;
    encoder.num_turns_last = 0;
}