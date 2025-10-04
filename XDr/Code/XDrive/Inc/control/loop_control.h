#ifndef __LOOP_CONTROL_H
#define __LOOP_CONTROL_H
#include "main.h"
#include "stdbool.h"

typedef struct
{
    u16 current_steps;
    u16 speed_steps;
    u16 position_steps;
    u16 updata_steps;
    bool current_updata;
    bool speed_updata;
    bool position_updata;
    float Tcur;
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
} LOOP_CON_t;
extern LOOP_CON_t g_loop_con;

void Frequency_division_updatta(f_Division_t *fd);
void Frequency_division_reset(f_Division_t *fd);
void LOOP_Parameter_writing(float kfd, float Udc, float max_current, float max_speed, float kp_id, float ki_id,
                            float kp_iq, float ki_iq, float kp_speed, float ki_speed, float kp_pos, float ki_pos, float kd_pos);
float Current_loop(float current_ref, float current_fb);
float Speed_loop(float omega_ref, float omega_fb);
float Position_loop(float position_ref, float position_fb);
float weak_mag_loop(float id_ref, float id_fb);
#endif