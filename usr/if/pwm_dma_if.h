#ifndef __PWM_DMA_IF_H
#define __PWM_DMA_IF_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================
// pwm_dma_if.h — PWM+DMA 推流接口（usr/if 契约，异步例外）
//
// 用于 WS2812 这类"必须用 PWM 精确时序 + DMA 连续推送"的外设：
// 芯片驱动把颜色编码成 CCR 比较值序列，交给本接口异步推送。
//
// 实现者：board/<b>/hw（HAL 板 = TIM PWM + DMA + PulseFinished 中断，
//   中断只出现在 hw，并只服务本接口的 done 回调）
//
// 规则：
//   - 纯 C 类型参数；ctx 对调用方不透明
//   - start_dma 非阻塞；调用方先查 is_busy，忙则不重复启动
//   - 传输完成（每帧推完）后 is_busy 复位，并可经注册的回调通知
//   - get_code_cfg 返回该 PWM 分辨率下 0/1 逻辑电平对应的 CCR 值
// ============================================================

typedef struct
{
    void *ctx; // hw 侧 PWM/DMA 资源（对调用方不透明）

    // 启动一轮 CCR 值序列推流（异步）；返回 false 表示无法启动
    bool (*start_dma)(void *ctx, const uint32_t *ccr_buf, uint16_t len);

    // 停止推流（如异常恢复）
    void (*stop_dma)(void *ctx);

    // 上一轮是否仍在推流
    bool (*is_busy)(void *ctx);

    // 注册"一轮推完"回调（每实例单回调，可传 NULL 注销）
    void (*register_done)(void *ctx, void (*cb)(void *arg), void *arg);

    // 获取 0/1 码 CCR 值（逻辑电平对应的占空比比较值）
    bool (*get_code_cfg)(void *ctx, uint32_t *code_1, uint32_t *code_0);
} tPwmDmaIf;

#endif // __PWM_DMA_IF_H
