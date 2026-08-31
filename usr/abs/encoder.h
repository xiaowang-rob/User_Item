#ifndef __HAL_ENCODER_H
#define __HAL_ENCODER_H

#include <stdint.h>
#include <stdbool.h>

// 编码器位置
typedef enum
{
    ENC_INTERNAL, // 内部编码器
    ENC_EXTERNAL  // 外部编码器
} eEncoderType;

// 定义一个空的指针类型，用于声明驱动的句柄
typedef void *EncoderChipHandle;

// 驱动操作函数表（每个实例一份）
typedef struct
{
    bool (*init)(EncoderChipHandle handle, eEncoderType type);
    bool (*get_resolution)(EncoderChipHandle handle, uint16_t *res);                    // 驱动初始化
    bool (*start_read)(EncoderChipHandle handle);                                       // 启动一次异步读取（非阻塞）
    bool (*is_data_ready)(EncoderChipHandle handle);                                    // 检查是否有新数据
    bool (*get_raw_data)(EncoderChipHandle handle, uint16_t *raw, uint32_t *timestamp); // 获取原始数据（拷贝）
    void (*reset)(EncoderChipHandle handle);                                            // 复位硬件计数器
    void (*set_cs)(EncoderChipHandle handle, bool active);                              // 片选控制（若驱动内部管理可免）
} tEncoderDriverOps;

typedef struct
{
    const tEncoderDriverOps *drv_ops; // 绑定的驱动的函数表
    EncoderChipHandle drv_handle;     // 驱动句柄

    eEncoderType type;   // 编码器类型
    uint16_t resolution; // 单圈分辨率，如 16384
    float rad_per_lsb;   // 每 LSB 弧度

    // 业务状态
    float angle_abs;   // 绝对角度
    float pos;         // 位置
    float vel;         // 速度
    int32_t num_turns; // 转数

    // 内部变量
    uint16_t last_raw_angle;    // 上一次原始角度
    float pos_offset;           // 位置偏移量（用于归零）
    uint32_t last_timestamp_ms; // 上一次时间戳
    float last_angle_abs;       // 上一次绝对角度

    // PLL
    float pll_theta;       // PLL 输出角度
    float pll_vel;         // PLL 输出速度
    float pll_kp;          // PLL 比例增益
    float pll_ki;          // PLL 积分增益
    float pll_integ;       // PLL 积分累加
    float pll_theta_delta; // PLL 误差

    float com_error_rate;  // 通信错误率（指数移动平均）
    uint8_t valid_counter; // 有效数据计数
    bool first_run;        // 首次运行标志
    bool data_valid;       // 数据有效性标志
} tEncoder;

bool encoder_core_init(tEncoder *enc,
                       const tEncoderDriverOps *ops,
                       EncoderChipHandle handle,
                       eEncoderType type);

void encoder_update(tEncoder *enc);
void encoder_pll_update(tEncoder *enc, float dt);
void encoder_set_zero(tEncoder *enc);

static inline float encoder_get_angle_abs(tEncoder *enc) { return enc->angle_abs; }
static inline float encoder_get_position(tEncoder *enc) { return enc->pos; }
static inline float encoder_get_velocity(tEncoder *enc) { return enc->pll_vel; }
static inline int32_t encoder_get_turns(tEncoder *enc) { return enc->num_turns; }

#endif // __HAL_ENCODER_H