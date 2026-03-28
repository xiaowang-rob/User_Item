#include "encoder.h"
#include "string.h"
#include "stdbool.h"
#include "spi.h"
#include "math_fast.h"

// 全局变量
static u16 reg03_cmd = 0x83ff;
static u16 reg03_data = 0; // 高位寄存器数据
static u16 reg04_cmd = 0x84ff;
static u16 reg04_data = 0; // 低位寄存器数据
static u32 transfer_start_time = 0;

static tEncoder encoder = {0};

void ENCODER_SPI_CS_H()
{
    HAL_GPIO_WritePin(ENcoderCS_CPIOx, ENcoderCS_CPIOx_PIN, GPIO_PIN_SET);
}
void ENCODER_SPI_CS_L()
{
    HAL_GPIO_WritePin(ENcoderCS_CPIOx, ENcoderCS_CPIOx_PIN, GPIO_PIN_RESET);
}
static void ENCODER_Reg3_Read()
{
    ENCODER_SPI_CS_L();

    if (HAL_OK != HAL_SPI_TransmitReceive_DMA(&ENcoder_SPI_Get_HSPI, (u8 *)&reg03_cmd, (u8 *)&reg03_data, 1))
    {
        ENCODER_SPI_CS_H();
        encoder.state = ENCODER_STATE_START_READ;
    }
}
static void ENCODER_Reg4_Read()
{
    ENCODER_SPI_CS_L();

    if (HAL_OK != HAL_SPI_TransmitReceive_DMA(&ENcoder_SPI_Get_HSPI, (u8 *)&reg04_cmd, (u8 *)&reg04_data, 1))
    {
        ENCODER_SPI_CS_H();
        encoder.state = ENCODER_STATE_START_READ;
    }
}

static u8 valid = 0;

/**
 * @brief 处理读取到的数据并启动下一次读取
 */
static void ENCODER_ProcessAndNextRead(void)
{

    u32 current_time = HAL_GetTick();
    u32 time_diff = current_time - encoder.last_time;

    //  解析14位角度值
    u16 angle_raw = ((reg03_data & 0x00FF) << 6) | ((reg04_data & 0x00FC) >> 2);
    encoder.angle_abs = angle_raw * 0.000383495197f; // 16384 = 2^14

    // 增量角度
    float angle_delta = encoder.angle_abs - encoder.angle_last;
    if (angle_delta < -MATH_PI || angle_delta > MATH_PI)
    { // 圈数改变
        encoder.num_turns += (angle_delta < 0) ? 1 : -1;
    }
    encoder.angle_inc = encoder.num_turns * MATH_2PI + encoder.angle_abs; // 弧度值
    //  计算角速度 (rad/s)
    if (time_diff > 0)
    {
        encoder.omega = (encoder.angle_inc - encoder.angle_inc_last) / (time_diff * 0.001f);
    }

    // 更新数据
    encoder.angle_last = encoder.angle_abs;
    encoder.angle_inc_last = encoder.angle_inc;
    encoder.last_time = current_time;

    // 工作状态检查
    if (reg04_data & MT6816_NO_MAG_WARNING)
    {
        g_device_status.encoder_state = OFFLINE;
        encoder.state = ENCODER_STATE_START_READ;
        encoder.angle_abs = 0;
        encoder.angle_last = 0;
        encoder.angle_inc = 0;
        encoder.angle_inc_last = 0;
        encoder.omega = 0;
        encoder.num_turns = 0;
        return;
    }
    else if (g_device_status.encoder_state == OFFLINE)
    {
        g_device_status.encoder_state = RUNNING;
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
        g_device_status.encoder_state = RUN_ERROR;
    else if (g_device_status.encoder_state == RUN_ERROR)
        g_device_status.encoder_state = RUNNING;
    // 立即启动下一次读取
    encoder.state = ENCODER_STATE_WAIT_HIGH;
    ENCODER_Reg3_Read();
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi != &ENcoder_SPI_Get_HSPI)
    {
        return;
    }
    ENCODER_SPI_CS_H(); // 拉高CS
    if (encoder.state == ENCODER_STATE_WAIT_HIGH)
    {
        encoder.state = ENCODER_STATE_WAIT_LOW;
        ENCODER_Reg4_Read();
    }
    else if (encoder.state == ENCODER_STATE_WAIT_LOW)
    {
        encoder.state = ENCODER_STATE_PROCESS_DATA;
    }
}

void fEncoderMainLoopTask()
{
    // 检查是否需要处理数据
    if (encoder.state == ENCODER_STATE_PROCESS_DATA)
    {
        ENCODER_ProcessAndNextRead();
    }
    else if (encoder.state == ENCODER_STATE_START_READ)
    {
        encoder.state = ENCODER_STATE_WAIT_HIGH;
        ENCODER_Reg3_Read();
    }
}
float fGetEncoderAngle_ABS()
{
    return encoder.angle_abs;
}
float fGetEncoderAngle_INC()
{
    return encoder.angle_inc;
}
float fGetEncoderOmega()
{
    return encoder.omega;
}
