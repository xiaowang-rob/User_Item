#include "adaptive_control.h"
#include "adcDr.h"
ADAPTIVE_CON_T g_adaptive_con = {0};

void adaptive_control_init()
{
    ADC_GET_Temp(&g_adaptive_con.temp_u, &g_adaptive_con.temp_v, &g_adaptive_con.temp_w, &g_adaptive_con.tempareture);
    ADC_GET_Voltage(&g_adaptive_con.Udc);
}
void adaptive_control_update()
{
    ADC_GET_Temp(&g_adaptive_con.temp_u, &g_adaptive_con.temp_v, &g_adaptive_con.temp_w, &g_adaptive_con.tempareture);
    ADC_GET_Voltage(&g_adaptive_con.Udc);
    // todo: 启动弱磁控制  更新母线电压   随温度降低电流限制、开启风扇等
}