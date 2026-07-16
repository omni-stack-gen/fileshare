/******************************************************************************
Copyright(c) 2016-2018 Digital Power Inc.
File name: ADecApi.h
Author: LiuZhengzhong
Version: 1.0.0
Date: 2018/3/9
Description: Platform of DP X5 audio decode C api
History:
Bug report: liuzhengzhong@d-power.com.cn
******************************************************************************/


#ifndef __ADECAPI_H__
#define __ADECAPI_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <audio/CodecType.h>


/******************************************************************************
Function: ADEC_Init
Description: 初始化音频解码器
Param:
Return: 成功返回1，失败返回0
Others:
******************************************************************************/
int ADEC_Init(void);


/******************************************************************************
Function: ADEC_DeInit
Description: 反初始化音频解码器
Param:
Return: 成功返回1，失败返回0
Others:
******************************************************************************/
int ADEC_DeInit(void);


/******************************************************************************
Function: ADEC_CreateChn
Description: 打开音频解码通道
Param:
    Attr    in      通道属性
Return: 成功返回大于等于0的通道号，失败返回-1
Others:
******************************************************************************/
int ADEC_CreateChn(AUDIO_CHN_ATTR_S *Attr);


/******************************************************************************
Function: ADEC_DestroyChn
Description: 销毁音频解码通道
Param:
    Channel     in      通道号
Return: 成功返回1，失败返回0
Others:
******************************************************************************/
int ADEC_DestroyChn(int Channel);


/******************************************************************************
Function: ADEC_SendStream
Description: 发送需要解码的音频数据
Param:
    Channel     in      通道号
    StreamIn    in      码流属性结构
    StreamOut   out     码流属性结构
Return: 
    MP3和AAC解码:
        文件模式:成功返回1，失败返回0 
        流模式:成功返回解码真实长度, 失败返回 < 0
        
    其他解码:
        成功返回1，失败返回0 
Others:
******************************************************************************/
int ADEC_SendStream(int Channel, ADUIO_STREAM_S *StreamIn, ADUIO_STREAM_S *StreamOut);


/******************************************************************************
Function: ADEC_ClearChnBuf
Description: 清空解码通道缓存
Param:
    Channel     in      通道号
Return: 成功返回1，失败返回0
Others:
******************************************************************************/
int ADEC_ClearChnBuf(int Channel);

/******************************************************************************
Function: ADEC_GetBufferSize
Description: 获取解码器最大输入数据长度
Param:
    Channel     in      通道号
Return: 成功返回一帧音频数据长度，失败返回0
Others:
******************************************************************************/
int ADEC_GetBufferSize(int Channel);

#ifdef __cplusplus
}
#endif

#endif // !__ADECAPI_H__
