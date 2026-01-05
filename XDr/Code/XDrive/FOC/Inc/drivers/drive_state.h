#ifndef __DRIVE_STATE_H__
#define __DRIVE_STATE_H__

typedef enum
{
    OFFLINE,
    ONLINE,
    INIT_ERROR,
    RUN_ERROR,
    SINGNAL_ERROR,
} Drive_state_e;

typedef struct
{
    Drive_state_e ENCODER_state;
    Drive_state_e FLASH_state;
} Drive_state_t;

void ENCODER_state_set(Drive_state_e state);
void FLASH_state_set(Drive_state_e state);

Drive_state_e ENCODER_state_get(void);
Drive_state_e FLASH_state_get(void);

#endif // __DRIVE_STATE_H__