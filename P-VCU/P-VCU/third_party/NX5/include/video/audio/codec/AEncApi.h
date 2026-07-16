/******************************************************************************
Copyright(c) 2016-2018 Digital Power Inc.
File name: AEncApi.h
Author: LiuZhengzhong
Version: 1.0.0
Date: 2018/3/9
Description: Platform of DP X5 audio encode C api
History:
Bug report: liuzhengzhong@d-power.com.cn
******************************************************************************/


#ifndef __AENCAPI_H__
#define __AENCAPI_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <audio/CodecType.h>


/******************************************************************************
Function: AENC_Init
Description: 初始化音频编码器
Param:
Return: 成功返回1，失败返回0
Others:
******************************************************************************/
int AENC_Init(void);


/******************************************************************************
Function: AENC_DeInit
Description: 反初始化音频编码器
Param:
Return: 成功返回1，失败返回0
Others:
******************************************************************************/
int AENC_DeInit(void);


/******************************************************************************
Function: AENC_CreateChn
Description: 打开音频编码通道
Param:
    Attr    in      通道属性
Return: 成功返回大于等于0的通道号，失败返回-1
Others:
******************************************************************************/
int AENC_CreateChn(AUDIO_CHN_ATTR_S *Attr);


/******************************************************************************
Function: AENC_DestroyChn
Description: 销毁音频编码通道
Param:
    Channel     in      通道号
Return: 成功返回1，失败返回0
Others:
******************************************************************************/
int AENC_DestroyChn(int Channel);


/******************************************************************************
Function: AENC_SendStream
Description: 发送需要编码的音频数据
Param:
    Channel     in      通道号
    StreamIn    in      码流属性结构
    StreamOut   out     码流属性结构
Return: 
    MP3和AAC编码:
        文件模式:成功返回1，失败返回0 
        流模式:成功返回编码真实长度, 失败返回 < 0
        
    其他编码:
        成功返回1，失败返回0 
Others:
******************************************************************************/
int AENC_SendStream(int Channel, ADUIO_STREAM_S *StreamIn, ADUIO_STREAM_S *StreamOut);


/******************************************************************************
Function: AENC_ClearChnBuf
Description: 清空编码通道缓存
Param:
    Channel     in      通道号
Return: 成功返回1，失败返回0
Others:
******************************************************************************/
int AENC_ClearChnBuf(int Channel);


/******************************************************************************
Function: AENC_GetBufferSize
Description: 获取输入编码器数据长度
Param:
    Channel     in      通道号
Return: 成功返回一帧音频数据长度，失败返回0
Others:
******************************************************************************/
int AENC_GetBufferSize(int Channel);

#ifdef __cplusplus
}
#endif

#endif // !__AENCAPI_H__
