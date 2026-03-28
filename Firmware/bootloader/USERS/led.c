#include "led.h"

/* ========== 私有变量 ========== */
static LedState_t g_led_state = LED_IDLE;
static uint32_t g_led_timer = 0;
static uint8_t g_led_toggle = 1; /* LED 翻转状态 */

/* ========== 内部函数：根据状态更新 LED 物理输出 ========== */
static void _LED_Update(void)
{
    switch (g_led_state)
    {
    case LED_IDLE:
    case LED_ERROR:
        /* 两个灯状态相反，产生交替效果 */
        HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, g_led_toggle ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED2_PORT, LED2_PIN, g_led_toggle ? GPIO_PIN_RESET : GPIO_PIN_SET);
        break;

    case LED_WRITING:
    case LED_SUCCESS:
        /* 两个灯状态相同，产生同步效果 */
        HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, g_led_toggle ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED2_PORT, LED2_PIN, g_led_toggle ? GPIO_PIN_SET : GPIO_PIN_RESET);
        break;
    }
}

/* ========== 初始化 ========== */
void LED_Init(void)
{
    HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED2_PORT, LED2_PIN, GPIO_PIN_RESET);
    g_led_state = LED_IDLE;
    g_led_timer = HAL_GetTick();
    g_led_toggle = 1;
}

/* ========== 设置状态 ========== */
void LED_SetState(LedState_t state)
{
		if(state==g_led_state)
			return;
    g_led_state = state;
    g_led_timer = HAL_GetTick();
    g_led_toggle = 1;

    /* 立即更新显示 */
    _LED_Update();
}
volatile static uint16_t _tic = 0;
/* ========== 主循环中调用，非阻塞 ========== */
void LED_Process(void)
{
    uint32_t now = HAL_GetTick();
    uint32_t interval = 0;

    /* 根据状态确定闪烁间隔 */
    switch (g_led_state)
    {
    case LED_IDLE: /* 交替慢闪 - 500ms */
        interval = 500;
        break;

    case LED_SUCCESS: /* 同步慢闪 - 400ms */
        interval = 500;
        break;

    case LED_ERROR: /* 交替快闪 - 100ms */
        interval = 100;
        break;

    case LED_WRITING: /* 同步闪烁 */
				interval = 100;
				break;
    default:
        return;
    }

    /* 时间到则翻转 LED */
    if (now - g_led_timer >= interval)
    {
        g_led_toggle = !g_led_toggle;
        g_led_timer = now;
        _LED_Update();
    }
}