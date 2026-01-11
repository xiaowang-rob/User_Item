#ifndef __LOOP_CONTROL_H
#define __LOOP_CONTROL_H

#include "main.h"
#include "stdbool.h"
#include "parameter_manager.h"
typedef struct
{
    u8 tic;
    u8 current_steps;
    u8 speed_steps;
    u8 current_updata_steps;
    u8 speed_updata_steps;
    u8 position_updata_steps;
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
    PI_t PI_weakmag;
    PI_t PI_speed;
    PID_t PID_pos;
    float max_Vs;
    float position_min;
    float position_max;
} LOOP_CON_t;
extern LOOP_CON_t g_loop_con;

void Frequency_division_update();
void loop_parameter_init(Parameter_t param, float Vmax);
void loop_reset();
float Current_loop(float current_ref, float current_fb);
float Magnetic_loop(float id_ref, float id_fb);
float WeakMag_loop(float ud, float uq);
float Speed_loop(float omega_ref, float omega_fb);
float Position_abs_loop(float position_ref, float position_fb);
float Position_rel_loop(float position_ref, float position_fb);
void POS_LOOP_set_omega(float omega);

LOOP_CON_t *get_loop_con_adr();

#endif