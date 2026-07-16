/******************************************************************************
Copyright(c) 2016-2018 Digital Power Inc.
File name: ao.h
Author: WamgErChi
Version: 1.0.0
Date: 2018/2/28
Description: Platform of DP X5 audio output api
History:
Bug report: wangerchi@d-power.com.cn
******************************************************************************/


#ifndef __AOAPI_H__
#define __AOAPI_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <audio/IOType.h>


/******************************************************************************
Function: AO_Enable
Description: 使能AO设备
Param:
Return: 成功返回大于等于0的通道号，失败返回错误码
Others:
******************************************************************************/
int AO_Enable(void);


/******************************************************************************
Function: AO_Disable
Description: 关闭 AO 设备
Param:
Return: 成功返回0，失败返回错误码
Others:
******************************************************************************/
int AO_Disable(void);


/******************************************************************************
Function: AO_EnableChn
Description: 使能 AO 设备通道
Param:
	AoChn		in		AO设备通道号
Return: 成功返回0，失败返回错误码
Others:
******************************************************************************/
int AO_EnableChn(AIO_ATTR_S *Attr);


/******************************************************************************
Function: AO_DisableChn
Description: 关闭 AO 设备通道
Param:
	AoChn		in		AO设备通道号
Return: 成功返回0，失败返回错误码
Others: 
******************************************************************************/
int AO_DisableChn(AIO_CHN AoChn);


/******************************************************************************
Function: AO_SetPubAttr
Description: 设置AO 设备属性
Param:
	AoChn		in		AO设备通道号
	pstAttr		in		设置属性参数指针
Return: 成功返回0，失败返回错误码
Others: 此函数已废弃，通道属性通过AO_EnableChn设置
******************************************************************************/
int AO_SetPubAttr(AIO_CHN AoChn, const AIO_ATTR_S *pstAttr);


/******************************************************************************
Function: AO_GetPubAttr
Description: 获取AO 设备属性
Param:
	AoChn		in		AO设备通道号
	pstAttr		out		读取属性参数指针
Return: 成功返回0，失败返回错误码
Others: 设置属性时参数参考 AI_ATTR_S 结构体
******************************************************************************/
int AO_GetPubAttr(AIO_CHN AoChn, AIO_ATTR_S *pstAttr);


/******************************************************************************
Function: AO_SetCache
Description: 设置放音的数据缓存大小
Param:
	ChnID		in		AO设备通道号
	pstCache	in		缓冲区配置参数
Return: 成功返回0，失败返回错误码
Others: 此函数已废弃
******************************************************************************/
int AO_SetCache(AIO_CHN ChnID, const AIO_CACHE_S *pstCache);


/******************************************************************************
Function: AO_GetCache
Description: 获取放音的数据缓存大小配置
Param:
	ChnID			in		AO设备通道号
	pstCache	out		获取缓冲区配置参数指针
Return: 成功返回0，失败返回错误码
Others: 此函数已废弃
******************************************************************************/
int AO_GetCache(AIO_CHN ChnID, AIO_CACHE_S *pstCache);


/******************************************************************************
Function: AO_EnableReSmp
Description: 开启重采样
Param:
	AOChn			in		AO设备通道号
	enOutSampleRate in		重采样输出采样率
Return: 成功返回0，失败返回错误码
Others: 暂未实现
******************************************************************************/
int AO_EnableReSmp(AIO_CHN AOChn, AUDIO_SAMPLE_RATE_E enOutSampleRate);


/******************************************************************************
Function: AO_DisableReSmp
Description: 关闭重采样
Param:
	AOChn			in		AO设备通道号
Return: 成功返回0，失败返回错误码
Others: 暂未实现
******************************************************************************/
int AO_DisableReSmp(AIO_CHN AOChn);


/******************************************************************************
Function: AO_SendFrame
Description: 发送音频帧
Param:
	AOChn			in		AO设备通道号
	pstData			in		音频帧数据
	s32MilliSec		in		超时时间(ms)
Return: 成功返回0，失败返回错误码
Others: 参数s32MilliSec为0时，函数立即返回，上层需要根据返回结果控制发送数据速度
											>0时，函数阻塞，超时时间为s32MilliSec
******************************************************************************/
int AO_SendFrame(AIO_CHN AoChn, AUDIO_FRAME_S *pstData, int s32MilliSec);


/******************************************************************************
Function: AO_ClearChnBuf
Description: 清空通道缓存
Param:
	AoChn		in		AO设备通道号
Return: 成功返回0，失败返回错误码
Others:
******************************************************************************/
int AO_ClearChnBuf(AIO_CHN AoChn);


/******************************************************************************
Function: AO_QueryChnStat
Description: 查询通道状态 忙/空闲
Param:
	AoChn		in		AO设备通道号
	pstStatus	out		AO设备通道状态
Return: 成功返回0，失败返回错误码
Others: 此函数已废弃
******************************************************************************/
int AO_QueryChnStat(void);


/******************************************************************************
Function: AO_PauseChn
Description: 暂停播放
Param:
	AoChn		in		AO设备通道号
Return: 成功返回0，失败返回错误码
Others: 
******************************************************************************/
int AO_PauseChn(AIO_CHN AoChn);


/******************************************************************************
Function: AO_ResumeChn
Description: 重新恢复播放
Param:
	AoChn		in		AO设备通道号
Return: 成功返回0，失败返回错误码
Others:
******************************************************************************/
int AO_ResumeChn(AIO_CHN AoChn);


/******************************************************************************
Function: AO_SetVolume
Description: 设置AI设备音量
Param:
	AoChn		in		AO设备通道号
	s32VolumeDb		in		音量值
Return: 成功返回0，失败返回错误码
Others:
******************************************************************************/
int AO_SetVolume(AIO_CHN AoChn, int s32VolumeDb);


/******************************************************************************
Function: AO_GetVolume
Description: 获取输入音量
Param:
	AoChn		in		AO设备通道号
	ps32VolumeDb	out		获取的音量值
Return: 成功返回0，失败返回错误码
Others:
******************************************************************************/
int AO_GetVolume(AIO_CHN AoChn, int *ps32VolumeDb);


/******************************************************************************
Function: AO_SetSystemVolume
Description: 设置系统放音音量
Param:
    s32Volume   in  设置的音量值，0到100，百分比
Return: 成功返回0，失败返回错误码
Others:
******************************************************************************/
int AO_SetSystemVolume(int s32Volume);

#ifdef __cplusplus
}
#endif

#endif // !__AUDIO_OUT_API_H__
