#ifndef __DRIVE_STATE_H__
#define __DRIVE_STATE_H__

typedef enum
{
    OFFLINE,
    ONLINE,
    RUN_ERROR,
} Drive_state_e;

typedef struct
{
    Drive_state_e ENCODER_state;
    Drive_state_e FLASH_state;
} Drive_state_t;
Drive_state_t *drive_state_get_adr();
void ENCODER_state_set(Drive_state_e state);
void FLASH_state_set(Drive_state_e state);

Drive_state_e ENCODER_state_get(void);
Drive_state_e FLASH_state_get(void);

void drive_init();
#endif // __DRIVE_STATE_H__