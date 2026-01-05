#ifndef __LOOP_CONTROL_H
#define __LOOP_CONTROL_H
#include "main.h"
#include "stdbool.h"

typedef struct
{
    float fspeed_loop;
    float fposition_loop;
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
    PI_t PI_weakmag;
    PI_t PI_speed;
    PID_t PID_pos;
    float position_min;
    float position_max;
} LOOP_CON_t;
extern LOOP_CON_t g_loop_con;

void Frequency_division_init(float fspeed, float fpos);
void Frequency_division_update();
void Frequency_division_reset();
void loop_reset(void);
void PI_init(PI_t *pi, float kp, float ki, float output_limit);
void PID_init(PID_t *pid, float kp, float ki, float kd, float output_limit);
void loop_reset(void);
float Current_loop(float current_ref, float current_fb);
float Magnetic_loop(float id_ref, float id_fb);
float WeakMag_loop(float ud, float uq, float max_Vs);
float Speed_loop(float omega_ref, float omega_fb);
float Position_abs_loop(float position_ref, float position_fb);
float Position_rel_loop(float position_ref, float position_fb);
bool speed_loop_updata();
bool position_loop_updata();

#endif