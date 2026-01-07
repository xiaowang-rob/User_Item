#ifndef __AUTO_CALIBRATION_H
#define __AUTO_CALIBRATION_H

#include "main.h"
#include "smo.h"
#define OMEGA_TUNE_POLE_PAIRS 200 // 约 7对 272rpm
#define Umax_TUNE_LS 3

void auto_calibration_init(float initial_Rs, float initial_Ls,
                           float initial_Psi_f, float initial_pole_pairs, TUNE_MODE_E tune_mode);
bool auto_calibration_update();
bool get_motor_fault_flag();
#endif