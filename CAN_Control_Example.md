# XDr-P 电机驱动器 CAN 控制例程

适用于 STM32F4 / STM32H7 主控端，通过 CAN 总线控制 XDr-P 驱动器。

## 1. 协议说明

### 1.1 CAN 帧格式

使用标准帧（11位ID），波特率 500kbps。

**驱动器端解析规则**（基于 `port_mapping.c` 的 `fCAN_RxDataCallback`）：

| 数据长度 | 用途 | 帧格式 |
|---------|------|--------|
| 1 字节 | 单字节命令 | `[cmd_id]` |
| 2 字节 | 命令 + 1字节参数 | `[cmd_id, param]` |
| 4 字节 | 目标值设置 | `[float_l, float_h, ...]` → 自动识别为 `CMD_REFVALUE_SET` |
| 8 字节 | 双目标值设置 | `[float1(4B), float2(4B)]` → 自动识别为 `CMD_REFVALUE_SET` |

> CAN ID = 驱动器节点 ID（通过参数 `CAN_ID` 配置）

### 1.2 命令 ID 定义

```c
/* 控制命令 */
#define CMD_ENABLE            0xF4    /* 电机使能 */
#define CMD_DISABLE           0xF5    /* 电机失能 */
#define CMD_MODE_SET          0x22    /* 模式设置（1字节参数） */
#define CMD_REFVALUE_SET      0x21    /* 目标值设置（4/8字节float） */
#define CMD_STREAM_GET        0x23    /* 监测值获取（返回4字节float） */
#define CMD_STREAM_SET        0x25    /* 数据流设置（N字节数据ID） */
#define CMD_SET_ZERO_POS      0x26    /* 设置零点 */
#define CMD_SET_LIMIT_POS     0x27    /* 设置极限位置 */
#define CMD_SYSTEM_RESET      0x30    /* 系统复位 */
#define BRAKE                 0xF2    /* 刹车 */
#define FOC_NRST              0xF3    /* FOC复位 */
#define START_TUNNING         0xF1    /* 开始调参 */

/* 参数操作 */
#define PARAM_ERASE           0x01    /* 参数擦除 */
#define PARAM_WRITE           0x02    /* 参数写入 */
#define PARAM_READ            0x03    /* 参数读取 */
#define PARAM_SAVE            0x04    /* 参数保存 */

/* 反馈 */
#define FEEDBACK_EXECUTE      0xF0    /* 成功 */
#define FEEDBACK_FAILURE      0xFE    /* 失败 */
```

### 1.3 运行模式

```c
typedef enum {
    CURRENT_MODE  = 0,    /* 电流模式，目标值单位: A */
    SPEED_MODE    = 1,    /* 速度模式，目标值单位: rpm */
    POSITION_MODE = 2,    /* 位置模式，目标值单位: deg */
    OPEN_LOOP     = 3,    /* 开环模式 */
} eRunMode;
```

### 1.4 数据流 ID（用于 `CMD_STREAM_GET`）

```c
typedef enum {
    CURRENT_U     =  0,    /* U相电流 */
    CURRENT_V     =  1,    /* V相电流 */
    CURRENT_W     =  2,    /* W相电流 */
    VOLTAGE_Q     =  3,    /* q轴电压 */
    VOLTAGE_D     =  4,    /* d轴电压 */
    CURRENT_ALPHA =  5,    /* α轴电流 */
    CURRENT_BETA  =  6,    /* β轴电流 */
    CURRENT_Q     =  7,    /* q轴电流 */
    CURRENT_D     =  8,    /* d轴电流 */
    CURRENT_Q_REF =  9,    /* Iq_ref */
    CURRENT_D_REF = 10,    /* Id_ref */
    SPEED         = 11,    /* 速度 (rpm) */
    SPEED_REF     = 12,    /* 目标速度 */
    THETA_ELEC    = 13,    /* 电角度 */
    THETA_MECH    = 14,    /* 机械角度 */
    POSITION      = 15,    /* 位置 (deg) */
    POSITION_REF  = 16,    /* 目标位置 */
} eData_stream;
```

### 1.5 参数索引（用于 `PARAM_WRITE` / `PARAM_READ`）

```c
typedef enum {
    CAN_ID             =  8,    /* CAN ID */
    MOTOR_POLEPAIRS    =  7,    /* 电机极对数 */
    MOTOR_RS           = 11,    /* 相电阻 */
    MOTOR_Ld           = 12,    /* Ld */
    MOTOR_Lq           = 13,    /* Lq */
    MOTOR_PSIF         = 14,    /* 磁链 */
    MOTOR_KE           = 15,    /* 反电动势常数 */
    KP_SPEED           = 18,    /* 速度环比例 */
    KI_SPEED           = 19,    /* 速度环积分 */
    KP_POSITION        = 20,    /* 位置环比例 */
    KI_POSITION        = 21,    /* 位置环积分 */
    KD_POSITION        = 22,    /* 位置环微分 */
    LIMIT_CURRENT      = 24,    /* 电流限幅 */
    LIMIT_SPEED        = 25,    /* 速度限幅 */
    /* ... 更多参数见 protocol.h */
} eParameter;
```

---

## 2. STM32F4 CAN 控制例程

### 2.1 CAN 初始化（HAL库）

```c
/*
 * can_drv.c - STM32F4 CAN 驱动层
 * 使用 CAN1, PB8(RX)/PB9(TX), 500kbps
 */

#include "stm32f4xx_hal.h"
#include <string.h>

CAN_HandleTypeDef hcan1;

/* CAN 过滤器配置 - 接收所有标准帧 */
static void CAN_Filter_Config(void)
{
    CAN_FilterTypeDef filter;
    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0x0000;
    filter.FilterIdLow = 0x0000;
    filter.FilterMaskIdHigh = 0x0000;
    filter.FilterMaskIdLow = 0x0000;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14;
    HAL_CAN_ConfigFilter(&hcan1, &filter);
}

/* CAN1 初始化: 500kbps, APB1=42MHz */
void CAN_Driver_Init(void)
{
    hcan1.Instance = CAN1;
    hcan1.Init.Prescaler = 6;            // 42MHz / 6 / (1+9+2) = 500kbps
    hcan1.Init.Mode = CAN_MODE_NORMAL;
    hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan1.Init.TimeSeg1 = CAN_BS1_9TQ;
    hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
    hcan1.Init.TimeTriggeredMode = DISABLE;
    hcan1.Init.AutoBusOff = ENABLE;
    hcan1.Init.AutoWakeUp = DISABLE;
    hcan1.Init.AutoRetransmission = ENABLE;
    hcan1.Init.ReceiveFifoLocked = DISABLE;
    hcan1.Init.TransmitFifoPriority = DISABLE;
    HAL_CAN_Init(&hcan1);

    CAN_Filter_Config();

    HAL_CAN_Start(&hcan1);
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
}

/* CAN 发送 */
int CAN_Send(uint32_t id, uint8_t *data, uint8_t len)
{
    CAN_TxHeaderTypeDef header;
    uint32_t mailbox;

    header.StdId = id;
    header.ExtId = 0;
    header.RTR = CAN_RTR_DATA;
    header.IDE = CAN_ID_STD;
    header.DLC = len;

    if (HAL_CAN_AddTxMessage(&hcan1, &header, data, &mailbox) != HAL_OK)
        return -1;

    /* 等待发送完成 */
    uint32_t tick = HAL_GetTick();
    while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) != 3) {
        if (HAL_GetTick() - tick > 10) return -2;
    }
    return 0;
}

/* CAN 接收缓冲区 */
static volatile uint8_t rx_data[8];
static volatile uint8_t rx_len = 0;
static volatile uint32_t rx_id = 0;
static volatile uint8_t rx_flag = 0;

/* FIFO0 接收中断回调 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef header;
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &header, (uint8_t*)rx_data) == HAL_OK) {
        rx_id = header.StdId;
        rx_len = header.DLC;
        rx_flag = 1;
    }
}

/* 非阻塞接收，返回 1=有数据, 0=无数据 */
int CAN_Recv(uint32_t *id, uint8_t *data, uint8_t *len)
{
    if (!rx_flag) return 0;
    *id = rx_id;
    *len = rx_len;
    memcpy(data, (const void*)rx_data, rx_len);
    rx_flag = 0;
    return 1;
}
```

### 2.2 驱动器控制层

```c
/*
 * x_drive.c - XDr-P 驱动器控制协议层
 */

#include <stdint.h>
#include <string.h>

/* ---------- 来自驱动器的协议定义 ---------- */
#define CMD_ENABLE            0xF4
#define CMD_DISABLE           0xF5
#define CMD_MODE_SET          0x22
#define CMD_REFVALUE_SET      0x21
#define CMD_STREAM_GET        0x23
#define CMD_STREAM_SET        0x25
#define CMD_SET_ZERO_POS      0x26
#define CMD_SYSTEM_RESET      0x30
#define BRAKE                 0xF2
#define FOC_NRST              0xF3
#define START_TUNNING         0xF1
#define PARAM_WRITE           0x02
#define PARAM_READ            0x03
#define PARAM_SAVE            0x04
#define PARAM_ERASE           0x01

#define FEEDBACK_EXECUTE      0xF0
#define FEEDBACK_FAILURE      0xFE

/* 运行模式 */
#define MODE_CURRENT    0
#define MODE_SPEED      1
#define MODE_POSITION   2
#define MODE_OPEN_LOOP  3

/* 数据流 ID */
#define STREAM_IQ       7
#define STREAM_ID       8
#define STREAM_SPEED    11
#define STREAM_POSITION 15
#define STREAM_POSITION_REF 16
#define STREAM_IQ_REF   9

/* 外部接口（由上面的 can_drv.c 提供） */
extern int CAN_Send(uint32_t id, uint8_t *data, uint8_t len);
extern int CAN_Recv(uint32_t *id, uint8_t *data, uint8_t *len);
extern uint32_t HAL_GetTick(void);

/* ==================== 命令发送函数 ==================== */

/**
 * @brief 使能电机
 * @param drv_id 驱动器 CAN ID
 */
int xDrive_Enable(uint32_t drv_id)
{
    uint8_t cmd = CMD_ENABLE;
    return CAN_Send(drv_id, &cmd, 1);
}

/**
 * @brief 失能电机
 */
int xDrive_Disable(uint32_t drv_id)
{
    uint8_t cmd = CMD_DISABLE;
    return CAN_Send(drv_id, &cmd, 1);
}

/**
 * @brief 刹车
 */
int xDrive_Brake(uint32_t drv_id)
{
    uint8_t cmd = BRAKE;
    return CAN_Send(drv_id, &cmd, 1);
}

/**
 * @brief FOC 复位
 */
int xDrive_FOC_Reset(uint32_t drv_id)
{
    uint8_t cmd = FOC_NRST;
    return CAN_Send(drv_id, &cmd, 1);
}

/**
 * @brief 系统复位
 */
int xDrive_SystemReset(uint32_t drv_id)
{
    uint8_t cmd = CMD_SYSTEM_RESET;
    return CAN_Send(drv_id, &cmd, 1);
}

/**
 * @brief 设置零点（以当前位置为零点）
 */
int xDrive_SetZeroPos(uint32_t drv_id)
{
    uint8_t cmd = CMD_SET_ZERO_POS;
    return CAN_Send(drv_id, &cmd, 1);
}

/**
 * @brief 设置运行模式
 * @param mode: MODE_CURRENT(0), MODE_SPEED(1), MODE_POSITION(2), MODE_OPEN_LOOP(3)
 */
int xDrive_SetMode(uint32_t drv_id, uint8_t mode)
{
    uint8_t data[2] = { CMD_MODE_SET, mode };
    return CAN_Send(drv_id, data, 2);
}

/**
 * @brief 设置目标值（单float，4字节）
 * @param value 目标值（电流A / 速度rpm / 位置deg，取决于当前模式）
 *
 * 驱动器端：收到4字节数据自动识别为 CMD_REFVALUE_SET
 */
int xDrive_SetTarget(uint32_t drv_id, float value)
{
    uint8_t data[4];
    memcpy(data, &value, 4);
    return CAN_Send(drv_id, data, 4);
}

/**
 * @brief 设置双目标值（8字节）
 * @param v1 第一个目标值（位置模式下为目标位置）
 * @param v2 第二个目标值（位置模式+PVT下为目标速度）
 *
 * 驱动器端：收到8字节数据自动识别为 CMD_REFVALUE_SET
 */
int xDrive_SetTarget2(uint32_t drv_id, float v1, float v2)
{
    uint8_t data[8];
    memcpy(data,     &v1, 4);
    memcpy(data + 4, &v2, 4);
    return CAN_Send(drv_id, data, 8);
}

/**
 * @brief 查询监测值
 * @param stream_id 数据流 ID（见 eData_stream）
 * @param value     输出：返回的float值
 * @return 0=成功, -1=超时
 *
 * 发送: [CMD_STREAM_GET, stream_id] (2字节)
 * 接收: [float: 4字节]
 */
int xDrive_StreamGet(uint32_t drv_id, uint8_t stream_id, float *value)
{
    uint8_t tx[2] = { CMD_STREAM_GET, stream_id };
    CAN_Send(drv_id, tx, 2);

    /* 等待响应 */
    uint32_t tick = HAL_GetTick();
    uint32_t rx_id;
    uint8_t rx_data[8], rx_len;

    while (HAL_GetTick() - tick < 50) {
        if (CAN_Recv(&rx_id, rx_data, &rx_len)) {
            if (rx_len == 4) {
                memcpy(value, rx_data, 4);
                return 0;
            }
        }
    }
    return -1; /* 超时 */
}

/**
 * @brief 设置数据流（上位机连接后持续推送）
 * @param stream_ids 数据流 ID 数组
 * @param count      数组长度（最多8个）
 */
int xDrive_StreamSet(uint32_t drv_id, uint8_t *stream_ids, uint8_t count)
{
    if (count > 8) count = 8;
    uint8_t data[9];
    data[0] = CMD_STREAM_SET;
    memcpy(&data[1], stream_ids, count);
    return CAN_Send(drv_id, data, count + 1);
}

/**
 * @brief 写入参数
 * @param param_idx 参数索引（见 eParameter）
 * @param value     参数值（float）
 *
 * 发送: [PARAM_WRITE, param_idx, float: 4B] = 6字节 → 驱动器解析为 cmd=PARAM_WRITE
 */
int xDrive_ParamWrite(uint32_t drv_id, uint8_t param_idx, float value)
{
    uint8_t data[6];
    data[0] = PARAM_WRITE;
    data[1] = param_idx;
    memcpy(&data[2], &value, 4);
    return CAN_Send(drv_id, data, 6);
}

/**
 * @brief 读取参数
 * @param param_idx 参数索引
 * @param value     输出：参数值
 * @return 0=成功, -1=超时
 *
 * 发送: [PARAM_READ, param_idx] = 2字节
 * 接收: [param_idx, float: 4B] = 5字节 (由上位机通道回传)
 *
 * 注意: 驱动器对 CAN 的 PARAM_READ 可能走不同通道，
 *       建议通过 USB/UART 上位机读取参数。
 *       CAN 端口主要用于实时控制。
 */
int xDrive_ParamRead(uint32_t drv_id, uint8_t param_idx, float *value)
{
    uint8_t tx[2] = { PARAM_READ, param_idx };
    CAN_Send(drv_id, tx, 2);

    uint32_t tick = HAL_GetTick();
    uint32_t rx_id;
    uint8_t rx_data[8], rx_len;

    while (HAL_GetTick() - tick < 50) {
        if (CAN_Recv(&rx_id, rx_data, &rx_len)) {
            if (rx_len >= 5) {
                memcpy(value, &rx_data[1], 4);
                return 0;
            }
        }
    }
    return -1;
}

/**
 * @brief 保存参数到 Flash
 */
int xDrive_ParamSave(uint32_t drv_id)
{
    uint8_t cmd = PARAM_SAVE;
    return CAN_Send(drv_id, &cmd, 1);
}

/**
 * @brief 擦除参数
 */
int xDrive_ParamErase(uint32_t drv_id)
{
    uint8_t cmd = PARAM_ERASE;
    return CAN_Send(drv_id, &cmd, 1);
}

/**
 * @brief 开始自动校准
 */
int xDrive_StartTunning(uint32_t drv_id)
{
    uint8_t cmd = START_TUNNING;
    return CAN_Send(drv_id, &cmd, 1);
}

/**
 * @brief 等待驱动器反馈
 * @return  1=执行成功(0xF0), 0=失败(0xFE), -1=超时
 */
int xDrive_WaitFeedback(uint32_t drv_id, uint32_t timeout_ms)
{
    uint32_t tick = HAL_GetTick();
    uint32_t rx_id;
    uint8_t rx_data[8], rx_len;

    while (HAL_GetTick() - tick < timeout_ms) {
        if (CAN_Recv(&rx_id, rx_data, &rx_len)) {
            if (rx_len == 1) {
                if (rx_data[0] == FEEDBACK_EXECUTE) return 1;
                if (rx_data[0] == FEEDBACK_FAILURE) return 0;
            }
        }
    }
    return -1;
}
```

### 2.3 主函数使用示例

```c
/*
 * main.c - 使用示例
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>

/* 外部接口 */
extern void CAN_Driver_Init(void);
extern int xDrive_Enable(uint32_t drv_id);
extern int xDrive_Disable(uint32_t drv_id);
extern int xDrive_SetMode(uint32_t drv_id, uint8_t mode);
extern int xDrive_SetTarget(uint32_t drv_id, float value);
extern int xDrive_SetTarget2(uint32_t drv_id, float v1, float v2);
extern int xDrive_StreamGet(uint32_t drv_id, uint8_t stream_id, float *value);
extern int xDrive_SetZeroPos(uint32_t drv_id);
extern int xDrive_WaitFeedback(uint32_t drv_id, uint32_t timeout_ms);

/* 驱动器 CAN ID（需与驱动器参数 CAN_ID 一致） */
#define DRIVE_ID    0x01

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    CAN_Driver_Init();

    /* ---------- 示例1: 速度模式控制 ---------- */

    /* 切换到速度模式 */
    xDrive_SetMode(DRIVE_ID, MODE_SPEED);

    /* 使能电机 */
    xDrive_Enable(DRIVE_ID);
    HAL_Delay(100);

    /* 设置目标速度 1000 rpm */
    xDrive_SetTarget(DRIVE_ID, 1000.0f);

    /* 运行 5 秒 */
    HAL_Delay(5000);

    /* 读取当前速度 */
    float speed = 0;
    if (xDrive_StreamGet(DRIVE_ID, STREAM_SPEED, &speed) == 0) {
        printf("当前速度: %.1f rpm\r\n", speed);
    }

    /* 停止 */
    xDrive_SetTarget(DRIVE_ID, 0.0f);
    HAL_Delay(1000);
    xDrive_Disable(DRIVE_ID);


    /* ---------- 示例2: 位置模式控制 ---------- */

    /* 切换到位置模式 */
    xDrive_SetMode(DRIVE_ID, MODE_POSITION);

    /* 使能 */
    xDrive_Enable(DRIVE_ID);
    HAL_Delay(100);

    /* 设置目标位置 180 度 */
    xDrive_SetTarget(DRIVE_ID, 180.0f);

    /* 等待到位 */
    HAL_Delay(3000);

    /* 读取当前位置 */
    float pos = 0;
    if (xDrive_StreamGet(DRIVE_ID, STREAM_POSITION, &pos) == 0) {
        printf("当前位置: %.2f deg\r\n", pos);
    }

    xDrive_Disable(DRIVE_ID);


    /* ---------- 示例3: 电流模式控制 ---------- */

    xDrive_SetMode(DRIVE_ID, MODE_CURRENT);
    xDrive_Enable(DRIVE_ID);
    HAL_Delay(100);

    /* 设置 Iq = 0.5A, Id = 0A */
    xDrive_SetTarget2(DRIVE_ID, 0.5f, 0.0f);

    HAL_Delay(2000);
    xDrive_Disable(DRIVE_ID);


    /* ---------- 示例4: 查询多个数据 ---------- */

    float iq, id, spd, pos_fb;
    xDrive_StreamGet(DRIVE_ID, STREAM_IQ, &iq);
    xDrive_StreamGet(DRIVE_ID, STREAM_ID, &id);
    xDrive_StreamGet(DRIVE_ID, STREAM_SPEED, &spd);
    xDrive_StreamGet(DRIVE_ID, STREAM_POSITION, &pos_fb);
    printf("Iq=%.3fA Id=%.3fA Speed=%.1frpm Pos=%.2fdeg\r\n", iq, id, spd, pos_fb);

    while (1) {
        HAL_Delay(100);
    }
}
```

---

## 3. STM32H7 FDCAN 控制例程

### 3.1 FDCAN 初始化

```c
/*
 * fdcan_drv.c - STM32H7 FDCAN 驱动层
 * 使用 FDCAN1, PD0(RX)/PD1(TX), 经典CAN 500kbps
 */

#include "stm32h7xx_hal.h"
#include <string.h>

FDCAN_HandleTypeDef hfdcan1;

/* FDCAN 过滤器配置 */
static void FDCAN_Filter_Config(void)
{
    FDCAN_FilterTypeDef filter;
    filter.IdType = FDCAN_STANDARD_ID;
    filter.FilterIndex = 0;
    filter.FilterType = FDCAN_FILTER_MASK;
    filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filter.FilterID1 = 0x000;
    filter.FilterID2 = 0x000;   /* mask=0 → 接收所有 */
    HAL_FDCAN_ConfigFilter(&hfdcan1, &filter);

    /* 接收所有帧（拒绝列表为空） */
    HAL_FDCAN_ConfigGlobalFilter(&hfdcan1,
        FDCAN_ACCEPT_IN_RX_FIFO0,   /* 非匹配标准帧 */
        FDCAN_ACCEPT_IN_RX_FIFO0,   /* 非匹配扩展帧 */
        DISABLE, DISABLE);
}

/* FDCAN1 初始化: 经典CAN模式, 500kbps */
void FDCAN_Driver_Init(void)
{
    hfdcan1.Instance = FDCAN1;
    hfdcan1.Init.FrameFormat = FDCAN_FRAME_CLASSIC;   /* 经典CAN */
    hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
    hfdcan1.Init.AutoRetransmission = ENABLE;
    hfdcan1.Init.TransmitPause = DISABLE;
    hfdcan1.Init.ProtocolException = DISABLE;

    /* 时钟配置: FDCAN时钟 = 80MHz (来自 PLL2Q 或 HSE 分频)
     * 80MHz / 10 / (1+9+2) = 500kbps  (和驱动器端一致)
     * 注意: 实际分频值需根据你的 RCC 配置调整 */
    hfdcan1.Init.NominalPrescaler = 10;
    hfdcan1.Init.NominalSyncJumpWidth = 1;
    hfdcan1.Init.NominalTimeSeg1 = 9;
    hfdcan1.Init.NominalTimeSeg2 = 2;

    /* 数据段参数（经典模式下不关键，但需配置） */
    hfdcan1.Init.DataPrescaler = 10;
    hfdcan1.Init.DataSyncJumpWidth = 1;
    hfdcan1.Init.DataTimeSeg1 = 9;
    hfdcan1.Init.DataTimeSeg2 = 2;

    hfdcan1.Init.MessageRAMOffset = 0;
    hfdcan1.Init.StdFiltersNbr = 1;
    hfdcan1.Init.ExtFiltersNbr = 0;
    hfdcan1.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
    hfdcan1.Init.RxBuffersNbr = 0;
    hfdcan1.Init.RxFifo0ElmtsNbr = 16;
    hfdcan1.Init.RxFifo0ElmtSize = FDCAN_DATA_BYTES_8;
    hfdcan1.Init.RxFifo1ElmtsNbr = 0;
    hfdcan1.Init.TxBuffersNbr = 0;
    hfdcan1.Init.TxFifoQueueElmtsNbr = 4;
    hfdcan1.Init.TxElmtSize = FDCAN_DATA_BYTES_8;

    HAL_FDCAN_Init(&hfdcan1);

    FDCAN_Filter_Config();

    HAL_FDCAN_Start(&hfdcan1);
    HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
}

/* FDCAN 发送 */
int FDCAN_Send(uint32_t id, uint8_t *data, uint8_t len)
{
    FDCAN_TxHeaderTypeDef header;
    header.Identifier = id;
    header.IdType = FDCAN_STANDARD_ID;
    header.TxFrameType = FDCAN_DATA_FRAME;
    header.DataLength = len << 16;  /* FDCAN DLC 编码 */
    header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    header.BitRateSwitch = FDCAN_BRS_OFF;
    header.FDFormat = FDCAN_CLASSIC_CAN;
    header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    header.MessageMarker = 0;

    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &header, data) != HAL_OK)
        return -1;
    return 0;
}

/* FDCAN 接收缓冲区 */
static volatile uint8_t rx_data[8];
static volatile uint8_t rx_len = 0;
static volatile uint32_t rx_id = 0;
static volatile uint8_t rx_flag = 0;

/* FIFO0 接收回调 */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    FDCAN_RxHeaderTypeDef header;
    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &header, (uint8_t*)rx_data) == HAL_OK) {
        rx_id = header.Identifier;
        rx_len = (header.DataLength >> 16) & 0x0F;
        rx_flag = 1;
    }
}

/* 非阻塞接收 */
int FDCAN_Recv(uint32_t *id, uint8_t *data, uint8_t *len)
{
    if (!rx_flag) return 0;
    *id = rx_id;
    *len = rx_len;
    memcpy(data, (const void*)rx_data, rx_len);
    rx_flag = 0;
    return 1;
}
```

### 3.2 H7 平台适配

将控制层的外部接口替换为 FDCAN 版本：

```c
/* 在 x_drive.c 中，将 extern 声明替换为: */
extern int FDCAN_Send(uint32_t id, uint8_t *data, uint8_t len);
extern int FDCAN_Recv(uint32_t *id, uint8_t *data, uint8_t *len);

/* 或者统一适配层: */
int CAN_Send(uint32_t id, uint8_t *data, uint8_t len) {
    return FDCAN_Send(id, data, len);
}
int CAN_Recv(uint32_t *id, uint8_t *data, uint8_t *len) {
    return FDCAN_Recv(id, data, len);
}
```

主函数用法与 F4 完全一致，无需修改。

---

## 4. 多驱动器控制

```c
/* 控制多个驱动器 */
#define DRIVE1_ID   0x01
#define DRIVE2_ID   0x02

/* 同时使能 */
xDrive_Enable(DRIVE1_ID);
xDrive_Enable(DRIVE2_ID);

/* 分别设置不同速度 */
xDrive_SetMode(DRIVE1_ID, MODE_SPEED);
xDrive_SetMode(DRIVE2_ID, MODE_SPEED);
xDrive_SetTarget(DRIVE1_ID, 1000.0f);
xDrive_SetTarget(DRIVE2_ID, -1000.0f);

/* 读取各自速度 */
float spd1, spd2;
xDrive_StreamGet(DRIVE1_ID, STREAM_SPEED, &spd1);
xDrive_StreamGet(DRIVE2_ID, STREAM_SPEED, &spd2);
```

---

## 5. 注意事项

1. **波特率匹配**: 主控端 CAN 波特率必须与驱动器一致（默认 1Mbps）
2. **CAN ID**: 驱动器默认 CAN_ID 需通过 USB/UART 上位机预先配置，CAN 端口主要用于实时控制
3. **参数读写**: 建议通过 USB/UART 上位机进行参数配置，CAN 端口更适合实时目标值下发和数据监测
4. **F4 时钟**: 示例中 APB1=42MHz，如果你的时钟树不同，需调整分频值
5. **H7 FDCAN 时钟**: 需确保 FDCAN 外设时钟已通过 RCC 配置（通常来自 PLL2Q）
6. **中断优先级**: CAN 接收中断优先级不要高于 FOC 控制中断（TIM8），避免影响实时性
