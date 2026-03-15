#include "encoder.h"
#include "string.h"
#include "stdbool.h"
#include "spi.h"
#include "math_fast.h"

/* ========== 全局变量 ========== */
static u16 reg03_cmd = 0x83ff;
static u16 reg03_data = 0;
static u16 reg04_cmd = 0x84ff;
static u16 reg04_data = 0;

static tEncoder encoder = {0};

/* ========== DMA 缓冲区 (32位对齐) ========== */
// 使用 __attribute__((aligned(4))) 确保 32 位对齐
static __attribute__((aligned(4))) u32 dma_tx_buf;
static __attribute__((aligned(4))) u32 dma_rx_buf;

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

    // 准备 DMA 缓冲区 (MT6816: 16-bit 命令 +16-bit 数据)
    dma_tx_buf = reg03_cmd; // 0x83ff
    dma_rx_buf = 0;

    ENCODER_SPI_CS_L();

    // 启动 DMA 传输
    if (HAL_SPI_TransmitReceive_DMA(&ENcoder_SPI_Get_HSPI,
                                    (uint8_t *)&dma_tx_buf,
                                    (uint8_t *)&dma_rx_buf,
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

    dma_tx_buf = reg04_cmd; // 0x84ff
    dma_rx_buf = 0;

    ENCODER_SPI_CS_L();

    if (HAL_SPI_TransmitReceive_DMA(&ENcoder_SPI_Get_HSPI,
                                    (uint8_t *)&dma_tx_buf,
                                    (uint8_t *)&dma_rx_buf,
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
    u32 current_time = HAL_GetTick();
    u32 time_diff = current_time - encoder.last_time;

    // 解析 14 位角度值
    u16 angle_raw = ((reg03_data & 0x00FF) << 6) | ((reg04_data & 0x00FC) >> 2);
    encoder.angle_abs = angle_raw * 0.02197265625f; // 2^14 = 16384

    // 增量角度计算
    float angle_delta = encoder.angle_abs - encoder.angle_last;
    if (angle_delta < -180)
    {
        encoder.num_turns++;
    }
    else if (angle_delta > 180)
    {
        encoder.num_turns--;
    }
    encoder.angle_inc = encoder.num_turns * 360 + encoder.angle_abs;

    // 角速度计算
    if (time_diff > 0)
    {
        encoder.omega = (encoder.angle_inc - encoder.angle_inc_last) / (time_diff * 0.001f);
    }

    // 更新历史数据
    encoder.angle_last = encoder.angle_abs;
    encoder.angle_inc_last = encoder.angle_inc;
    encoder.last_time = current_time;

    // 状态检查：弱磁报警
    if (reg04_data & MT6816_NO_MAG_WARNING)
    {
        g_device_status.encoder_state = OFFLINE;
        ENCODER_RecoverFromError();
        return;
    }
    else if (g_device_status.encoder_state == OFFLINE)
    {
        g_device_status.encoder_state = RUNNING;
    }

    // 奇偶校验
    bool parity_check = (reg04_data & MT6816_PARITY_CHECK) ? true : false;
    u8 bit_count = __builtin_popcount(angle_raw & 0x3FFF); // 14 位角度
    bool expected_parity = (bit_count % 2 == 1);

    static u8 valid = 0;
    if (expected_parity != parity_check)
    {
        valid += 10;
    }
    else if (valid > 0)
    {
        valid--;
    }

    if (valid > 100)
    {
        g_device_status.encoder_state = RUN_ERROR;
    }
    else if (g_device_status.encoder_state == RUN_ERROR)
    {
        g_device_status.encoder_state = RUNNING;
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
    encoder.angle_abs = 0;
    encoder.angle_last = 0;
    encoder.angle_inc = 0;
    encoder.angle_inc_last = 0;
    encoder.omega = 0;
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
        reg03_data = (u16)dma_rx_buf;
        encoder.state = ENCODER_STATE_WAIT_LOW;
        // 立即启动低位读取
        ENCODER_StartReg4Read();
    }
    else if (encoder.state == ENCODER_STATE_WAIT_LOW)
    {
        reg04_data = (u16)dma_rx_buf;
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

/* ========== 主循环任务 ========== */
void fEncoderMainLoopTask(void)
{
    switch (encoder.state)
    {
    case ENCODER_STATE_START_READ:
        ENCODER_StartReg3Read();
        break;

    case ENCODER_STATE_PROCESS_DATA:
        ENCODER_ProcessData();
        break;

    case ENCODER_STATE_WAIT_HIGH:
    case ENCODER_STATE_WAIT_LOW:
        // 等待中断回调处理，主循环不干预
        break;

    default:
        ENCODER_RecoverFromError();
        break;
    }
}

/* ========== 数据获取接口 ========== */
float fGetEncoderAngle_ABS(void) { return encoder.angle_abs; }
float fGetEncoderAngle_INC(void) { return encoder.angle_inc; }
float fGetEncoderRPM(void) { return encoder.omega * 0.16666666666667f; }