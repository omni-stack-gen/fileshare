/******************************************************************************
Copyright(c) 2016-2018 Digital Power Inc.
File name: ai.h
Author: WamgErChi
Version: 1.0.0
Date: 2018/2/28
Description: Platform of DP X5 audio input api
History:
Bug report: wangerchi@d-power.com.cn
******************************************************************************/


#ifndef __AIAPI_H__
#define __AIAPI_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <audio/IOType.h>


/******************************************************************************
Function: AI_Enable
Description: 使能 AI 设备,用于初始化并创建一个通道
Param:
Return: 成功返回0，失败返回错误码
Others: 可反复调用
******************************************************************************/
int AI_Enable();


/******************************************************************************
Function: AI_Disable
Description: 关闭 AI 设备
Param:
Return: 成功返回0，失败返回错误码
Others: 自动销毁创建的通道
******************************************************************************/
int AI_Disable();


/******************************************************************************
Function: AI_EnableChn
Description: 使能 AI 设备通道
Param:
	AiChn		in		AI设备通道号
Return: 成功返回>=0通道号，失败返回错误码
Others:
******************************************************************************/
int AI_EnableChn(AIO_ATTR_S *Attr);


/******************************************************************************
Function: AI_EnableChnEx
Description: 使能 AI 设备通道
Param:
	AiChn		in		AI设备通道号
	bloopback   in		是否为回采音频(如果为false, 功能同AI_EnableChn)
Return: 成功返回>=0通道号，失败返回错误码
Others:
******************************************************************************/
int AI_EnableChnEx(AIO_ATTR_S *Attr, bool bloopback);

/******************************************************************************
Function: AI_DisableChn
Description: 关闭 AI 设备通道
Param:
	AiChn		in		AI设备通道号
Return: 成功返回0，失败返回错误码
Others:
******************************************************************************/
int AI_DisableChn(AIO_CHN ChnID);


/******************************************************************************
Function: AI_SetPubAttr
Description: 设置AI设备属性
Param:
	pstAttr		in		设置属性参数指针
Return: 成功返回0，失败返回错误码
Others: 此函数已废弃，无实际作用，永远返回0，在AI_EnableChn时设置通道属性
******************************************************************************/
int AI_SetPubAttr(AIO_CHN ChnID, AIO_ATTR_S *pstAttr);


/******************************************************************************
Function: AI_GetPubAttr
Description: 获取AI设备属性
Param:
	pstAttr		out		读取属性参数指针
Return: 成功返回0，失败返回错误码
Others: 属性参数参考 AI_ATTR_S结构体
******************************************************************************/
int AI_GetPubAttr(AIO_CHN ChnID, AIO_ATTR_S *pstAttr);


/******************************************************************************
Function: AI_SetCache
Description: 设置录音的数据缓存大小
Param:
	AiChn		in		AI设备通道号
	Blocks		in		数据缓存块个数
	BlockTime	in		每个数据缓存块缓存的录音数据时长(ms)
Return: 成功返回0，失败返回错误码
Others: 此函数已废弃，无实际作用，永远返回0，在AI_EnableChn时设置通道属性
******************************************************************************/
int AI_SetCache(AIO_CHN ChnID, const AIO_CACHE_S *pstCache);


/******************************************************************************
Function: AI_GetCache
Description: 获取录音的数据缓存大小配置
Param:
	AiChn			in		AI设备通道号
	pstCache	out		获取缓冲区配置参数指针
Return: 成功返回0，失败返回错误码
Others: 此函数已废弃，无实际作用，永远返回0，通过AI_GetPubAttr可以获得Cache属性
******************************************************************************/
int AI_GetCache(AIO_CHN ChnID, AIO_CACHE_S *pstCache);


/******************************************************************************
Function: AI_EnableAec
Description: 开启回声消除
Param:
Return: 成功返回0，失败返回错误码
Others:
******************************************************************************/
int AI_EnableAec();


/******************************************************************************
Function: AI_DisableAec
Description: 关闭回声消除
Param:
Return: 成功返回0，失败返回错误码
Others:
******************************************************************************/
int AI_DisableAec();


/******************************************************************************
Function: AI_EnableReSmp
Description: 开启重采样
Param:
	AudioDevId		in      AI设备号
	AiChn			in		AI设备通道号
	enOutSampleRate in		重采样输出采样率
Return: 成功返回0，失败返回错误码
Others: 暂未实现
******************************************************************************/
int AI_EnableReSmp(AIO_CHN AiChn, AUDIO_SAMPLE_RATE_E enOutSampleRate);


/******************************************************************************
Function: AI_DisableReSmp
Description: 关闭重采样
Param:
	AudioDevId		in      AI设备号
	AiChn			in		AI设备通道号
Return: 成功返回0，失败返回错误码
Others: 暂未实现
******************************************************************************/
int AI_DisableReSmp(AIO_CHN AiChn);


/******************************************************************************
Function: AI_GetFrame
Description: 获取音频帧
Param:
	AiChn			in		AI设备通道号
Return: 成功返回0，失败返回错误码
Others: 参数s32MilliSec为0时，函数立即返回，上层需要根据返回结果控制读取数据速度
											>0时，函数阻塞，超时时间为s32MilliSec
******************************************************************************/
int AI_GetFrame(AIO_CHN AiChn, AUDIO_FRAME_S *pstFrm, int s32MilliSec);


/******************************************************************************
Function: AI_ReleaseFrame
Description: 释放音频帧
Param:
	AiChn			in		AI设备通道号
Return: 成功返回0，失败返回错误码
Others: 此函数已废弃
******************************************************************************/
int AI_ReleaseFrame(AIO_CHN AiChn, AUDIO_FRAME_S *pstFrm);


/******************************************************************************
Function: AI_SetVolume
Description: 设置AI设备音量
Param:
	s32VolumeDb		in		音量值
Return: 成功返回0，失败返回错误码
Others:
******************************************************************************/
int AI_SetVolume(AIO_CHN ChnID, int s32VolumeDb);


/******************************************************************************
Function: AI_GetVolume
Description: 获取输入音量
Param:
	ps32VolumeDb	out		获取的音量值
Return: 成功返回0，失败返回错误码
Others:
******************************************************************************/
int AI_GetVolume(AIO_CHN ChnID, int *ps32VolumeDb);


/******************************************************************************
Function: AI_SetSystemVolume
Description: 设置audio in 系统音量
Param:
    s32Volume   in  设置的音量值，0到100，百分比
Return: 成功返回0，失败返回错误码
Others:
******************************************************************************/
int AI_SetSystemVolume(int s32Volume);

#ifdef __cplusplus
}
#endif

#endif // !__AIAPI_H__
