#include "drive_state.h"
#include "flashDr.h"
#include "encoder.h"

Drive_state_t drive_state;

Drive_state_t *drive_state_get_adr()
{
    return &drive_state;
}
void ENCODER_state_set(Drive_state_e state)
{
    drive_state.ENCODER_state = state;
}
void FLASH_state_set(Drive_state_e state)
{
    drive_state.FLASH_state = state;
}

Drive_state_e ENCODER_state_get(void)
{
    return drive_state.ENCODER_state;
}
Drive_state_e FLASH_state_get(void)
{
    return drive_state.FLASH_state;
}

void drive_init()
{
    FLASH_Init();
    ENCODER_Init();
    ENCODER_Init();
    ADC_DR_Init(); // 开启FOC循环
}