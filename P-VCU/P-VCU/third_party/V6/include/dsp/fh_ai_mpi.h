#ifndef __MPI_AI_H__
#define __MPI_AI_H__

#include "fh_aio_mpipara.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/**
 * @brief         配置音频输入设备的属性
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     cmd                音频输入命令号，默认传 IOC_CMD_BUTT
 * @param[in]     pstAttr            音频输入设备属性结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 * @note
 *    - 必须在执行 FH_AI_Enable 前配置。
 *
 */
FH_SINT32 FH_AI_SetPubAttr(AUDIO_DEV AiDevId,FH_UINT32 cmd, FH_VOID *pstAttr);

/**
 * @brief         获取音频输入设备的属性
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     cmd                音频输入命令号，默认传 IOC_CMD_BUTT
 * @param[out]    pstAttr            音频输入设备属性结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 */
FH_SINT32 FH_AI_GetPubAttr(AUDIO_DEV AiDevId,FH_SINT32 cmd, FH_VOID *pstAttr);

/**
 * @brief         使能音频输入设备
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 */
FH_SINT32 FH_AI_Enable(AUDIO_DEV AiDevId);

/**
 * @brief         禁用音频输入设备
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 */
FH_SINT32 FH_AI_Disable(AUDIO_DEV AiDevId);

/**
 * @brief         获取音频输入设备文件句柄
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 */
FH_SINT32 FH_AI_GetDevFd(AUDIO_DEV AiDevId);

/**
 * @brief         配置音频输入设备声道模式
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     enTrackMode        声道属性参数，参考 AUDIO_TRACK_MODE_E
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 * @note
 *    - 仅支持AUDIO_TRACK_LEFT/AUDIO_TRACK_RIGHT/AUDIO_TRACK_MIX。
 *
 */
FH_SINT32 FH_AI_SetTrackMode(AUDIO_DEV AiDevId, AUDIO_TRACK_MODE_E enTrackMode);

/**
 * @brief         获取音频输入设备声道模式
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[out]    penTrackMode       声道属性参数指针，参考 AUDIO_TRACK_MODE_E
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 * @note
 *    - 仅支持 AUDIO_TRACK_LEFT / AUDIO_TRACK_RIGHT / AUDIO_TRACK_MIX。
 *
 */
FH_SINT32 FH_AI_GetTrackMode(AUDIO_DEV AiDevId, AUDIO_TRACK_MODE_E *penTrackMode);

/**
 * @brief         使能音频输入设备通道
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     AiChn              音频输入设备通道号，范围 0~chn_num
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 * @note
 *    - 在 FH_AI_Enable 配置后生效。
 *
 */
FH_SINT32 FH_AI_EnableChn(AUDIO_DEV AiDevId, AI_CHN AiChn);

/**
 * @brief         禁用音频输入设备通道
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     AiChn              音频输入设备通道号，范围 0~chn_num
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 * @note
 *    - 在 FH_AI_Enable 配置后生效。
 *
 */
FH_SINT32 FH_AI_DisableChn(AUDIO_DEV AiDevId, AI_CHN AiChn);

/**
 * @brief         暂停音频输入设备通道
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     AiChn              音频输入设备通道号，范围 0~chn_num
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 * @note
 *    - 在 FH_AI_Enable 配置后生效。
 *
 */
FH_SINT32 FH_AI_PauseChn(FH_UINT32 AiDevId, AI_CHN AiChn);

/**
 * @brief         恢复音频输入设备通道
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     AiChn              音频输入设备通道号，范围 0~chn_num
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 * @note
 *    - 在 FH_AI_Enable 配置后生效，需硬件支持。
 *
 */
FH_SINT32 FH_AI_ResumeChn(FH_UINT32 AiDevId, AI_CHN AiChn);

/**
 * @brief         获取音频输入设备数据信息
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     AiChn              音频输入设备通道号，范围 0~chn_num
 * @param[out]    pstFrm             VQE使能后取到算法处理后音频数据指针结构体,默认裸数据结构体
 * @param[out]    pstAecFrm          arc驱动双回采/ACW硬回采方案获取回采数据结构体
 * @param[in]     s32MilliSec        超时时间，-1 阻塞/ 0 返回
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 * @note
 *    - 在 FH_AI_Enable 配置后生效，需硬件支持。
 *
 */
FH_SINT32 FH_AI_GetChnFrame(AUDIO_DEV AiDevId, AI_CHN AiChn, AUDIO_FRAME_S *pstFrm, AEC_FRAME_S *pstAecFrm, FH_SINT32 s32MilliSec);

/**
 * @brief         释放音频输入设备数据信息
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     AiChn              音频输入设备通道号，范围 0~chn_num
 * @param[in]     pstFrm             VQE处理前音频数据
 * @param[in]     pstAecFrm          VQE处理后音频，仅支持 AUDIO_ALGO_MPP_MIC1
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 * @note
 *    - 在 FH_AI_Enable 配置后生效。
 *
 */
FH_SINT32 FH_AI_ReleaseChnFrame(AUDIO_DEV AiDevId, AI_CHN AiChn, AUDIO_FRAME_S *pstFrm, AEC_FRAME_S *pstAecFrm);

/**
 * @brief         设置音频输入通道参数信息
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     AiChn              音频输入设备通道号，范围 0~chn_num
 * @param[in]     pFrmInfo           通道参数结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 * @note
 *    - 在 FH_AI_EnableChn 配置前调用。
 *
 */
FH_SINT32 FH_AI_SetChnParam(AUDIO_DEV AiDevId, AI_CHN AiChn, AUDIO_CHN_PARAM_S* pFrmInfo);

/**
 * @brief         获取音频输入通道参数信息
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     AiChn              音频输入设备通道号，范围 0~chn_num
 * @param[out]    pFrmInfo           通道参数结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 * @note
 *    - 在 FH_AI_EnableChn 配置后生效。
 *
 */
FH_SINT32 FH_AI_GetChnParam(AUDIO_DEV AiDevId, AI_CHN AiChn, AUDIO_CHN_PARAM_S* pFrmInfo);

/**
 * @brief         设置音频输入设备音量
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     ptr                音量参数结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 *
 */
FH_SINT32 FH_AI_SetVolume(AUDIO_DEV AiDevId, VOLUME_PARAM_S* ptr);

/**
 * @brief         获取音频输入设备音量
 *
 * @param[in]      AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[out]     ptr     			  音量参数结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 *
 */
FH_SINT32 FH_AI_GetVolume(AUDIO_DEV AiDevId, VOLUME_PARAM_S* ptr);

/**
 * @brief         设置音频输入通道VQE参数信息
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     AiChn              音频输入设备通道号，范围 0~chn_num
 * @param[in]     mode               VQE 算法模式
 * @param[in]    pstVqeAttr         VQE 参数结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 * @note
 *    - 仅 AUDIO_ALGO_MPP_MIC1模式使用，其它模式请参考 FHSes.h。
 *
 */
FH_SINT32 FH_AI_SetVqeAttr(AUDIO_DEV AiDevId, AI_CHN AiChn, AUDIO_ALGO_E mode, FH_VOID *pstVqeAttr);

/**
 * @brief         获取音频输入通道VQE参数信息
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     AiChn              音频输入设备通道号，范围 0~chn_num
 * @param[in]     mode               VQE 算法模式
 * @param[out]    pstVqeAttr         VQE 参数结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 * @note
 *    - 仅 AUDIO_ALGO_MPP_MIC1模式使用，其它模式请参考 FHSes.h。
 *
 */
FH_SINT32 FH_AI_GetVqeAttr(AUDIO_DEV AiDevId, AI_CHN AiChn, AUDIO_ALGO_E mode, FH_VOID *pstVqeAttr);

/**
 * @brief         使能音频通道 VQE
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     AiChn              音频输入设备通道号，范围 0~chn_num
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 *
 */
FH_SINT32 FH_AI_EnableVqe(AUDIO_DEV AiDevId, AI_CHN AiChn);

/**
 * @brief         禁用音频通道 VQE
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     AiChn              音频输入设备通道号，范围 0~chn_num
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 *
 */
FH_SINT32 FH_AI_DisableVqe(AUDIO_DEV AiDevId, AI_CHN AiChn);

/**
 * @brief         使能音频输入通道重采样
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     AiChn              音频输入设备通道号，范围 0~chn_num
 * @param[in]     ptr                重采样参数结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 *
 */
FH_SINT32 FH_AI_EnableReSmp(AUDIO_DEV AiDevId, AI_CHN AiChn, RESAMPLE_PARAM_S* ptr);

/**
 * @brief         禁用音频输入通道重采样
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     AiChn              音频输入设备通道号，范围 0~chn_num
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 *
 */
FH_SINT32 FH_AI_DisableReSmp(AUDIO_DEV AiDevId, AI_CHN AiChn);

/**
 * @brief         查询音频输入通道文件状态信息
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     AiChn              音频输入设备通道号，范围 0~chn_num
 * @param[out]    pstFileStatus      文件状态信息结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 *
 */
FH_SINT32 FH_AI_QueryFileStatus(AUDIO_DEV AiDevId, AI_CHN AiChn, AUDIO_SAVE_FILE_INFO_S* pstFileStatus);

/**
 * @brief         配置音频输入通道文件保存信息
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     AiChn              音频输入设备通道号，范围 0~chn_num
 * @param[out]    pstSaveFileInfo      文件状态信息结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 * @note
 *    - 保存的文件路径要有可写权限。
 *
 */
FH_SINT32 FH_AI_SaveFile(AUDIO_DEV AiDevId, AI_CHN AiChn, AUDIO_SAVE_FILE_INFO_S *pstSaveFileInfo);

/**
 * @brief         音频输入设备绑定初始化
 *
 * @param[in]     s32ModId           注册的模块ID
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 * @note
 *    - 暂不支持绑定。
 *
 */
FH_SINT32 FH_AI_Bind_Init(FH_SINT32 s32ModId);

/**
 * @brief         音频输入设备绑定去初始化
 *
 * @param[in]     s32ModId           注册的模块ID
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 * @note
 *    - 暂不支持绑定。
 *
 */
FH_SINT32 FH_AI_Bind_Exit(FH_SINT32 s32ModId);

/**
 * @brief         音频输入模块扩展API
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     AiChn              音频输入设备通道号，范围 0~chn_num
 * @param[in]     pAtr               参数结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 *
 */
FH_SINT32 FH_AI_ExtAPI(AUDIO_DEV AiDevId, AI_CHN AiChn, FH_VOID* pAtr);

/**
 * @brief         音频输入通道上电 POP 音处理
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     AiChn              音频输入设备通道号，范围 0~chn_num
 * @param[in]     pcm_buf            数据结构体指针
 * @param[in]     size               数据长度
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 *
 */
FH_SINT16 FH_AI_VolCtrl_increase(AUDIO_DEV AiDevId, AI_CHN AiChn, FH_UINT16* pcm_buf, FH_UINT32 size);

/**
 * @brief         音频输入通道下电 POP 音处理
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     AiChn              音频输入设备通道号，范围 0~chn_num
 * @param[in]     pcm_buf            数据结构体指针
 * @param[in]     size               数据长度
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 *
 */
FH_SINT16 FH_AI_VolCtrl_decrease(AUDIO_DEV AiDevId, AI_CHN AiChn, FH_UINT16* pcm_buf, FH_UINT32 size);

/**
 * @brief         音频输入通道 POP 音初始化
 *
 * @param[in]     AiDevId            音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     AiChn              音频输入设备通道号，范围 0~chn_num
 * @param[in]     len	             去除pop音数据长度
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 *
 */
FH_SINT32 FH_AI_VolCtrl_init(AUDIO_DEV AiDevId, AI_CHN AiChn, FH_UINT32 len);

/**
 * @brief         音频输入输出设备同步接口
 *
 * @param[in]     sync            同步命令，参考 AUDIO_AIO_SYNC
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 *
 */
FH_SINT32 FH_AI_AO_SYNC(AUDIO_AIO_SYNC sync);

/**
 * @brief         设置音频输入硬回采同步信息
 *
 * @param[in]     ptr            参数指针，参考 demo
 * @param[in]     len            指针长度，用于MPI数据校验
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 *
 */
FH_SINT32 FH_AI_Set_HwEchoInfo(FH_VOID* ptr, FH_UINT32 len);

/**
 * @brief         设置音频输入设备数据获取方式
 *
 * @param[in]     AiDevId       音频输入设备号，参考 AUDIO_DEVID
 * @param[in]     tp            数据获取方式[0:arm侧配置/arm侧获取数据;1:arm侧配置/arc侧获取数据;2:arc侧配置/arc侧获取数据]
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 *
 */
FH_SINT32 FH_AI_Set_FrmTp(FH_UINT32 AiDevId, FH_UINT32 tp);

/**
 * @brief         AUDIO数据重采样
 *
 * @param[in]     param_in       音频输入数据信息结构体，参考 AUDIO_FRAME_S
 * @param[in,out] param_out      音频输出信息结构体，参考 AUDIO_FRAME_S
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 * @note
 *    - enFsRate/enBitwidth/enSoundmode/u32Len/pVirAddr必须传入,只支持升采样。
 *
 */
FH_SINT32 FH_AI_Data_ReSampleUp(AUDIO_FRAME_S* param_in, AUDIO_FRAME_S* param_out);

/**
 * @brief         定向拾音算法配置
 *
 * @param[in]		argv		音频输入设备号/通道号信息结构体
 * @param[in]		argc      	输入结构体个数，最大支持 4mic
 * @param[in]		para      	控制参数:0-不做回声消除;1-软回采回声消除;2-硬回采回声消除;
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ai_errcode} 。
 *
 * @note
 *    - enFsRate/enBitwidth/enSoundmode/u32Len/pVirAddr必须传入,只支持升采样。
 *
 */
FH_SINT32 FH_AI_Directional_Pickup(DMIC_EN_CFG argv[], FH_UINT32 argc, FH_UINT32 para);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __MPI_AI_H__ */
