/**
 ******************************************************************************
 * @file    flashDr.c
 * @brief   W25Q128 Flash存储器驱动实现
 * @details 实现SPI接口的Flash存储器基本操作
 *          包括初始化、读写、擦除、状态查询等功能
 ******************************************************************************
 * @note
 * - Flash写入前必须先擦除
 * - 单次编程最大256字节（一页）
 * - 扇区擦除大小4KB，块擦除大小64KB
 * - 等待操作完成需查询状态寄存器BUSY位
 ******************************************************************************
 */

#include "flashDr.h"
#include "device.h"

/* 私有函数声明 ----------------------------------------------------------*/
static void FLASH_Enable(void);
static void FLASH_Disable(void);
static bool spi_Transmit_one_byte(u8 _dataTx);
static u8 spi_Receive_one_byte(void);
static void FLASH_Write_Enable(void);
static void FLASH_Write_Disable(void);
static u8 FLASH_ReadSR(void);
static void FLASH_Wait_Busy(void);
static void fFLASH_WritePage(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite);

/* 私有变量 --------------------------------------------------------------*/
static u8 _init_fault_tic = 0; ///< 初始化失败重试计数器

/* 私有函数实现 ----------------------------------------------------------*/

/**
 * @brief  使能Flash芯片（拉低CS片选）
 */
static void FLASH_Enable(void)
{
    HAL_GPIO_WritePin(FLASH_CS_CPIOx, FLASH_CS_CPIOx_PIN, GPIO_PIN_RESET);
}

/**
 * @brief  禁用Flash芯片（拉高CS片选）
 */
static void FLASH_Disable(void)
{
    HAL_GPIO_WritePin(FLASH_CS_CPIOx, FLASH_CS_CPIOx_PIN, GPIO_PIN_SET);
}

/**
 * @brief  SPI发送单字节数据
 * @param  _dataTx: 待发送数据
 * @retval true: 发送成功，false: 发送失败
 */
static bool spi_Transmit_one_byte(u8 _dataTx)
{
    if (HAL_SPI_Transmit(&FLASH_SPI_Get_HSPI, (u8 *)&_dataTx, 1, 1000) == HAL_OK)
        return true;
    else
        return false;
}

/**
 * @brief  SPI接收单字节数据
 * @retval 接收到的数据
 */
static u8 spi_Receive_one_byte(void)
{
    u16 _dataRx;
    HAL_SPI_Receive(&FLASH_SPI_Get_HSPI, (u8 *)&_dataRx, 1, 1000);
    return _dataRx;
}

/**
 * @brief  Flash写使能（设置WEL位）
 * @note   任何编程或擦除操作前必须执行此命令
 */
static void FLASH_Write_Enable(void)
{
    FLASH_Enable();
    spi_Transmit_one_byte(0x06); // Write Enable命令
    FLASH_Disable();
}

/**
 * @brief  Flash写禁止（清除WEL位）
 */
static void FLASH_Write_Disable(void)
{
    FLASH_Enable();
    spi_Transmit_one_byte(0x04); // Write Disable命令
    FLASH_Disable();
}

/**
 * @brief  读取状态寄存器
 * @retval 状态寄存器值
 * @note   Bit0: BUSY（忙标志），Bit1: WEL（写使能锁存）
 */
static u8 FLASH_ReadSR(void)
{
    u8 byte = 0;
    FLASH_Enable();
    spi_Transmit_one_byte(0x05);   // Read Status Register-1命令
    byte = spi_Receive_one_byte(); // 读取状态字节
    FLASH_Disable();
    return byte;
}

/**
 * @brief  等待Flash操作完成
 * @note   轮询BUSY位，直到操作完成（BUSY=0）
 */
static void FLASH_Wait_Busy(void)
{
    while ((FLASH_ReadSR() & 0x01) == 0x01)
    {
        // 等待BUSY位清空
    }
}

/**
 * @brief  擦除单个扇区（4KB）
 * @param  Address: 扇区内任意地址
 * @note   自动对齐到扇区边界，擦除需要一定时间
 */
void fEraseOneSector(u32 Address)
{
    FLASH_Write_Enable();        // 使能写操作
    FLASH_Wait_Busy();           // 等待空闲
    FLASH_Enable();              // 片选
    spi_Transmit_one_byte(0x20); // Sector Erase命令（4KB）
    // 发送24位地址（MSB first）
    spi_Transmit_one_byte((u8)((Address) >> 16));
    spi_Transmit_one_byte((u8)((Address) >> 8));
    spi_Transmit_one_byte((u8)Address);
    FLASH_Disable();   // 取消片选
    FLASH_Wait_Busy(); // 等待擦除完成（典型时间45ms）
}

/**
 * @brief  擦除指定地址范围的扇区
 * @param  Address: 起始地址
 * @param  Write_data_NUM: 需要写入的数据字节数
 * @note   自动计算需要擦除的扇区范围
 */
void fFLASH_EraseSector(u32 Address, u32 Write_data_NUM)
{
    // 计算起始和结束扇区
    u16 Star_Sector = Address / 4096;                   // 起始扇区
    u16 End_Sector = (Address + Write_data_NUM) / 4096; // 结束扇区
    u16 Num_Sector = End_Sector - Star_Sector;          // 跨扇区数

    // 擦除所有相关扇区
    for (u16 i = 0; i <= Num_Sector; i++)
    {
        fEraseOneSector(Address); // 擦除当前扇区
        Address += 4096;          // 移动到下一个扇区
    }
}

/**
 * @brief  擦除整个芯片
 * @note   全片擦除，等待时间较长（10-20秒）
 *         非必要不建议使用
 */
void fEraseFLASHChip(void)
{
    FLASH_Write_Enable();        // 写使能
    FLASH_Wait_Busy();           // 等待空闲
    FLASH_Enable();              // 片选
    spi_Transmit_one_byte(0x60); // Chip Erase命令
    FLASH_Disable();             // 取消片选
    FLASH_Wait_Busy();           // 等待擦除完成
}

/**
 * @brief  从Flash读取数据
 * @param  pBuffer: 数据缓冲区
 * @param  ReadAddr: 读取起始地址
 * @param  NumByteToRead: 读取字节数
 * @retval true: 成功，false: 失败
 */
bool fFLASH_ReadData(u8 *pBuffer, u32 ReadAddr, u16 NumByteToRead)
{
    u16 i = 0;
    FLASH_Enable();                   // 片选
    if (!spi_Transmit_one_byte(0x03)) // Read Data命令
    {
        g_device_status.flash_state = RUN_ERROR;
        FLASH_Disable();
        return false;
    }

    // 发送24位地址
    spi_Transmit_one_byte((u8)((ReadAddr) >> 16));
    spi_Transmit_one_byte((u8)((ReadAddr) >> 8));
    spi_Transmit_one_byte((u8)ReadAddr);

    // 连续读取数据
    for (; i < NumByteToRead; i++)
    {
        pBuffer[i] = spi_Receive_one_byte();
    }

    FLASH_Disable();
    g_device_status.flash_state = ONLINE; // 设置在线状态
    return true;
}

/**
 * @brief  页编程写入数据（单页内）
 * @param  pBuffer: 数据缓冲区
 * @param  WriteAddr: 写入起始地址
 * @param  NumByteToWrite: 写入字节数（≤256）
 * @retval true: 成功，false: 失败
 * @note   必须在擦除的扇区内写入，跨页写入需调用fFLASH_WritePage
 */
bool fFLASH_WriteWord(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
    u16 i;
    FLASH_Write_Enable(); // 写使能
    FLASH_Enable();       // 片选

    if (!spi_Transmit_one_byte(0x02)) // Page Program命令
    {
        g_device_status.flash_state = RUN_ERROR;
        FLASH_Disable();
        return false;
    }

    // 发送24位地址
    spi_Transmit_one_byte((u8)((WriteAddr) >> 16));
    spi_Transmit_one_byte((u8)((WriteAddr) >> 8));
    spi_Transmit_one_byte((u8)WriteAddr);

    // 连续写入数据
    for (i = 0; i < NumByteToWrite; i++)
        spi_Transmit_one_byte(pBuffer[i]);

    FLASH_Disable();
    FLASH_Wait_Busy(); // 等待编程完成（典型时间0.7ms）
    g_device_status.flash_state = ONLINE;
    return true;
}

/**
 * @brief  跨页写入数据（自动处理页边界）
 * @param  pBuffer: 数据缓冲区
 * @param  WriteAddr: 写入起始地址
 * @param  NumByteToWrite: 写入字节数
 * @note   自动处理跨页情况，支持写入任意长度数据
 */
static void fFLASH_WritePage(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
    // 计算当前页剩余空间
    u16 Word_remain = 256 - WriteAddr % 256;
    if (NumByteToWrite <= Word_remain)
        Word_remain = NumByteToWrite;

    while (1)
    {
        // 写入当前页能容纳的数据
        fFLASH_WriteWord(pBuffer, WriteAddr, Word_remain);

        if (NumByteToWrite == Word_remain)
        {
            break; // 全部写入完成
        }
        else // 需要跨页写入
        {
            // 更新指针和地址
            pBuffer += Word_remain;
            WriteAddr += Word_remain;
            NumByteToWrite -= Word_remain;

            // 计算下一页可写入量
            if (NumByteToWrite > 256)
                Word_remain = 256; // 下一页可写满
            else
                Word_remain = NumByteToWrite; // 最后一页
        }
    }
}
/**
 * @brief  初始化Flash存储器
 * @note   读取设备ID（0xEF4018）验证连接
 *         失败时最多重试5次
 */
void fFLASH_Init(void)
{
    u8 id[3] = {0};
    FLASH_Enable(); // 片选
    HAL_Delay(100); // 上电延时

    spi_Transmit_one_byte(0x9F);    // Read JEDEC ID命令
    id[0] = spi_Receive_one_byte(); // Manufacturer ID
    id[1] = spi_Receive_one_byte(); // Memory Type
    id[2] = spi_Receive_one_byte(); // Capacity

    // 验证是否为W25Q128（Winbond 16MB）
    if ((id[0] == 0xEF) && (id[1] == 0x40) && (id[2] == 0x18))
    {
        g_device_status.flash_state = ONLINE; // 设置在线状态
    }
    else if (_init_fault_tic < 5) // 重试逻辑
    {
        _init_fault_tic++;
        fFLASH_Init(); // 递归重试
    }

    FLASH_Disable(); // 取消片选
}