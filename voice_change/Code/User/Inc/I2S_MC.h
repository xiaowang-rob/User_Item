#include "main.h"

#define STEREO_PCM_FRAME_SIZE 32 // 单个声道PCM数据每次接收数据点数

/*环形缓冲区*/
typedef struct CQ_32_BUFFER
{
    uint32_t *pHead;
    uint32_t *pTail;
    uint32_t *write;
    uint32_t *read;
    uint32_t size;
    uint32_t *length;
} CQ_handleTypeDef;

void MicroPhone_Init();
void CQ_32_init(CQ_handleTypeDef *hCq, uint32_t *pBuffer, uint16_t BufferSize);
void CQ_32putData(CQ_handleTypeDef *hCq, const uint32_t *data, uint16_t size);
void CQ_32getData(CQ_handleTypeDef *hCq, uint32_t *pData, uint16_t size);
uint32_t CQ_getLength(CQ_handleTypeDef *hCq);