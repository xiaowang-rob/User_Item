#include "traj.h"

float decas(int degree, float coeff[], float t) {
    int r, i;
    float point[13];
    for (i = 0; i < degree; i++)
        point[i] = coeff[i];
    for (r = 1; r < degree; r++)
        for (i = 0; i < degree - r; i++)
            point[i] = (1 - t) * point[i] + t * point[i + 1];

    return (point[0]);
}

void Rotate(float *x, float *y, float x0, float y0, float a) {
    float x1 = *x;
    float y1 = *y;
    *x = (x1 - x0) * cos(a) - (y1 - y0) * sin(a) + x0;
    *y = (y1 - y0) * cos(a) + (x1 - x0) * sin(a) + y0;
}

// 线性插值
float Lerp(float start, float end, float t) {
    if (t > 1)
        t = 1;
    return (end - start) * t + start;
}

// 贝塞尔
void Bezier(float *x, float *y, float mx[], float my[], int n, float t, float Nonlinear) {
    if (t >= 1)
        t = 1;
    else if (t <= 0)
        t = 0;

    if (Nonlinear != 1.0f)
        t = pow(t, Nonlinear);

    *x = decas(n, mx, t);
    *y = decas(n, my, t);
}

// 摆线
void Cycloid(float *x, float *y, float H, float L, float x0, float y0, float t, float Nonlinear) {
    if (t >= 1)
        t = 1;
    else if (t <= 0)
        t = 0;

    if (Nonlinear != 1.0f)
        t = pow(t, Nonlinear);

    *x = -(L * (t - 1.0f / (2.0f * PI) * sinf(2.0f * PI * t)) - L / 2.0f) + x0;
    *y = -H * (1.0f / 2.0f - 1.0f / 2.0f * cos(2.0f * PI * t)) + y0;
}
