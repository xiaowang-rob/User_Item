#include "encoder.h"
#include "string.h"
#include "stdbool.h"
#include "spi.h"
#include "math_fast.h"
#include "drive_state.h"

ENCODER_t encoder = {0};

void ENCODER_SPI_CS_H()
{
    HAL_GPIO_WritePin(ENcoderCS_CPIOx, ENcoderCS_CPIOx_PIN, GPIO_PIN_SET);
}
void ENCODER_SPI_CS_L()
{
    HAL_GPIO_WritePin(ENcoderCS_CPIOx, ENcoderCS_CPIOx_PIN, GPIO_PIN_RESET);
}
/**
 * @brief 启动单个寄存器读取
 * @param reg_addr: 寄存器地址 (0x03或0x04)
 * @return true: 启动成功，false: 失败
 */
static bool ENCODER_StartRegisterRead(uint8_t reg_addr)
{
    tx_buffer[0] = reg_addr;
    ENCODER_SPI_CS_L();

    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_DMA(
        &ENcoder_SPI_Get_HSPI,
        tx_buffer,
        rx_buffer,
        1);

    if (status != HAL_OK)
    {
        ENCODER_SPI_CS_H();
        return false;
    }

    transfer_start_time = HAL_GetTick();
    return true;
}

static u8 valid = 0;

/**
 * @brief 处理读取到的数据并启动下一次读取
 */
static void ENCODER_ProcessAndNextRead(void)
{
    u32 current_time = HAL_GetTick();
    u32 time_diff = current_time - encoder.last_time;

    // 1. 解析14位角度值
    u16 angle_raw = ((reg03_data & 0xFF) << 6) | ((reg04_data & 0xFC) >> 2);
    float angle_deg = angle_raw * (360.0f / 16384.0f);                 // 16384 = 2^14
    float angle_abs = angle_deg * 0.017453292f + encoder.angle_offset; // deg to rad

    // 2. 计算角速度 (rad/s)
    if (time_diff > 0)
    {
        encoder.omega = (angle_abs - encoder.angle_last) / (time_diff * 0.001f);
    }

    // 3. 累积角度
    encoder.angle_inc += angle_abs - encoder.angle_last;

    // 4. 更新数据
    encoder.angle_abs = angle_abs;
    encoder.angle_last = angle_abs;
    encoder.last_time = current_time;

    // 5. 状态检查
    if (reg04_data & MT6816_NO_MAG_WARNING)
    {
        ENCODER_state_set(SINGNAL_ERROR);
    }
    else if (ENCODER_state_get() == SINGNAL_ERROR)
    {
        ENCODER_state_set(ONLINE);
    }
    // 奇偶校验验证
    bool parity_check = (reg04_data & MT6816_PARITY_CHECK) ? true : false;
    // 计算14位角度值中1的个数
    u8 bit_count = 0;
    u16 temp_angle = angle_raw;
    for (int i = 0; i < 14; i++)
    {
        if (temp_angle & (1 << i))
            bit_count++;
    }

    // 奇数个1 → PC应为1，偶数个1 → PC应为0
    bool expected_parity = (bit_count % 2 == 1);
    // 通讯异常检测
    if (expected_parity != parity_check)
        valid += 10;
    else
        valid = valid > 0 ? valid - 1 : 0;
    if (valid > 100)
        ENCODER_state_set(RUN_ERROR);
    else if (ENCODER_state_get() == RUN_ERROR)
        ENCODER_state_set(ONLINE);
    // 6. 立即启动下一次读取
    if (ENCODER_StartRegisterRead(0x03))
    {
        encoder.state = ENCODER_STATE_WAIT_HIGH;
    }
    else
    {
        encoder.state = ENCODER_STATE_START_READ;
    }
    encoder.data_ready = true;
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi != &ENcoder_SPI_Get_HSPI)
    {
        return;
    }

    ENCODER_SPI_CS_H(); // 拉高CS

    u32 current_time = HAL_GetTick();

    // 检查超时
    if (current_time - transfer_start_time > TRANSFER_TIMEOUT_MS)
    {
        // 超时处理：重置状态机
        encoder.state = ENCODER_STATE_START_READ;
        return;
    }

    switch (encoder.state)
    {
    case ENCODER_STATE_WAIT_HIGH:
        // 保存高位数据
        reg03_data = rx_buffer[0];

        if (reg03_data == 0xFF)
        {
            ENCODER_state_set(OFFLINE);
            encoder.state = ENCODER_STATE_START_READ;
            return;
        }
        else if (ENCODER_state_get() == OFFLINE)
            ENCODER_state_set(ONLINE);

        // 启动低位寄存器读取
        if (ENCODER_StartRegisterRead(0x04))
        {
            encoder.state = ENCODER_STATE_WAIT_LOW;
        }
        else
        {
            encoder.state = ENCODER_STATE_START_READ;
        }
        break;

    case ENCODER_STATE_WAIT_LOW:
        // 保存低位数据
        reg04_data = rx_buffer[0];

        // 进入数据处理状态
        encoder.state = ENCODER_STATE_PROCESS_DATA;
        break;

    default:
        encoder.state = ENCODER_STATE_START_READ;
        break;
    }
}

void ENCODER_Init()
{
    memset(&encoder, 0, sizeof(ENCODER_t));
    ENCODER_MainLoopTask();
}
void ENCODER_MainLoopTask()
{
    // 检查是否需要处理数据
    if (encoder.state == ENCODER_STATE_PROCESS_DATA)
    {
        ENCODER_ProcessAndNextRead();
    }
}
float GET_ENCODER_ANGLE_ABS()
{
    return encoder.angle_abs;
}
float GET_ENCODER_ANGLE_INC()
{
    return encoder.angle_inc;
}
float GET_ENCODER_OMEGA()
{
    return encoder.omega;
}
void SET_ENCODER_ANGLE_OFFSET(float offset)
{
    encoder.angle_offset = offset;
}
float GET_ENCODER_ANGLE_OFFSET()
{
    return encoder.angle_offset;
}
