#include "adaptive_control.h"
#include "adcDr.h"
ADAPTIVE_CON_T g_adaptive_con = {0};

void adaptive_control_init(void)
{
}
void adaptive_control_update()
{
    ADC_GET_Temp(&g_adaptive_con.temp_u, &g_adaptive_con.temp_v, &g_adaptive_con.temp_w, &g_adaptive_con.tempareture);
    g_adaptive_con.Udc = ADC_GET_Voltage();
}