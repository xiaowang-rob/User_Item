#ifndef __DATA_MONITORING_H
#define __DATA_MONITORING_H

#include "bsp.h"
#include "protocol.h"
// 读取数据流
void fStreamDataGet(eData_stream stream, float *data);
void fStreamDataPrepare(eData_stream stream, u8 index, u8 *data, bool _tx);
#endif