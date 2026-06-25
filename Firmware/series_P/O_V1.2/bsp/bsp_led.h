#ifndef __BSP_LED_H
#define __BSP_LED_H

#include "bsp.h"

void bsp_led_can_toggle_pin(void);
void bsp_led_encoder_toggle_pin(void);
void bsp_led_can_set_pin(bool on);
void bsp_led_encoder_set_pin(bool on);

#endif // __BSP_LED_H