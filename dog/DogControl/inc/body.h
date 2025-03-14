#pragma once

#include "arm_math.h"
#include "traj.h"
#include "leg.h"

typedef struct Leg {
    float L1;
    float L2;
    float L3;
    float L4;
    float L5;
    float L6;
    float c1;
    float c4;
} Leg;

typedef struct Body {
    Leg leg[4];
    float c10; // c1零位置
    float c40; // c4零位置
} Body;

void BodyInit();
void InverseKinematics(Point *point, Leg *leg);
void LegControl(Body body);

extern float Runtime;
extern Body body;

