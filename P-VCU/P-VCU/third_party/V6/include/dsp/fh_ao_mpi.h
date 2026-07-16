#ifndef __MPI_AO_H__
#define __MPI_AO_H__

#include "fh_aio_mpipara.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/**
 * @brief         配置音频输出设备属性
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     cmd                音频输出命令号，默认传 IOC_CMD_BUTT
 * @param[in]     pstAttr            音频输出设备属性结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 * @note
 *    - 音频输入设备属性为静态属性，必须在执行 FH_AO_Enable 前配置。
 *
 */
FH_SINT32 FH_AO_SetPubAttr(AUDIO_DEV AoDevId,FH_UINT32 cmd, FH_VOID *pstAttr);

/**
 * @brief         获取音频输出设备的属性
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     cmd                音频输出命令号，默认传 IOC_CMD_BUTT
 * @param[out]    pstAttr            音频输出设备属性结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 */
FH_SINT32 FH_AO_GetPubAttr(AUDIO_DEV AoDevId,FH_UINT32 cmd, FH_VOID *pstAttr);

/**
 * @brief         使能音频输出设备
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 */
FH_SINT32 FH_AO_Enable(AUDIO_DEV AoDevId);

/**
 * @brief         禁用音频输出设备
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 */
FH_SINT32 FH_AO_Disable(AUDIO_DEV AoDevId);

/**
 * @brief         获取音频输出设备句柄
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 */
FH_SINT32 FH_AO_GetFd(AUDIO_DEV AoDevId);

/**
 * @brief         使能音频输出设备通道
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     AoChn              音频输出设备通道号，范围 0~chn_num
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 * @note
 *    - 在 FH_AO_Enable 配置后生效。
 *
 */
FH_SINT32 FH_AO_EnableChn(AUDIO_DEV AoDevId, AO_CHN AoChn);

/**
 * @brief         禁用音频输出设备通道
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     AoChn              音频输出设备通道号，范围 0~chn_num
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 * @note
 *    - 在 FH_AO_Enable 配置后生效。
 *
 */
FH_SINT32 FH_AO_DisableChn(AUDIO_DEV AoDevId, AO_CHN AoChn);

/**
 * @brief         设置音频输出通道参数信息
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     AoChn              音频输出设备通道号，范围 0~chn_num
 * @param[in]     pFrmInfo           通道参数结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 * @note
 *    - 在 FH_AO_EnableChn 配置前调用。
 *
 */
FH_SINT32 FH_AO_SetChnParam(AUDIO_DEV AoDevId, AO_CHN AoChn,AUDIO_CHN_PARAM_S* pFrmInfo);

/**
 * @brief         获取音频输入通道参数信息
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     AoChn              音频输出设备通道号，范围 0~chn_num
 * @param[out]    pFrmInfo           通道参数结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 * @note
 *    - 在 FH_AO_EnableChn 配置后生效。
 *
 */
FH_SINT32 FH_AO_GetChnParam(AUDIO_DEV AoDevId, AO_CHN AoChn,AUDIO_CHN_PARAM_S* pFrmInfo);

/**
 * @brief         发送音频输出通道数据
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     AoChn              音频输出设备通道号，范围 0~chn_num
 * @param[in]     pstData            通道数据结构体指针
 * @param[in]     s32MilliSec        超时时间，-1 阻塞/0 立即返回
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 * @note
 *    - 在 FH_AO_EnableChn 配置后生效。
 *
 */
FH_SINT32 FH_AO_SendFrame(AUDIO_DEV AoDevId, AO_CHN AoChn, AUDIO_FRAME_S *pstData, FH_SINT32 s32MilliSec);

/**
 * @brief         清除音频输出通道缓存数据
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     AoChn              音频输出设备通道号，范围 0~chn_num
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 * @note
 *    - 在 FH_AO_EnableChn 配置后生效。
 *
 */
FH_SINT32 FH_AO_ClearChnBuf(AUDIO_DEV AoDevId ,AO_CHN AoChn);

/**
 * @brief         暂停音频输出通道
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     AoChn              音频输出设备通道号，范围 0~chn_num
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 * @note
 *    - 在 FH_AO_EnableChn 配置后生效，需硬件支持。
 *
 */
FH_SINT32 FH_AO_PauseChn(AUDIO_DEV AoDevId, AO_CHN AoChn);

/**
 * @brief         恢复音频输出通道
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     AoChn              音频输出设备通道号，范围 0~chn_num
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 * @note
 *    - 在 FH_AO_EnableChn 配置后生效，需硬件支持。
 *
 */
FH_SINT32 FH_AO_ResumeChn(AUDIO_DEV AoDevId, AO_CHN AoChn);

/**
 * @brief         设置音频输出设备音量
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     ptr                音量结构体指针，VOLUME_PARAM_S
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 * @note
 *    - 在 FH_AO_Enable 配置后生效。
 *
 */
FH_SINT32 FH_AO_SetVolume(AUDIO_DEV AoDevId, VOLUME_PARAM_S* ptr);

/**
 * @brief         获取音频输出设备音量
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[out]    ptr                音量结构体指针，VOLUME_PARAM_S
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 * @note
 *    - 在 FH_AO_Enable 配置后生效。
 *
 */
FH_SINT32 FH_AO_GetVolume(AUDIO_DEV AoDevId, VOLUME_PARAM_S* ptr);

/**
 * @brief         设置音频输出设备声道模式
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     enTrackMode        声道模式结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 * @note
 *    - 在 FH_AO_Enable 配置后生效，需硬件支持。
 *
 */
FH_SINT32 FH_AO_SetTrackMode(AUDIO_DEV AoDevId, AUDIO_TRACK_MODE_E enTrackMode);

/**
 * @brief         获取音频输出设备声道模式
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[out]    penTrackMode        声道模式结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 * @note
 *    - 在 FH_AO_Enable 配置后生效，需硬件支持。
 *
 */
FH_SINT32 FH_AO_GetTrackMode(AUDIO_DEV AoDevId, AUDIO_TRACK_MODE_E *penTrackMode);

/**
 * @brief         查询音频输出设备通道状态信息
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     AoChn              音频输出设备通道号，范围 0~chn_num
 * @param[out]    pstStatus          状态信息结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 */
FH_SINT32 FH_AO_QueryChnStat(AUDIO_DEV AoDevId ,AO_CHN AoChn, AO_CHN_STATE_S *pstStatus);

/**
 * @brief         配置音频输出设备通道静音状态
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     AoChn              音频输出设备通道号，范围 0~chn_num
 * @param[in]     bEnable            静音状态
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 */
FH_SINT32 FH_AO_SetMute(AUDIO_DEV AoDevId, AO_CHN AoChn, FH_BOOL bEnable);

/**
 * @brief         获取音频输出设备通道静音状态
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     AoChn              音频输出设备通道号，范围 0~chn_num
 * @param[in]     pEnable            静音状态
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 */
FH_SINT32 FH_AO_GetMute(AUDIO_DEV AoDevId, AO_CHN AoChn, FH_BOOL *pEnable);

/**
 * @brief         配置音频输出通道播放数据文件信息
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     AoChn              音频输出设备通道号，范围 0~chn_num
 * @param[in]     pstSaveFileInfo    文件信息结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 */
FH_SINT32 FH_AO_SaveFile(AUDIO_DEV AoDevId, AO_CHN AoChn, AUDIO_SAVE_FILE_INFO_S* pstSaveFileInfo);

/**
 * @brief         查询音频输出通道播放数据文件信息
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     AoChn              音频输出设备通道号，范围 0~chn_num
 * @param[out]    pstFileStatus      文件信息结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 */
FH_SINT32 FH_AO_QueryFileStatus(AUDIO_DEV AoDevId, AO_CHN AoChn, AUDIO_FILE_STATUS_S* pstFileStatus);

/**
 * @brief         清除音频输出设备配置信息
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 */
FH_SINT32 FH_AO_ClrPubAttr(AUDIO_DEV AoDevId);

/**
 * @brief         设置音频输出通道VQE参数信息
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     AoChn              音频输出设备通道号，范围 0~chn_num
 * @param[in]     mode               VQE 算法模式
 * @param[in]     pstVqeAttr         VQE 参数结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 * @note
 *    - 仅 AUDIO_ALGO_MPP_MIC1模式使用，其它模式请参考 FHSes.h。
 *
 */
FH_SINT32 FH_AO_SetVqeAttr(AUDIO_DEV AoDevId, AO_CHN AoChn, AUDIO_ALGO_E mode, FH_VOID *pstVqeAttr);

/**
 * @brief         获取音频输出通道VQE参数信息
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     AoChn              音频输出设备通道号，范围 0~chn_num
 * @param[in]     mode               VQE 算法模式
 * @param[out]    pstVqeAttr         VQE 参数结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 * @note
 *    - 仅 AUDIO_ALGO_MPP_MIC1模式使用，其它模式请参考 FHSes.h。
 *
 */
FH_SINT32 FH_AO_GetVqeAttr(AUDIO_DEV AoDevId, AO_CHN AoChn, AUDIO_ALGO_E mode, FH_VOID *pstVqeAttr);

/**
 * @brief         使能音频通道 VQE
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     AoChn              音频输出设备通道号，范围 0~chn_num
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 *
 */
FH_SINT32 FH_AO_EnableVqe(AUDIO_DEV AoDevId, AO_CHN AoChn);

/**
 * @brief         禁用音频通道 VQE
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     AoChn              音频输出设备通道号，范围 0~chn_num
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 *
 */
FH_SINT32 FH_AO_DisableVqe(AUDIO_DEV AoDevId, AO_CHN AoChn);

/**
 * @brief         音频输入设备绑定初始化
 *
 * @param[in]     s32ModId           注册的模块ID
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 * @note
 *    - 暂不支持绑定。
 *
 */
FH_SINT32 FH_AO_Bind_Init(FH_SINT32 s32ModId);

/**
 * @brief         音频输出设备绑定去初始化
 *
 * @param[in]     s32ModId           注册的模块ID
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 * @note
 *    - 暂不支持绑定。
 *
 */
FH_SINT32 FH_AO_Bind_Exit(FH_SINT32 s32ModId);

/**
 * @brief         音频输出模块扩展API
 *
 * @param[in]     AoDevId            音频输出设备号，参考 AUDIO_DEVID
 * @param[in]     AoChn              音频输出设备通道号，范围 0~chn_num
 * @param[in]     pAtr               参数结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 *
 */
FH_SINT32 FH_AO_ExtAPI(AUDIO_DEV AoDevId, AO_CHN AoChn, FH_VOID* pAtr);

/**
 * @brief		  音频输出通道获取当前播放帧数据(用于回采)
 *
 * @param[in]	  AoDevId			 音频输出设备号，参考 AUDIO_DEVID
 * @param[in]	  AoChn 			 音频输出设备通道号，范围 0~chn_num
 * @param[in]	  pAtr				 参数结构体指针
 *
 * @retval	0			  成功
 * @retval	"非0"			失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 *
 */
FH_SINT32 FH_AO_GetPlayingFrame(AUDIO_DEV AoDevId, AO_CHN AoChn, AUDIO_FRAME_S* pAtr);

/**
 * @brief		  等待音频输出通道把缓存数据播放完毕(阻塞当前调用线程，直到播放完毕)
 *
 * @param[in]	  AoDevId			 音频输出设备号，参考 AUDIO_DEVID
 * @param[in]	  AoChn 			 音频输出设备通道号，范围 0~chn_num
 *
 * @retval	0			  成功
 * @retval	"非0"			失败，其值参见 @namelink{错误码,ao_errcode} 。
 *
 *
 */
FH_SINT32 FH_AO_WaitPlayComplete(AUDIO_DEV AoDevId, AO_CHN AoChn);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __MPI_AO_H__ */

