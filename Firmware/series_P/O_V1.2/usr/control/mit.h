#ifndef __MIT_H
#define __MIT_H

typedef struct
{
    // 可调参数
    float Kp;     // 刚度 (Nm/rad)
    float Kd;     // 阻尼 (Nm/(rad/s))
    float tau_ff; // 前馈扭矩 (Nm)
    float J;      // 转动惯量 (kg*m^2)
    float B;      // 摩擦系数 (Nms/rad)

    // 限幅
    float tau_max; // 最大扭矩 (Nm)

} tMIT_HandleTypeDef;

void mit_init(float Kp, float Kd, float tau_ff, float tau_max);
float mit_loop_update(float pos_ref, float pos_fb, float vel_ref, float vel_fb);
void mit_config_static(float Kp, float Kd);
void mit_config_tff(float tau_ff);
void mit_config_dynamic(float alpha_ref, float vel_ref);

#endif
