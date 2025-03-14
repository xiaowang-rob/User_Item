#pragma once

#include "arm_math.h"

typedef struct
{
    float x;
    float y;
}Point;

float decas(int degree, float coeff[], float t);
void Rotate(float *x, float *y, float x0, float y0, float a);
float Lerp(float start, float end, float t);
void Bezier(float *x, float *y, float mx[], float my[], int n, float t, float Nonlinear);
void Cycloid(float *x, float *y, float H, float L, float x0, float y0, float t, float Nonlinear);