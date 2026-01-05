#include "mode_manager.h"

void FOC_RUN(RUN_mode_e run_mode, LOOP_mode_e loop_mode)
{
    if (run_mode == ENCODER_CONTROL)
    {
    }
    switch (run_mode)
    {
    case ENCODER_CONTROL:
        switch (loop_mode)
        {
        case VOLTAGE_LOOP:

            break;
        case CURRENT_LOOP:

            break;
        case SPEED_LOOP:
            break;
        case POSITION_ABS_LOOP:

            break;
        case POSITION_REL_LOOP:
            break;
        default:
            break;
        }
        break;
    case SENSORLESS_CONTROL:
        switch (loop_mode)
        {
        case VOLTAGE_LOOP:

            break;
        case CURRENT_LOOP:

            break;
        case SPEED_LOOP:
            break;

        default:
            break;
        }
        break;
    default:
        break;
    }
}