#ifndef __LED_INDICATOR_H
#define __LED_INDICATOR_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "main.h"

/* ========== LED 配置 ========== */
#define LED1_PIN GPIO_PIN_2
#define LED1_PORT GPIOD
#define LED2_PIN GPIO_PIN_3
#define LED2_PORT GPIOB

    /* ========== LED 状态枚举 (6 种) ========== */
    typedef enum
    {
        LED_IDLE = 0, /* 交替慢闪：等待升级命令/空闲 */
        LED_WRITING,  /* 同步快闪：正在擦除 Flash */
        LED_SUCCESS,  /* 同步慢闪3s：升级成功，准备跳转 */
        LED_ERROR     /* 交替快闪：升级失败/校验错误/跳转失败 */
    } LedState_t;

    /* ========== 函数声明 ========== */
    void LED_Init(void);
    void LED_SetState(LedState_t state);
    void LED_Process(void);

#ifdef __cplusplus
}
#endif

#endif /* __LED_INDICATOR_H */