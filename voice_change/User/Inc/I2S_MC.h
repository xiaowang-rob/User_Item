#include "main.h"


#define STEREO_PCM_FRAME_SIZE 256// 单个声道PCM数据每帧数据长度

/*环形缓冲区*/
typedef struct CQ_BUFFER
{
    uint16_t *pHead;
    uint16_t *pTail;
    uint16_t *write;
    uint16_t *read;
    uint32_t size;
    uint32_t *length;
} CQ_handleTypeDef;

void PCM_TO_PDM(int16_t *pcm_data, uint16_t *pdm_data, uint32_t length);

    void MicroPhone_Init();

void CQ_16_init(CQ_handleTypeDef *hCq, uint16_t *pBuffer, uint16_t BufferSize);
void CQ_16putData(CQ_handleTypeDef *hCq, const uint16_t *data, uint32_t size);
void CQ_16getData(CQ_handleTypeDef *hCq, uint16_t *pData, uint32_t size);
uint32_t CQ_getLength(CQ_handleTypeDef *hCq);