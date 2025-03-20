#include "I2S_MC.h"
#include "usb_buffer_part.h"
#include "i2s.h"

int16_t PCM_DMA[STEREO_PCM_FRAME_SIZE] = {0};
static uint32_t CQ_Len = 0;
uint8_t OK_Flag = 0;
/*音频数据DMA接收回调函数*/
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s == &hi2s2)
    {
        OK_Flag = 1;
    }
}

/*麦克风音频数据传输初始化*/
void MicroPhone_Init()
{
USB_Audio_Port_Init();
HAL_I2S_Receive_DMA(&hi2s2, (uint16_t *)PCM_DMA, STEREO_PCM_FRAME_SIZE);
while (1)
{
    if (OK_Flag)
    {
        if (USB_Audio_Port_Can_Update_Data() == true)
            USB_Audio_Port_Put_Data((int16_t *)PCM_DMA, (int16_t *)PCM_DMA);
        OK_Flag = 0;
        HAL_I2S_Receive_DMA(&hi2s2, (uint16_t *)PCM_DMA, STEREO_PCM_FRAME_SIZE);
    }
}
}

/**
 ******************************************************************
 * @brief   环形缓冲区初始化
 * @param   [in]hCq 缓冲区句柄指针
 * @param   [in]pBuffer 静态存储空间指针
 * @param   [in]BufferSize 分配空间
 * @return  None.
 * @author  xiaowang
 * @version V2.0
 * @date    2025-3-13
 ******************************************************************
 */
void CQ_16_init(CQ_handleTypeDef *hCq, uint16_t *pBuffer, uint16_t BufferSize)
{
    hCq->pHead = pBuffer;
    hCq->pTail = pBuffer + BufferSize - 1;
    hCq->read = pBuffer;
    hCq->write = pBuffer;
    hCq->size = BufferSize;
    hCq->length = &CQ_Len;
}
/**
 ******************************************************************
 * @brief   环形缓冲区装入数据
 * @param   [in]hCq 缓冲区句柄指针
 * @param   [in]data 待装入数据
 * @param   [in]Size 数据大小
 * @return  None.
 * @author  xiaowang
 * @version V2.0
 * @date    2025-3-13
 ******************************************************************
 */
void CQ_16putData(CQ_handleTypeDef *hCq, const uint16_t *data, uint32_t size)
{

    if (*hCq->length + size < (hCq->pTail - hCq->pHead))
    {
        for (int i = 0; i < size; i++)
        {
            if (hCq->write >= hCq->pTail)
            {
                hCq->write = hCq->pHead;
            }
            *(hCq->write) = *(data);
            hCq->write++;
            data++;
        }
        *hCq->length += size;
    }
}
/**
 ******************************************************************
 * @brief   环形缓冲区读取数据
 * @param   [in]hCq 缓冲区句柄指针
 * @param   [in]pData 存入数据指针
 * @param   [in]Size 数据大小
 * @return  None.
 * @author  xiaowang
 * @version V2.0
 * @date    2025-3-13
 ******************************************************************
 */
void CQ_16getData(CQ_handleTypeDef *hCq, uint16_t *pData, uint32_t size)
{
    if (hCq->length - size >= 0)
    {
        for (int i = 0; i < size; i++)
        {
            if (hCq->read >= hCq->pTail)
            {
                hCq->read = hCq->pHead;
            }
            *(pData) = *(hCq->read);
            *(hCq->read) = 0;
            hCq->read++;
            pData++;
        }
        *hCq->length -= size;
    }
}
/**
 ******************************************************************
 * @brief   环形缓冲区获取在存数据大小
 * @param   [in]hCq 缓冲区句柄指针
 * @param   [in]None.
 * @return  在存数据长度
 * @author  xiaowang
 * @version V2.0
 * @date    2025-3-13
 ******************************************************************
 */
uint32_t CQ_getLength(CQ_handleTypeDef *hCq)
{
    return *(hCq->length);
}
