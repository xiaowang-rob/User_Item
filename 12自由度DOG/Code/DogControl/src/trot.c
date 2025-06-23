#include "trot.h"

Trot trot1;
Trot trot2;
Trot trot3;
Trot trot4;

Point PosNew1;
Point PosNew2;
Point PosNew3;
Point PosNew4;

void setBezierParam(float x0, float y0, float L, float H0, float phaseTime, float bezierRotation, float phaseDiff,
                    bool isWalk, char BezierLen, float Nonlinear, Trot *trot)
{
    trot->x0 = x0, trot->y0 = y0, trot->L = L, trot->H = H0;
    trot->AirTime = phaseTime, trot->LandTime = isWalk ? 3 * phaseTime : (phaseTime);
    trot->phaseDiff = phaseDiff, trot->BezierLen = BezierLen;
    trot->bezierRotation = bezierRotation * 0.0174532925f;
    trot->AllPhaseTime = trot->AirTime + trot->LandTime, trot->TnRecord = phaseDiff * trot->AllPhaseTime;
    trot->Nonlinear = Nonlinear;
    updateBezierParam(trot);
}

// Trot步态单腿运算
void Calulate(Point *PosNew, float Runtime, Trot *trot)
{
    trot->Tn = trot->TnRecord + Runtime - trot->RuntimeRecord; // 更新Tn

    while (trot->Tn < 0)
        trot->Tn += trot->AllPhaseTime;
    while (trot->Tn > trot->AllPhaseTime)
        trot->Tn -= trot->AllPhaseTime;

    if (trot->Tn < trot->AirTime) // 目前处于腾空相
    {
        Bezier(&PosNew->x, &PosNew->y, trot->BezierX12, trot->BezierY12, 12, trot->Tn / trot->AirTime,
               trot->Nonlinear); // 腾空相在前
    }
    else // 目前处于着地相
    {
        Bezier(&PosNew->x, &PosNew->y, trot->BezierX2, trot->BezierY2, 2, (trot->Tn - trot->AirTime) / trot->LandTime,
               1);
    }
    if (trot->bezierRotation) // 旋转贝塞尔
        Rotate(&PosNew->x, &PosNew->y, 0, 0, trot->bezierRotation);
}

/**
 * @brief bizier参数更新
 *
 * @param trot
 */
void updateBezierParam(Trot *trot)
{

    trot->BezierX2[0] = -trot->L / 2;
    trot->BezierX2[1] = trot->L / 2;
    trot->BezierY2[0] = 0;
    trot->BezierY2[1] = 0;
    for (int i = 0; i < 2; i++)
        trot->BezierX2[i] += trot->x0, trot->BezierY2[i] += trot->y0;

    float dL = trot->L / 11;
    trot->BezierX12[0] = trot->L / 2;
    trot->BezierX12[1] = trot->L / 2 + dL;
    trot->BezierX12[2] = trot->L / 2 + 2 * dL;
    trot->BezierX12[3] = trot->L / 2 + 3 * dL;
    trot->BezierX12[4] = trot->L / 2 + 2 * dL;
    trot->BezierX12[5] = trot->L / 2;
    trot->BezierX12[6] = -trot->L / 2;
    trot->BezierX12[7] = -trot->L / 2 - 2 * dL;
    trot->BezierX12[8] = -trot->L / 2 - 3 * dL;
    trot->BezierX12[9] = -trot->L / 2 - 2 * dL;
    trot->BezierX12[10] = -trot->L / 2 - dL;
    trot->BezierX12[11] = -trot->L / 2;
    trot->BezierY12[0] = 0;
    trot->BezierY12[1] = 0;
    trot->BezierY12[2] = -trot->H / 3;
    trot->BezierY12[3] = -trot->H;
    trot->BezierY12[4] = -trot->H;
    trot->BezierY12[5] = -trot->H * 6 / 5;
    trot->BezierY12[6] = -trot->H * 6 / 5;
    trot->BezierY12[7] = -trot->H;
    trot->BezierY12[8] = -trot->H;
    trot->BezierY12[9] = -trot->H / 3;
    trot->BezierY12[10] = 0;
    trot->BezierY12[11] = 0;
    for (int i = 0; i < 12; i++)
        trot->BezierX12[i] += trot->x0, trot->BezierY12[i] += trot->y0;
}