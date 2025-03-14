/**
 *  @file USB_Audio_Port.h
 *
 *  @date 2021-06-01
 *
 *  @author aron566
 *
 *  @brief
 *
 *  @version V1.0
 */
#ifndef USB_AUDIO_PORT_H
#define USB_AUDIO_PORT_H
/** Includes -----------------------------------------------------------------*/
#include <stdbool.h>/**< need definition of BOOL    */
#include "main.h"
/** Exported macros-----------------------------------------------------------*/
#define AUDIO_PORT_CHANNEL_NUMS 2U        /**< MIC音频通道数*/
#define MONO_CHANNEL_SEL 2U               /**< 0使用L声道 1使用R声道 2配置MONO*/
#define AUDIO_PORT_USBD_AUDIO_FREQ 16000U /**< 设置音频采样率*/

/*音频类终端类型定义*/
#define AUDIO_PORT_INPUT_TERMINAL_ID_1 0x01
#define AUDIO_PORT_OUTPUT_TERMINAL_ID_3 0x03
#define AUDIO_PORT_INPUT_CTL_ID_2 0x02

#define AUDIO_PORT_SPEAKE_TERMINAL_TYPE_L 0x01U
#define AUDIO_PORT_SPEAKE_TERMINAL_TYPE_H 0x03U

#define AUDIO_PORT_STREAM_TERMINAL_TYPE_L 0x01U
#define AUDIO_PORT_STREAM_TERMINAL_TYPE_H 0x01U

#define AUDIO_PORT_MICRO_PHONE_TERMINAL_L 0x01U
#define AUDIO_PORT_MICRO_PHONE_TERMINAL_H 0x02U

#define AUDIO_PORT_IN_EP_DIR_ID 0x81  /**< (Direction=IN EndpointID=1)*/
#define AUDIO_PORT_OUT_EP_DIR_ID 0x01 /**< (Direction=OUT EndpointID=1)*/
#define USB_RX_BUF_SIZE_MAX 1024
/*立体声配置*/
#if AUDIO_PORT_CHANNEL_NUMS == 2
/*使用L+R声道*/
#define AUDIO_PORT_CHANNEL_CONFIG_L 0x03U
#define AUDIO_PORT_CHANNEL_CONFIG_H 0x00U
/*单声道配置*/
#elif AUDIO_PORT_CHANNEL_NUMS == 1
#if MONO_CHANNEL_SEL == 0
/*使用L声道*/
#define AUDIO_PORT_CHANNEL_CONFIG_L 0x01U
#define AUDIO_PORT_CHANNEL_CONFIG_H 0x00U
#elif MONO_CHANNEL_SEL == 1
/*使用R声道*/
#define AUDIO_PORT_CHANNEL_CONFIG_L 0x02U
#define AUDIO_PORT_CHANNEL_CONFIG_H 0x00U
#elif MONO_CHANNEL_SEL == 2
/*使用默认MONO声道*/
#define AUDIO_PORT_CHANNEL_CONFIG_L 0x00U
#define AUDIO_PORT_CHANNEL_CONFIG_H 0x00U
#endif
#endif

/*轮询时间间隔*/
#define AUDIO_PORT_FS_BINTERVAL 1U /**< 1ms一次轮询*/
/*音频传输大小设置*/
#define AUDIO_PORT_PACKET_SZE(frq) (uint8_t)(((frq * 2U * 2U) / (1000U / AUDIO_PORT_FS_BINTERVAL)) & 0xFFU), \
                                   (uint8_t)((((frq * 2U * 2U) / (1000U / AUDIO_PORT_FS_BINTERVAL)) >> 8) & 0xFFU)

#define AUDIO_PORT_OUT_SIZE ((AUDIO_PORT_USBD_AUDIO_FREQ * 2U * 2U) / (1000U / AUDIO_PORT_FS_BINTERVAL)) /**< 音频发送大小Byte*/
#define AUDIO_PORT_BUF_SIZE AUDIO_PORT_OUT_SIZE * 4                                                      /**< 音频缓冲区大小 大于3的偶数倍*/
/** Private includes ---------------------------------------------------------*/

/** Use C compiler -----------------------------------------------------------*/
#ifdef __cplusplus ///< use C compiler
extern "C"
{
#endif
  /** Private defines ----------------------------------------------------------*/
  /*环形缓冲区*/
  typedef struct CQ_BUFFER
  {
    uint16_t *pHead;
    uint16_t *pTail;
    uint16_t *write;
    uint16_t *read;
    uint32_t size;
  } CQ_handleTypeDef;
  void I2S_MC_DMA_Init();

  void CQ_16_init(CQ_handleTypeDef *hCq, uint16_t *pBuffer, uint16_t BufferSize);
  void CQ_16putData(CQ_handleTypeDef *hCq, const uint16_t *data, uint32_t size);
  void CQ_16getData(CQ_handleTypeDef *hCq, uint16_t *pData, uint32_t size);
  uint32_t CQ_getLength(CQ_handleTypeDef *hCq);
  /** Exported typedefines -----------------------------------------------------*/

  /** Exported constants -------------------------------------------------------*/

  /** Exported variables -------------------------------------------------------*/
  /** Exported functions prototypes --------------------------------------------*/
	/*音频数据传输初始化*/
	void USB_Audio_Port_Init(void);
  /*是否可以更新音频数据*/
  bool USB_Audio_Port_Can_Update_Data(void);
  /*向USB缓冲区数据加入数据*/
  static inline void USB_Audio_Port_Put_Audio_Data(const int16_t *Data, uint32_t Size);
  /*初始化音频输出端点*/
  uint8_t USB_Audio_Port_EP_IN_Init(void *xpdev, uint8_t cfgidx);
  /*初始化音频输入端点*/
  uint8_t USB_Audio_Port_EP_OUT_Init(void *xpdev, uint8_t cfgidx);
  /*反初始化音频输出端点*/
  uint8_t USB_Audio_Port_EP_IN_DeInit(void *xpdev, uint8_t cfgidx);
  /*反初始化音频输入端点*/
  uint8_t USB_Audio_Port_EP_OUT_DeInit(void *xpdev, uint8_t cfgidx);
  /*USB缓冲区数据发送至HOST*/
  uint8_t USB_Audio_Port_DataIn(void *xpdev, uint8_t epnum);
  /*USB缓冲区数据接收来自HOST*/
  uint8_t USB_Audio_Port_DataOut(void *xpdev, uint8_t epnum);

#ifdef __cplusplus ///< end extern c
}
#endif
#endif
/******************************** End of file *********************************/
