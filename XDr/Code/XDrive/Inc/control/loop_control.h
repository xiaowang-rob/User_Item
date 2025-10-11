#ifndef __LOOP_CONTROL_H
#define __LOOP_CONTROL_H
#include "main.h"
#include "stdbool.h"

typedef struct
{
    u16 current_steps;
    u16 speed_steps;
    u16 speed_updata_steps;
    u16 position_updata_steps;
    bool speed_updata;
    bool position_updata;
    float Tspd;
    float Tpos;
} f_Division_t;
typedef struct
{
    float kp;
    float ki;
    u8 enable_integral;
    float integral;
    float integral_limit;
    float output_limit;
    float output;
} PI_t;
typedef struct
{
    float kp;
    float ki;
    float kd;
    u8 enable_integral;
    float last_error;
    float integral;
    float integral_limit;
    float derivative;
    float last_derivative;
    float derivative_limit;
    float derivative_filter;
    float output_limit;
    float output;
} PID_t;
typedef struct
{
    f_Division_t fd;
    PI_t PI_iq;
    PI_t PI_id;
    PI_t PI_speed;
    PID_t PID_pos;
    float position_min;
    float position_max;
} LOOP_CON_t;

void Frequency_division_updatta();
void Frequency_division_reset();
void loop_reset(void);
void LOOP_Parameter_writing(float fspeed, float fpos, float Udc, float max_current, float max_speed, float position_min, float position_max, float kp_id, float ki_id,
                            float kp_iq, float ki_iq, float kp_speed, float ki_speed, float kp_pos, float ki_pos, float kd_pos);
float Current_loop(float current_ref, float current_fb);
float Magnetic_loop(float id_ref, float id_fb);
float Speed_loop(float omega_ref, float omega_fb);
float Position_abs_loop(float position_ref, float position_fb);
float Position_rel_loop(float position_ref, float position_fb);
bool speed_loop_updata();
bool position_loop_updata();

#endif