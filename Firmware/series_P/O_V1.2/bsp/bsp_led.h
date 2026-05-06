#ifndef __BSP_LED_H
#define __BSP_LED_H

#include "bsp.h"

void BSP_LED_CanTogglePin(void);
void BSP_LED_EncoderTogglePin(void);
void BSP_LED_CanSetPin(bool on);
void BSP_LED_EncoderSetPin(bool on);

#endif /* __BSP_LED_H */