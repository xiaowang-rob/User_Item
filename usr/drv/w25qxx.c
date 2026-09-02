// ******************************************************************************
// W25Q128 Flash存储器驱动实现
// 实现SPI接口的Flash存储器基本操作
//          包括初始化、读写、擦除、状态查询等功能
// ******************************************************************************
// TODO：均匀磨损策略
// 最小擦除单位为扇区总共4096个扇区
// 按照块分256个扇区组（16）
// 每组16个作为自由分配单元
// 然后整组定期换块均匀磨损
// 每个块的第一个扇区存储该块的累积擦除次数
// 然后在最后一个块中存储 当前块
// 初始化先从最后一个块开始读取块信息
// ******************************************************************************

#include "device.h"

#define W25Q128 0

// 存储器规格定义 ---------------------------------------------------------
#define FLASH_SIZE (128 / 8 * 1024 * 1024) ///< 总容量：16MB
#define FLASH_PAGE_SIZE 256                ///< 页大小：256字节
#define FLASH_PAGE_NUM 16                  ///< 页数量：16页
#define FLASH_SECTOR_NUM 16                ///< 扇区数量：16个
#define FLASH_BLOCK_NUM 256                ///< 块数量：256块

#define FLASH_SECTOR_SIZE 0x10000; // 扇区大小：4KB

const uint32_t sector_start_addr[FLASH_SECTOR_NUM] = {
    0x00000000,
    0x00001000,
    0x00002000,
    0x00003000,
    0x00004000,
    0x00005000,
    0x00006000,
    0x00007000,
    0x00008000,
    0x00009000,
    0x0000A000,
    0x0000B000,
    0x0000C000,
    0x0000D000,
    0x0000E000,
    0x0000F000,
};
// w25qxx配置表
#define W25Qxx_CHIP W25Q128

#ifdef W25Qxx_CHIP == W25Q128
const uint8_t W25Q128_CFG[] = {0xEF, 0x40, 0x18}; // W25Q128设备ID
#define FCMD_READ_ID 0x9F                         // 读取设备ID命令
#define FCMD_WRITE_ENABLE 0x06                    // 写使能命令
#define FCMD_WRITE_DISABLE 0x04                   // 写禁止命令
#define FCMD_READ_STATUS_REG 0x05                 // 读取状态寄存器命令
#define FCMD_ERASE_SECTOR 0x20                    // 扇区擦除命令（4KB）
#define FCMD_ERASE_CHIP 0x60                      // 全片擦除命令（64KB）
#define FCMD_READ_DATA 0x03                       // 读取数据命令
#define FCMD_WRITE_PAGE 0x02                      // 页编程命令
#define FBIT_SR_BUSY (0x01 << 0)                  // 状态寄存器BUSY位
#define FBIT_SR_WEL (0x01 << 1)                   // 状态寄存器WEL位

#define FTIMEOUT_WAIT_FREE 10      // 等待空闲超时（ms）
#define FTIMEOUT_REASE_SECTOR 1000 // 扇区擦除超时（ms）
#define FTIMEOUT_ERASE_CHIP 20000  // 全片擦除超时（ms）
#define FTIMEOUT_WRITE_PAGE 10     // 页编程超时（ms）

#endif

// 声明驱动上下文结构体
typedef struct
{
    eDeviceStatus Dstate;   // 设备状态
    uint8_t init_fault_tic; // 初始化失败重试计数器
    uint32_t block_idx;     // 当前块索引
} tW25Qxx_ctx;

// 函数操作表
static bool w25qxx_init(FlashHandle handle);
static uint32_t w25qxx_get_sector_size(FlashHandle handle, uint8_t sec_idx);
static uint32_t w25qxx_get_sector_addr(FlashHandle handle, uint8_t sec_idx);
static bool w25qxx_erase_sector(FlashHandle handle, uint8_t sec_idx);
static bool w25qxx_erase_chip(FlashHandle handle);
static bool w25qxx_read_data(FlashHandle handle, uint32_t ReadAddr, uint8_t *pBuffer, uint32_t NumByteToRead);
static bool w25qxx_write_data(FlashHandle handle, uint32_t WriteAddr, const uint8_t *pBuffer, uint32_t NumByteToWrite);
static uint8_t w25qxx_get_status(FlashHandle handle);

tFlashDriverBaseOps w25qxx_driver_baseops = {
    .Init = w25qxx_init,
    .Get_sector_size = w25qxx_get_sector_size,
    .Get_sector_addr = w25qxx_get_sector_addr,
    .Erase_sector = w25qxx_erase_sector,
    .Erase_chip = w25qxx_erase_chip,
    .Read = w25qxx_read_data,
    .Write = w25qxx_write_data,
    .Get_status = w25qxx_get_status};

// 句柄
FlashHandle w25qxx_create(void)
{
    tW25Qxx_ctx *ctx = (tW25Qxx_ctx *)calloc(1, sizeof(tW25Qxx_ctx));
    if (NULL == ctx)
        return NULL; // 内存分配失败
    return ctx;
}
void w25qxx_destroy(FlashHandle handle)
{
    if (NULL != handle)
        free(handle);
    handle = NULL;
}

// 内部函数
static void w25qxx_cs_enable(void)
{
    bsp_flash_cs(true);
}
static inline void w25qxx_cs_disable(void)
{
    bsp_flash_cs(false);
}
static inline bool w25qxx_transmit_one_byte(uint8_t _dataTx)
{
    return bsp_flash_spi_transmit(&_dataTx, 1, 1000);
}
static inline uint8_t w25qxx_receive_one_byte(void)
{
    uint8_t _dataRx = 0;
    bsp_flash_spi_receive(&_dataRx, 1, 1000);
    return _dataRx;
}
static inline void w25qxx_write_enable(void)
{
    w25qxx_cs_enable();
    w25qxx_transmit_one_byte(FCMD_WRITE_ENABLE); // Write Enable命令
    w25qxx_cs_disable();
}

static inline void w25qxx_write_disable(void)
{
    w25qxx_cs_enable();
    w25qxx_transmit_one_byte(FCMD_WRITE_DISABLE); // Write Disable命令
    w25qxx_cs_disable();
}

// 读取状态寄存器
// Bit0: BUSY（忙标志），Bit1: WEL（写使能锁存）
static inline uint8_t w25qxx_read_sr(void)
{
    uint8_t byte = 0;
    w25qxx_cs_enable();
    w25qxx_transmit_one_byte(FCMD_READ_STATUS_REG); // Read Status Register命令
    byte = w25qxx_receive_one_byte();               // 读取状态字节
    w25qxx_cs_disable();
    return byte;
}

// 等待Flash操作完成-flash 操作一定要等
// 轮询BUSY位，直到操作完成（BUSY=0）
static inline bool w25qxx_wait_busy(uint32_t timeout_ms)
{
    uint32_t timeout = bsp_get_tick() + timeout_ms;
    while (0x01 == (w25qxx_read_sr() & FBIT_SR_BUSY))
    {
        if (bsp_get_tick() > timeout)
            return false; // 超时
    }
    return true;
}

// 擦除单个扇区（4KB）
// Address: 扇区内任意地址
// 自动对齐到扇区边界，擦除需要一定时间
static inline bool w25qxx_erase_one_sector(uint32_t Address)
{
    bool status = false;
    w25qxx_write_enable();                         // 使能写操作
    status = w25qxx_wait_busy(FTIMEOUT_WAIT_FREE); // 等待空闲
    w25qxx_cs_enable();                            // 片选
    w25qxx_transmit_one_byte(FCMD_ERASE_SECTOR);   // Sector Erase命令（4KB）
    // 发送24位地址（MSB first）
    w25qxx_transmit_one_byte((uint8_t)((Address) >> 16));
    w25qxx_transmit_one_byte((uint8_t)((Address) >> 8));
    w25qxx_transmit_one_byte((uint8_t)Address);
    w25qxx_cs_disable();                              // 取消片选
    status = w25qxx_wait_busy(FTIMEOUT_REASE_SECTOR); // 等待擦除完成（典型时间45ms）
    return status;
}
// 页编程写入数据（单页内）
// pBuffer: 数据缓冲区
// WriteAddr: 写入起始地址
// NumByteToWrite: 写入字节数（≤256）
// true: 成功，false: 失败
// 必须在擦除的扇区内写入
static bool w25qxx_write_word(uint8_t *pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
    uint16_t i;
    w25qxx_write_enable(); // 写使能
    w25qxx_cs_enable();    // 片选

    if (!w25qxx_transmit_one_byte(FCMD_WRITE_PAGE)) // Page Program命令
    {
        w25qxx_cs_disable();
        return false;
    }

    // 发送24位地址
    w25qxx_transmit_one_byte((uint8_t)((WriteAddr) >> 16));
    w25qxx_transmit_one_byte((uint8_t)((WriteAddr) >> 8));
    w25qxx_transmit_one_byte((uint8_t)WriteAddr);

    // 连续写入数据
    for (i = 0; i < NumByteToWrite; i++)
        w25qxx_transmit_one_byte(pBuffer[i]);

    w25qxx_cs_disable();
    return w25qxx_wait_busy(FTIMEOUT_WRITE_PAGE); // 等待编程完成（典型时间0.7ms）
}

// 初始化w25qxx存储器
// 读取设备ID（0xEF4018）验证连接
// w25qxx初始化上电慢 失败时最多重试5次
static bool w25qxx_init(FlashHandle handle)
{
    if (NULL == handle)
        return false;
    tW25Qxx_ctx *ctx = (tW25Qxx_ctx *)handle;

    uint8_t id[3] = {0};
    w25qxx_cs_enable();                     // 片选
    bsp_delay(10);                          // 上电延时
    w25qxx_transmit_one_byte(FCMD_READ_ID); // Read JEDEC ID命令
    id[0] = w25qxx_receive_one_byte();      // Manufacturer ID
    id[1] = w25qxx_receive_one_byte();      // Memory Type
    id[2] = w25qxx_receive_one_byte();      // Capacity

    // 验证是否为W25Q128（Winbond 16MB）
    if ((id[0] == W25Q128_CFG[0]) && (id[1] == W25Q128_CFG[1]) && (id[2] == W25Q128_CFG[2]))
    {
        ctx->Dstate = ONLINE; // 设置在线状态
    }
    else if (ctx->init_fault_tic < 5) // 重试逻辑
    {
        ctx->init_fault_tic++;
        bsp_delay(10);
        w25qxx_init((FlashHandle *)ctx); // 递归重试
    }
    ctx->init_fault_tic = 0;
    w25qxx_cs_disable();          // 取消片选
    ctx->Dstate = OFFLINE;        // 设置离线状态
    return ctx->Dstate == ONLINE; // 返回初始化状态
}

static uint32_t w25qxx_get_sector_size(FlashHandle handle, uint8_t sec_idx)
{
    if (NULL == handle)
        return 0;
    return FLASH_SECTOR_SIZE; // 扇区大小：4KB
}
static uint32_t w25qxx_get_sector_addr(FlashHandle handle, uint8_t sec_idx)
{
    if (NULL == handle)
        return 0;
    return sector_start_addr[sec_idx]; // 扇区起始地址
}

static bool w25qxx_erase_sector(FlashHandle handle, uint8_t sec_idx)
{
    if (NULL == handle)
        return false;
    tW25Qxx_ctx *ctx = (tW25Qxx_ctx *)handle;
    // 计算起始和结束扇区
    uint32_t Address = sector_start_addr[sec_idx]; // 扇区起始地址
    uint16_t Star_Sector = Address / 4096;         // 起始扇区

    if (!w25qxx_erase_one_sector(Address)) // 擦除当前扇区
    {
        ctx->Dstate = RUN_ERROR;
        return false; // 擦除失败
    }

    ctx->Dstate = RUNNING;
    return true;
}

// 擦除整个芯片
// 全片擦除，等待时间较长（10-20秒）
// 非必要不建议使用
static bool w25qxx_erase_chip(FlashHandle handle)
{
    if (NULL == handle)
        return false;
    tW25Qxx_ctx *ctx = (tW25Qxx_ctx *)handle;

    bool status = false;
    w25qxx_write_enable();                          // 写使能
    status = w25qxx_wait_busy(FTIMEOUT_WAIT_FREE);  // 等待空闲
    w25qxx_cs_enable();                             // 片选
    w25qxx_transmit_one_byte(FCMD_ERASE_CHIP);      // Chip Erase命令
    w25qxx_cs_disable();                            // 取消片选
    status = w25qxx_wait_busy(FTIMEOUT_ERASE_CHIP); // 等待擦除完成
    ctx->Dstate = status ? RUNNING : RUN_ERROR;
    return status;
}
// 从Flash读取数据
// pBuffer: 数据缓冲区
// ReadAddr: 读取起始地址
// NumByteToRead: 读取字节数
// true: 成功，false: 失败
static bool w25qxx_read_data(FlashHandle handle,
                             uint32_t ReadAddr, uint8_t *pBuffer, uint32_t NumByteToRead)
{
    if (NULL == handle)
        return false;
    tW25Qxx_ctx *ctx = (tW25Qxx_ctx *)handle;

    uint16_t i = 0;
    w25qxx_cs_enable();                            // 片选
    if (!w25qxx_transmit_one_byte(FCMD_READ_DATA)) // Read Data命令
    {
        ctx->Dstate = RUN_ERROR;
        w25qxx_cs_disable();
        return false;
    }

    // 发送24位地址
    w25qxx_transmit_one_byte((uint8_t)((ReadAddr) >> 16));
    w25qxx_transmit_one_byte((uint8_t)((ReadAddr) >> 8));
    w25qxx_transmit_one_byte((uint8_t)ReadAddr);

    // 连续读取数据
    for (; i < NumByteToRead; i++)
    {
        pBuffer[i] = w25qxx_receive_one_byte();
    }

    w25qxx_cs_disable();
    ctx->Dstate = RUNNING;
    return true;
}

// 跨页写入数据（自动处理页边界）
// pBuffer: 数据缓冲区
// WriteAddr: 写入起始地址
// NumByteToWrite: 写入字节数
// 自动处理跨页情况，支持写入任意长度数据
static bool w25qxx_write_data(FlashHandle handle,
                              uint32_t WriteAddr, const uint8_t *pBuffer, uint32_t NumByteToWrite)
{
    if (NULL == handle)
        return false;
    tW25Qxx_ctx *ctx = (tW25Qxx_ctx *)handle;

    // 计算当前页剩余空间
    uint16_t Word_remain = 256 - WriteAddr % 256;
    if (NumByteToWrite <= Word_remain)
        Word_remain = NumByteToWrite;

    while (1)
    {
        // 写入当前页能容纳的数据
        if (!w25qxx_write_word(pBuffer, WriteAddr, Word_remain))
        {
            ctx->Dstate = RUN_ERROR;
            return false;
        }

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
    ctx->Dstate = RUNNING;
    return true;
}
static uint8_t w25qxx_get_status(FlashHandle handle)
{
    if (NULL == handle)
        return 0;
    tW25Qxx_ctx *ctx = (tW25Qxx_ctx *)handle;
    return ctx->Dstate;
}