#include "body.h"

Body body;
float Runtime;

/**
 * @brief 腿部杆长初始化
 *
 * @param body
 */
void BodyInit(Body *body)
{
    for (int i = 0; i < 4; i++)
    {
        body->leg[i].L1 = 40;  // 大腿
        body->leg[i].L2 = 80;  // 小腿
        body->leg[i].L3 = 80;  // 大腿
        body->leg[i].L4 = 40;  // 小腿
        body->leg[i].L5 = 30;  // 舵机间距
        body->leg[i].L6 = 80;  // 带脚小腿长度
        body->leg[i].c1 = 180; // 连舵机大腿杆与X轴正方向的夹角
        body->leg[i].c4 = 0;
    }
    body->c10 = 0;
    body->c40 = 0;
}

/**
 * @brief 逆运动学解算
 *
 * @param point 足端到达的点
 * @param leg 哪条腿
 */
void InverseKinematics(Point *point, Leg *leg)
{
    float x = point->x;
    float y = point->y;
    float A = 2 * leg->L1 * y;
    float B = 2 * leg->L1 * x;
    float C = leg->L6 * leg->L6 - leg->L1 * leg->L1 - x * x - y * y;
    leg->c1 = 2 * atan((A + sqrt(A * A + B * B - C * C)) / (B - C));
    if (leg->c1 < 0)
        leg->c1 += 2 * PI;
    float xb = leg->L1 * arm_cos_f32(leg->c1);
    float yb = leg->L1 * arm_sin_f32(leg->c1);
    float alp = 0;
    float xc = leg->L2 / leg->L6 * ((x - xb) * arm_cos_f32(alp) - (y - yb) * arm_sin_f32(alp)) + xb;
    float yc = leg->L2 / leg->L6 * ((x - xb) * arm_sin_f32(alp) + (y - yb) * arm_cos_f32(alp)) + yb;
    A = 2 * leg->L4 * yc;
    B = 2 * leg->L4 * (xc - leg->L5);
    C = leg->L3 * leg->L3 + 2 * leg->L5 * xc - leg->L4 * leg->L4 - leg->L5 * leg->L5 - xc * xc - yc * yc;
    leg->c4 = 2 * atan((A - sqrt(A * A + B * B - C * C)) / (B - C));
    leg->c1 = leg->c1 * 57.2957795f;
    leg->c4 = leg->c4 * 57.2957795f;
}

void MoveToPosCalculate(Point *new_point, Point *start_point, Point *target_point, float Runtime, float t_MoveToPos,
                        Leg *leg)
{
    new_point->x = Lerp(start_point->x, target_point->y, Runtime / t_MoveToPos);
    new_point->y = Lerp(target_point->x, target_point->y, Runtime / t_MoveToPos);
    InverseKinematics(new_point, leg);
}

/**
 * @brief 动腿，控制舵机转动，使用PWM控制
 *
 * @param body
 */
void LegControl(Body body)
{

    // 左前 0
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 50 + (body.leg[0].c4 + 90) / 180 * 200);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 50 + (body.leg[0].c1 - 90) / 180 * 200);

    // 右前 1
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 50 + (90 - body.leg[1].c4) / 180 * 200);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, 50 + (270 - body.leg[1].c1) / 180 * 200);

    // 左后 2
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 50 + (body.leg[2].c4 + 90) / 180 * 200);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 50 + (body.leg[2].c1 - 90) / 180 * 200);

    // 右后 3
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 50 + (90 - body.leg[3].c4) / 180 * 200);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 50 + (270 - body.leg[3].c1) / 180 * 200);
}