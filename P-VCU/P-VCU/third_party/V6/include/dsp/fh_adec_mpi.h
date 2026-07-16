#ifndef __MPI_ADEC_H__
#define __MPI_ADEC_H__

#include "fh_adec_mpipara.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/**
 * @brief         创建音频解码器通道
 *
 * @param[in]     AdChn              音频解码通道号，0~ADEC_MAX_CHN_NUM
 * @param[in]     pstAttr            音频解码通道属性结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,adec_errcode} 。
 *
 */
FH_SINT32 FH_ADEC_CreateChn(ADEC_CHN AdChn, ADEC_CHN_ATTR_S *pstAttr);

/**
 * @brief         销毁音频解码器通道
 *
 * @param[in]     AdChn              音频解码通道号，0~ADEC_MAX_CHN_NUM
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,adec_errcode} 。
 *
 */
FH_SINT32 FH_ADEC_DestroyChn(ADEC_CHN AdChn);

/**
 * @brief         发送音频解码前数据到解码器
 *
 * @param[in]     AdChn              音频解码通道号，0~ADEC_MAX_CHN_NUM
 * @param[in]     pstStream          解码前数据结构体指针
 * @param[in]     timeout            超时时间，-1 阻塞/0 立即返回
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,adec_errcode} 。
 *
 */
FH_SINT32 FH_ADEC_SendStream(ADEC_CHN AdChn, const AUDIO_STREAM_S *pstStream, FH_SINT32 timeout);

/**
 * @brief         清除音频解码器缓存数据
 *
 * @param[in]     AdChn              音频解码通道号，0~ADEC_MAX_CHN_NUM
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,adec_errcode} 。
 *
 */
FH_SINT32 FH_ADEC_ClearChnBuf(ADEC_CHN AdChn);

/**
 * @brief         获取音频解码后数据
 *
 * @param[in]     AdChn              音频解码通道号，0~ADEC_MAX_CHN_NUM
 * @param[in]     pstFrmInfo         解码后数据结构体指针
 * @param[in]     timeout            超时时间，-1 阻塞/0 立即返回
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,adec_errcode} 。
 *
 */
FH_SINT32 FH_ADEC_GetFrame(ADEC_CHN AdChn, AUDIO_FRAME_S *pstFrmInfo, FH_SINT32 timeout);

/**
 * @brief         释放音频解码通道数据
 *
 * @param[in]     AdChn              音频解码通道号，0~ADEC_MAX_CHN_NUM
 * @param[in]     pstFrmInfo         解码后数据结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,adec_errcode} 。
 *
 */
FH_SINT32 FH_ADEC_ReleaseFrame(ADEC_CHN AdChn, AUDIO_FRAME_S *pstFrmInfo);

/**
 * @brief         音频解码通道绑定初始化
 *
 * @param[in]     s32ModId           注册的模块ID
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,adec_errcode} 。
 *
 * @note
 *    - 暂不支持绑定。
 *
 */
FH_SINT32 FH_ADEC_Bind_Init(FH_SINT32 s32ModId);

/**
 * @brief         音频解码通道绑定去初始化
 *
 * @param[in]     s32ModId           注册的模块ID
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,adec_errcode} 。
 *
 * @note
 *    - 暂不支持绑定。
 *
 */
FH_SINT32 FH_ADEC_Bind_Exit(FH_SINT32 s32ModId);

/**
 * @brief         音频输解码模块扩展API
 *
 * @param[in]     AdChn              音频解码通道号，0~ADEC_MAX_CHN_NUM
 * @param[in]     pAtr               参数结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,adec_errcode} 。
 *
 *
 */
FH_SINT32 FH_ADEC_ExtAPI(ADEC_CHN AdChn, FH_VOID* pAtr);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __MPI_ADEC_H__ */
