#include "encoder.h"
#include "string.h"
#include "stdbool.h"
#include "spi.h"
#include "math_fast.h"
ENCODER_t encoder = {0};
static bool DealDone_flag = false;

/**
 * @brief 8位SPI读写函数
 * @param cmd: 发送的命令字节
 * @param rx_data: 接收的数据字节
 * @return true: 成功，false: 失败
 */
static bool MT6816_SPI_ReadWrite(u8 cmd, u8 *rx_data)
{
    ENCODER_SPI_CS_L();

    // 发送命令并接收数据（8位模式）
    u8 _status = HAL_SPI_TransmitReceive_DMA(&ENcoder_SPI_Get_HSPI, &cmd, rx_data, 1);

    if (_status != HAL_OK)
    {
        if (_status == HAL_ERROR)
            encoder.online_flag = false;
        ENCODER_SPI_CS_H();
        return false;
    }
    encoder.online_flag = true;
    ENCODER_SPI_CS_H();
    return true;
}

/**
 * @brief 读取MT6816寄存器值
 * @param reg_addr: 寄存器地址
 * @param data: 读取到的数据
 * @return true: 读取成功，false: 失败
 */
static bool MT6816_ReadRegister(u8 reg_addr, u8 *data)
{
    return MT6816_SPI_ReadWrite(reg_addr, data);
}

/**
 * @brief 读取MT6816所有数据（角度 + 状态）
 * @param data: 输出数据结构体
 * @return true: 读取成功，false: 失败
 */
u8 valid = 0;
u8 reg03_data = 0;
u8 reg04_data = 0;
void ENCODER_ReadData()
{
    // 读取寄存器0x03（角度高位：Angle<13:6>）
    if (!MT6816_ReadRegister(MT6816_REG_ANGLE_HIGH, &reg03_data))
    {
        return;
    }
    // 读取寄存器0x04（角度低位 + 状态位：Angle<5:0> + No_Mag_Warning + PC）
    if (!MT6816_ReadRegister(MT6816_REG_ANGLE_LOW, &reg04_data))
    {
        DealDone_flag = false;
        return;
    }
}
void ENCODER_DEALDATA()
{

    // 解析14位角度值
    // reg03_data: Angle<13:6> (高8位)
    // reg04_data: Angle<5:0> (低6位) + 状态位
    u16 angle_high = (reg03_data & 0xFF); // 8位数据
    u16 angle_low = (reg04_data & 0xFC);  // 取高6位作为Angle<5:0>
    // 组合14位角度值：Angle<13:6> << 6 + Angle<5:0>
    float angle_raw = ((angle_high & 0x3F) << 6) | ((angle_low >> 2) & 0x3F);

    // 转换为角度值（0~360°）
    encoder.angle_deg = (float)angle_raw * 0.02197265625f;                        // *360/16384  16384 = 2^14
    encoder.angle_rad = encoder.angle_deg * 0.0174532922f + encoder.angle_offset; // 角度值转弧度值
    encoder.angle_inc += encoder.angle_rad - encoder.angle_last;
    encoder.angle_last = encoder.angle_rad;
    // 提取状态位（从reg04_data和reg05_data中）
    encoder.no_mag_flag = (reg04_data & MT6816_NO_MAG_WARNING) ? true : false;
    bool parity_check = (reg04_data & MT6816_PARITY_CHECK) ? true : false;

    // 奇偶校验验证
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
        encoder.communication_error = true;
    else
        encoder.communication_error = false;
    DealDone_flag = true;
    ENCODER_ReadData();
}
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == (&ENcoder_SPI_Get_HSPI))
    {
        if (DealDone_flag == false)
            ENCODER_DEALDATA();
    }
}

bool ENCODER_Init()
{
    memset(&encoder, 0, sizeof(ENCODER_t));
    ENCODER_ReadData();
    return encoder.online_flag;
}
float GET_ENCODER_ANGLE_RAD(void)
{
    return encoder.angle_rad;
}
float GET_ENCODER_ANGLE_INC(void)
{
    return encoder.angle_inc;
}
void SET_ENCODER_ANGLE_OFFSET(float offset)
{
    encoder.angle_offset = offset;
}
float GET_ENCODER_ANGLE_OFFSET(void)
{
    return encoder.angle_offset;
}
bool GET_ENCODER_STATUS()
{
    return encoder.online_flag;
}
bool GET_ENCODER_COMMUNICATION_ERROR()
{
    return encoder.communication_error;
}
bool GET_ENCODER_NO_MAG_FLAG()
{
    return encoder.no_mag_flag;
}
