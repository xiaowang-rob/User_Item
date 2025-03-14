#pragma once

#include "stdbool.h"
#include "body.h"
#include "traj.h"

typedef struct Trot
{
    float x0;
    float y0;
    float L;
    float H;
    float AirTime;
    float LandTime;
    float phaseDiff;    
    float BezierLen;
    float AllPhaseTime;
    float bezierRotation;
    float Nonlinear;
    float Tn;
    float TnRecord;
    float RuntimeRecord;

    float BezierX2[2];
    float BezierY2[2];
    float BezierX12[12];
    float BezierY12[12];
}Trot;

void setBezierParam(float x0, float y0, float L, float H0, float phaseTime, float bezierRotation, float phaseDiff,
                    bool isWalk, char BezierLen, float Nonlinear, Trot *trot);
void Calulate(Point *PosNew, float Runtime, Trot *trot);
void updateBezierParam(Trot *trot);

extern Trot trot1;
extern Trot trot2;
extern Trot trot3;
extern Trot trot4;

extern Point PosNew1;
extern Point PosNew2;
extern Point PosNew3;
extern Point PosNew4;

