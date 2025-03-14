#include "led.h"
#include <stdbool.h>
#include "main.h"
/*
狗腿时钟控制分布
   *********
   *time1_4*
   *********
time2_1  time2_3
time2_2  time2_4

time3_1  time3_3
time3_2  time3_4
*/

#define angle_grovel_1 -5
#define angle_grovel_2 5

#define angle_stand_1 25
#define angle_stand_2 2

#define angle_walk_1 23
#define angle_walk_2 12


float angle_head = 0;
float angle_front_left_1 = 0;
float angle_front_left_2 = 0;
float angle_front_right_1 = 0;
float angle_front_right_2 = 0;
float angle_back_left_1 = 0;
float angle_back_left_2 = 0;
float angle_back_right_1 = 0;
float angle_back_right_2 = 0;

bool PWM_turn_init()
{

//  PWM_turn(htim1, 0, 0);

  PWM_turn(htim2, 1, 0);
  PWM_turn(htim2, 2, 0);
  PWM_turn(htim2, 3, 0);
  PWM_turn(htim2, 4, 0);

  PWM_turn(htim3, 5, 0);
  PWM_turn(htim3, 6, 0);
  PWM_turn(htim3, 7, 0);
  PWM_turn(htim3, 8, 0);
}
/*
脑袋转动
方向：逆时针为正，顺时针为负
speed：速度，范围0-10，0为最慢，10为最快
*/
bool HEAD_turn_abgle(float angle, int speed)
{
  int time = 10 - speed;
  if (speed < 0)
    time = 12;
  if (speed > 10)
    time = 0;

  float angle_turn = angle / 100;
  float angle0 = angle_head;

  for (int t = 0; t < 100; t++)
  {

    angle_head += angle_turn;
    PWM_turn(htim1, 0, angle_head);
    osDelay(time);
  }
  angle_turn = angle0 + angle;
  PWM_turn(htim1, 4, angle_head);
}
/*
舵机的角度速度模拟旋转一定角度
leg_num:左前1 右前2 左后3 右后4
方向：逆时针为正，顺时针为负
speed：速度，范围0-10，0为最慢，10为最快
*/

bool PWM_turn_angle(int leg_num, float angle_front, float angle_back, int speed)
{
  int time = 10 - speed;
  if (speed < 0)
    time = 12;
  if (speed > 10)
    time = 0;

  float angle_turn_1 = angle_front / 100;
  float angle_turn_2 = angle_back / 100;

  switch (leg_num)
  {
  case 1:
  {
    float angle0_1 = angle_front_left_1;
    float angle0_2 = angle_front_left_2;

    for (int t = 0; t < 100; t++)
    {

      angle_front_left_1 += angle_turn_1;
      angle_front_left_2 += angle_turn_2;
      PWM_turn(htim2, 1, angle_front_left_1);
      PWM_turn(htim2, 2, angle_front_left_2);
      osDelay(time);
    }
    angle_front_left_1 = angle0_1 + angle_front;
    angle_front_left_2 = angle0_2 + angle_back;
    PWM_turn(htim2, 1, angle_front_left_1);
    PWM_turn(htim2, 2, angle_front_left_2);
    break;
  }
  case 2:
  {
    float angle0_1 = angle_front_right_1;
    float angle0_2 = angle_front_right_2;

    for (int t = 0; t < 100; t++)
    {

      angle_front_right_1 -= angle_turn_1;
      angle_front_right_2 -= angle_turn_2;
      PWM_turn(htim2, 3, angle_front_right_1);
      PWM_turn(htim2, 4, angle_front_right_2);
      osDelay(time);
    }
    angle_front_right_1 = angle0_1 - angle_front;
    angle_front_right_2 = angle0_2 - angle_back;
    PWM_turn(htim2, 3, angle_front_right_1);
    PWM_turn(htim2, 4, angle_front_right_2);
    break;
  }
  case 3:
  {
    float angle0_1 = angle_back_left_1;
    float angle0_2 = angle_back_left_2;

    for (int t = 0; t < 100; t++)
    {

      angle_back_left_1 += angle_turn_1;
      angle_back_left_2 += angle_turn_2;
      PWM_turn(htim3, 5, angle_back_left_1);
      PWM_turn(htim3, 6, angle_back_left_2);
      osDelay(time);
    }
    angle_back_left_1 = angle0_1 + angle_front;
    angle_back_left_2 = angle0_2 + angle_back;
    PWM_turn(htim3, 5, angle_back_left_1);
    PWM_turn(htim3, 6, angle_back_left_2);
    break;
  }
  case 4:
  {
    float angle0_1 = angle_back_right_1;
    float angle0_2 = angle_back_right_2;

    for (int t = 0; t < 100; t++)
    {

      angle_back_right_1 -= angle_turn_1;
      angle_back_right_2 -= angle_turn_2;
      PWM_turn(htim3, 7, angle_back_right_1);
      PWM_turn(htim3, 8, angle_back_right_2);
      osDelay(time);
    }
    angle_back_right_1 = angle0_1 - angle_front;
    angle_back_right_2 = angle0_2 - angle_back;
    PWM_turn(htim3, 7, angle_back_right_1);
    PWM_turn(htim3, 8, angle_back_right_2);
    break;
  }
  default:
    return false;
  }
}
/*
舵机的角度速度模拟回转到一定角度
leg_num:左前1 右前2 左后3 右后4
方向：逆时针为正，顺时针为负
speed：速度，范围0-10，0为最慢，10为最快
*/
bool PWM_turn_back(int leg_num, float angle_back_1, float angle_back_2, int speed)
{

  int time = 10 - speed;
  if (speed < 0)
    time = 12;
  if (speed > 10)
    time = 0;

  switch (leg_num)
  {
  case 1:
  {
    float angle_1 = (angle_back_1 - angle_front_left_1) / 100;
    float angle_2 = (angle_back_2 - angle_front_left_2) / 100;
    for (int t = 0; t < 100; t++)
    {

      angle_front_left_1 += angle_1;
      angle_front_left_2 += angle_2;
      PWM_turn(htim2, 1, angle_front_left_1);
      PWM_turn(htim2, 2, angle_front_left_2);
      osDelay(time);
    }
    angle_front_left_1 = angle_back_1;
    angle_front_left_2 = angle_back_2;
    PWM_turn(htim2, 1, angle_front_left_1);
    PWM_turn(htim2, 2, angle_front_left_2);
    break;
  }
  case 2:
  {
    float angle_1 = (angle_back_1 - angle_front_right_1) / 100;
    float angle_2 = (angle_back_2 - angle_front_right_2) / 100;
    for (int t = 0; t < 100; t++)
    {

      angle_front_right_1 -= angle_1;
      angle_front_right_2 -= angle_2;
      PWM_turn(htim2, 3, angle_front_right_1);
      PWM_turn(htim2, 4, angle_front_right_2);
      osDelay(time);
    }
    angle_front_right_1 = -angle_back_1;
    angle_front_right_2 = -angle_back_2;
    PWM_turn(htim2, 3, angle_front_right_1);
    PWM_turn(htim2, 4, angle_front_right_2);
    break;
  }
  case 3:
  {
    float angle_1 = (angle_back_1 - angle_back_left_1) / 100;
    float angle_2 = (angle_back_2 - angle_back_left_2) / 100;
    for (int t = 0; t < 100; t++)
    {

      angle_back_left_1 += angle_1;
      angle_back_left_2 += angle_2;
      PWM_turn(htim3, 5, angle_back_left_1);
      PWM_turn(htim3, 6, angle_back_left_2);
      osDelay(time);
    }
    angle_back_left_1 = angle_back_1;
    angle_back_left_2 = angle_back_2;
    PWM_turn(htim3, 5, angle_back_left_1);
    PWM_turn(htim3, 6, angle_back_left_2);
    break;
  }
  case 4:
  {
    float angle_1 = (angle_back_1 - angle_back_right_1) / 100;
    float angle_2 = (angle_back_2 - angle_back_right_2) / 100;
    for (int t = 0; t < 100; t++)
    {

      angle_back_right_1 -= angle_1;
      angle_back_right_2 -= angle_2;
      PWM_turn(htim3, 7, angle_back_right_1);
      PWM_turn(htim3, 8, angle_back_right_2);
      osDelay(time);
    }
    angle_back_right_1 = -angle_back_1;
    angle_back_right_2 = -angle_back_2;
    PWM_turn(htim3, 7, angle_back_right_1);
    PWM_turn(htim3, 8, angle_back_right_2);
    break;
  }
  default:
    return false;
  }
}
/*
卧倒姿势
速度：7
*/
bool LEG_grovel(int leg_num)
{
  PWM_turn_back(leg_num, angle_grovel_1, angle_grovel_2, 7);
}
/*
站姿
速度：7
*/
bool LEG_stand(int leg_num)
{
  PWM_turn_back(leg_num, angle_grovel_1, angle_stand_2, 7);
}

bool LEG_move_up(int leg_num)
{

  PWM_turn_back(leg_num, angle_walk_1, angle_walk_2, 3);

  PWM_turn_angle(leg_num, -23, -2, 11);
  PWM_turn_angle(leg_num, -44, -32, 8);
  PWM_turn_angle(leg_num, 20, 0, 9);
  PWM_turn_angle(leg_num, 45, 30, 5);
  PWM_turn_back(leg_num, angle_walk_1, angle_walk_2, 6);
}
bool LEG_move_down();
