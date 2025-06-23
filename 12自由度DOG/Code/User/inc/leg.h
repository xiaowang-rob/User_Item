#include "main.h"
#include <stdbool.h>

bool PWM_turn_init();
bool HEAD_turn_abgle(float angle, int speed);
bool PWM_turn_angle(int leg_num, float angle_front, float angle_back, int speed);
bool PWM_turn_back(int leg_num, float angle_back_1, float angle_back_2, int speed);
bool LEG_grovel(int leg_num);
bool LEG_stand(int leg_num);
bool LEG_move_up(int leg_num);
bool LEG_move_down();
