#include "flashDr.h"
#include "drive_state.h"
#include "device.h"
// 要读的数据
u8 Read_data[100] = {0};
#define Read_data_SIZE sizeof(Read_data)

/* Nicky ******************************************************************* */
// 器件使能
void FLASH_Enable()
{
    HAL_GPIO_WritePin(FLASH_CS_CPIOx, FLASH_CS_CPIOx_PIN, GPIO_PIN_RESET);
}

/* Nicky ******************************************************************* */
// 器件失能
void FLASH_Disable()
{
    HAL_GPIO_WritePin(FLASH_CS_CPIOx, FLASH_CS_CPIOx_PIN, GPIO_PIN_SET);
}

/* Nicky ******************************************************************* */
// SPI2 发送 1 个字节数据
bool spi_Transmit_one_byte(u8 _dataTx)
{
    if (HAL_SPI_Transmit(&FLASH_SPI_Get_HSPI, (u8 *)&_dataTx, 1, 1000) == HAL_OK)
        return true;
    else
        return false;
}

/* Nicky ******************************************************************* */
// SPI2 接收 1 个字节数据
u8 spi_Receive_one_byte()
{
    u16 _dataRx;
    HAL_StatusTypeDef state;
    state = HAL_SPI_Receive(&FLASH_SPI_Get_HSPI, (u8 *)&_dataRx, 1, 1000);
    return _dataRx;
}

/* Nicky ******************************************************************* */
// W25Q128写使能,将WEL置1
void FLASH_Write_Enable()
{
    FLASH_Enable(); // 使能器件
    spi_Transmit_one_byte(0x06);
    FLASH_Disable(); // 取消片选
}

/* Nicky ******************************************************************* */
// W25Q128写失能,将WEL置0
void FLASH_Write_Disable()
{
    FLASH_Enable(); // 使能器件
    spi_Transmit_one_byte(0x04);
    FLASH_Disable(); // 取消片选
}

/* Nicky ******************************************************************* */
// 读取寄存器状态
u8 FLASH_ReadSR(void)
{
    u8 byte = 0;
    FLASH_Enable();                // 使能器件
    spi_Transmit_one_byte(0x05);   // 发送读取状态寄存器命令
    byte = spi_Receive_one_byte(); // 读取一个字节
    FLASH_Disable();               // 取消片选
    return byte;
}

/* Nicky ******************************************************************* */
// 等待空闲
void FLASH_Wait_Busy()
{
    while ((FLASH_ReadSR() & 0x01) == 0x01)
        ; // 等待BUSY位清空
}

/* Nicky ******************************************************************* */
// 擦除地址所在的一个扇区
void Erase_one_Sector(u32 Address)
{
    FLASH_Write_Enable(); // SET WEL
    FLASH_Wait_Busy();
    FLASH_Enable();                               // 使能器件
    spi_Transmit_one_byte(0x20);                  // 发送扇区擦除指令
    spi_Transmit_one_byte((u8)((Address) >> 16)); // 发送24bit地址
    spi_Transmit_one_byte((u8)((Address) >> 8));
    spi_Transmit_one_byte((u8)Address);
    FLASH_Disable();   // 取消片选
    FLASH_Wait_Busy(); // 等待擦除完成
}

/* Nicky ******************************************************************* */
// 擦除地址所在的扇区
void FLASH_erase_sector(u32 Address, u32 Write_data_NUM)
{
    // 总共4096个扇区
    // 计算 写入数据开始的地址 + 要写入数据个数的最后地址 所处的扇区
    u16 Star_Sector, End_Sector, Num_Sector;
    Star_Sector = Address / 4096;                   // 数据写入开始的扇区
    End_Sector = (Address + Write_data_NUM) / 4096; // 数据写入结束的扇区
    Num_Sector = End_Sector - Star_Sector;          // 数据写入跨几个扇区

    // 开始擦除扇区
    for (u16 i = 0; i <= Num_Sector; i++)
    {
        Erase_one_Sector(Address);
        Address += 4095;
    }
}

/* Nicky ******************************************************************* */
// 擦除整个芯片 等待时间超长... 10-20S
void Erase_FLASH_Chip(void)
{
    FLASH_Write_Enable(); // SET WEL
    FLASH_Wait_Busy();
    FLASH_Enable();              // 使能器件
    spi_Transmit_one_byte(0x60); // 发送片擦除命令
    FLASH_Disable();             // 取消片选
    FLASH_Wait_Busy();           // 等待芯片擦除结束
}

/* Nicky ******************************************************************* */
// 读取W25Q128数据
bool FLASH_Read_data(u8 *pBuffer, u32 ReadAddr, u16 NumByteToRead)
{
    u16 i = 0;
    FLASH_Enable();                   // 使能器件
    if (!spi_Transmit_one_byte(0x03)) // 发送读取命令
    {
        FLASH_state_set(RUN_ERROR);
        return false;
    }
    spi_Transmit_one_byte((u8)((ReadAddr) >> 16)); // 发送24bit地址
    spi_Transmit_one_byte((u8)((ReadAddr) >> 8));
    spi_Transmit_one_byte((u8)ReadAddr);
    for (; i < NumByteToRead; i++)
    {
        pBuffer[i] = spi_Receive_one_byte(); // 循环读数
    }
    FLASH_Disable();
    return true;
}

/* Nicky ******************************************************************* */
// 写字，一次最多一页
bool FLASH_Write_Word(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
    u16 i;
    FLASH_Write_Enable(); // SET WEL
    FLASH_Enable();       // 使能器件
    if (!spi_Transmit_one_byte(0x02))
    {
        FLASH_state_set(RUN_ERROR);
        return false;
    }
    spi_Transmit_one_byte((u8)((WriteAddr) >> 16)); // 写入的目标地址
    spi_Transmit_one_byte((u8)((WriteAddr) >> 8));
    spi_Transmit_one_byte((u8)WriteAddr);
    for (i = 0; i < NumByteToWrite; i++)
        spi_Transmit_one_byte(pBuffer[i]); // 循环写入字节数据
    FLASH_Disable();
    FLASH_Wait_Busy(); // 写完之后需要等待芯片操作完。
    return true;
}

/* Nicky ******************************************************************* */
// 定位到页
void FLASH_Write_Page(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
    u16 Word_remain;
    Word_remain = 256 - WriteAddr % 256; // 定位页剩余的字数

    if (NumByteToWrite <= Word_remain)
        Word_remain = NumByteToWrite; // 定位页能一次写完
    while (1)
    {
        FLASH_Write_Word(pBuffer, WriteAddr, Word_remain);
        if (NumByteToWrite == Word_remain)
        {
            break; // 判断写完就 break
        }
        else // 没写完，翻页了
        {
            pBuffer += Word_remain; // 直针后移当页已写字数
            WriteAddr += Word_remain;
            NumByteToWrite -= Word_remain; // 减去已经写入了的字数
            if (NumByteToWrite > 256)
                Word_remain = 256; // 一次可以写入256个字
            else
                Word_remain = NumByteToWrite; // 不够256个字了
        }
    }
}

static u8 _init_fault_tic = 0;
void FLASH_Init(void)
{
    u8 id[3] = {0};
    FLASH_Enable(); // 使能器件
    HAL_Delay(1);
    spi_Transmit_one_byte(0x9F);                               // 读取ID
    id[0] = spi_Receive_one_byte();                            // 读取一个字节
    id[1] = spi_Receive_one_byte();                            // 读取另一个字节
    id[2] = spi_Receive_one_byte();                            // 读取第三个字节
    if ((id[0] == 0xEF) && (id[1] == 0x40) && (id[2] == 0x18)) // 校验ID
    {
        FLASH_state_set(ONLINE);
    }
    else if (_init_fault_tic < 5)
    {
        _init_fault_tic++;
        FLASH_Init();
    }
    FLASH_Disable(); // 取消片选
}