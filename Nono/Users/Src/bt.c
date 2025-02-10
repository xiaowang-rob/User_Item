#include "bt.h"
/*
16位报文 0000     0000    0000   0000
        校验      功能     数据
        1001（9）  0-f     00-ff
*/
/*
function
0-
*/

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uint16_t buf=0;
HAL_UART_Receive_IT(&huart,buf,16);
}
void set_date(uint16_t cmd);
void set_time(uint16_t cmd);
