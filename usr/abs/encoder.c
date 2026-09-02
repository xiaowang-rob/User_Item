#include "encoder.h"
#include "math_fast.h"

#define ENCODER_PLL_KP 80.0f
#define ENCODER_PLL_KI 2000.0f

// enc 编码器句柄 ops 驱动操作函数表 handle 驱动句柄 type 编码器类型(内还是外)
bool encoder_init(tEncoder *enc,
                  const tEncoderDriverOps *ops,
                  EncoderChipHandle handle,
                  eEncoderType type)
{
    if (!enc || !ops || !handle)
        return false;
    memset(enc, 0, sizeof(tEncoder));
    enc->drv_ops = ops;
    enc->type = type;
    enc->drv_handle = handle;

    if (!ops->init(handle, type))
        return false;

    if (!ops->get_resolution(handle, &enc->resolution))
        return false;
    enc->rad_per_lsb = MATH_2PI / (float)enc->resolution;

    enc->first_run = true;
    enc->pll_kp = ENCODER_PLL_KP;
    enc->pll_ki = ENCODER_PLL_KI;
    enc->pll_integ = 0;
    return true;
}

void encoder_task(tEncoder *enc)
{
    if (!enc || !enc->drv_ops)
        return;
    if (!enc->drv_ops->is_data_ready(enc->drv_handle))
        return;

    uint16_t raw;
    uint32_t ts;
    if (!enc->drv_ops->get_raw_data(enc->drv_handle, &raw, &ts))
    {
        enc->valid_counter = (enc->valid_counter < 110) ? enc->valid_counter + 10 : 110;
        if (enc->valid_counter > 100)
            enc->data_valid = false;
        return;
    }

    enc->valid_counter = (enc->valid_counter > 0) ? enc->valid_counter - 1 : 0;
    enc->data_valid = true;
    float angle_abs = raw * enc->rad_per_lsb;

    if (enc->first_run)
    {
        enc->last_raw_angle = raw;
        enc->pos_offset = raw;
        enc->num_turns = 0;
        enc->pos = 0.0f;
        enc->last_angle_abs = angle_abs;
        enc->last_timestamp_ms = ts;
        enc->pll_theta = angle_abs;
        enc->first_run = false;
        enc->angle_abs = angle_abs;
        return;
    }

    const uint16_t half_res = enc->resolution / 2;
    // 多圈累计
    int16_t delta_raw = (int16_t)(raw - enc->last_raw_angle);
    if (delta_raw < -half_res)
        enc->num_turns++;
    else if (delta_raw > half_res)
        enc->num_turns--;

    enc->pos = (int32_t)(raw - enc->pos_offset) * enc->rad_per_lsb + enc->num_turns * MATH_2PI;

    // M/T 测速度 - 一般不用这个速度 没有pll速度平滑
    float dt = (ts - enc->last_timestamp_ms) / 1e3f;
    if (dt > 0.001f)
    {
        float delta_angle = normalize_angle_pi(angle_abs - enc->last_angle_abs);
        enc->vel = delta_angle / dt;
        if (fabsf(enc->vel) > 10472.0f)
            enc->vel = 0.0f;
        enc->last_timestamp_ms = ts;
        enc->last_angle_abs = angle_abs;
    }

    enc->last_raw_angle = raw;
    enc->angle_abs = angle_abs;
}

// pll 跟踪速度
void encoder_pll_update(tEncoder *enc, float dt)
{
    if (!enc || dt <= 0)
        return;
    enc->pll_theta_delta = normalize_angle_pi(enc->angle_abs - enc->pll_theta);
    enc->pll_integ += enc->pll_theta_delta * dt;
    if (enc->pll_integ > 0.1745f)
        enc->pll_integ = 0.1745f;
    if (enc->pll_integ < -0.1745f)
        enc->pll_integ = -0.1745f;

    float estimated_speed = enc->pll_kp * enc->pll_theta_delta + enc->pll_ki * enc->pll_integ;
    enc->pll_theta += estimated_speed * dt;
    enc->pll_theta = normalize_angle_360(enc->pll_theta);
    enc->pll_vel = (fabsf(estimated_speed) < 0.05f) ? 0.0f : estimated_speed;

    if (enc->pll_vel > 10472.0f || enc->pll_vel < -10472.0f)
    {
        enc->pll_integ = 0.0f;
        enc->pll_vel = 0.0f;
        enc->pll_theta = enc->angle_abs;
    }
}

void encoder_set_zero(tEncoder *enc)
{
    if (!enc)
        return;
    enc->pos_offset = enc->last_raw_angle;
    enc->num_turns = 0;
    enc->pos = 0.0f;
    enc->pll_theta = enc->angle_abs;
    enc->pll_integ = 0.0f;
}

uint8_t encoder_get_Dstate(tEncoder *enc)
{
    if (!enc || !enc->drv_ops)
        return 0;
    return enc->drv_ops->get_Dstate(enc->drv_handle);
}