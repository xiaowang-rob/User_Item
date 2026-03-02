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
        LED_IDLE = 0,  /* 0-全灭：等待升级命令/空闲 */
        LED_ERASING,   /* 1-同步慢闪：正在擦除 Flash */
        LED_WRITING,   /* 2-交替快闪：正在接收/写入数据 */
        LED_VERIFYING, /* 3-同步快闪：正在校验数据 */
        LED_SUCCESS,   /* 4-全亮：升级成功，准备跳转 */
        LED_ERROR      /* 5-交替慢闪：升级失败/校验错误/跳转失败 */
    } LedState_t;

    /* ========== 函数声明 ========== */
    void LED_Init(void);
    void LED_SetState(LedState_t state);
    void LED_Process(void);

#ifdef __cplusplus
}
#endif

#endif /* __LED_INDICATOR_H */