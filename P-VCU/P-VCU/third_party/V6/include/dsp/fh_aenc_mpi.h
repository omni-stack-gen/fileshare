#ifndef __MPI_AENC_H__
#define __MPI_AENC_H__

#include "fh_aenc_mpipara.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/**
 * @brief         创建音频编码器通道
 *
 * @param[in]     AeChn              音频编码通道号，0~AENC_MAX_CHN_NUM
 * @param[in]     pstAttr            音频编码通道属性结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,aenc_errcode} 。
 *
 */
FH_SINT32 FH_AENC_CreateChn(AENC_CHN AeChn, AENC_CHN_ATTR_S *pstAttr);

/**
 * @brief         销毁音频编码器通道
 *
 * @param[in]     AeChn              音频编码通道号，0~AENC_MAX_CHN_NUM
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,aenc_errcode} 。
 *
 */
FH_SINT32 FH_AENC_DestroyChn(AENC_CHN AeChn);

/**
 * @brief         获取编码通道编码后码流数据
 *
 * @param[in]     AeChn              音频编码通道号，0~AENC_MAX_CHN_NUM
 * @param[out]    pstStream          音频编码后数据结构体指针
 * @param[in]     timeout            超时时间,-1 阻塞/0 立即返回
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,aenc_errcode} 。
 *
 */
FH_SINT32 FH_AENC_GetStream(AENC_CHN AeChn, AUDIO_STREAM_S *pstStream, FH_SINT32 timeout);

/**
 * @brief         清除音频编码通道缓存数据
 *
 * @param[in]     AeChn              音频编码通道号，0~AENC_MAX_CHN_NUM
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,aenc_errcode} 。
 *
 */
FH_SINT32 FH_AENC_ClearChnBuf(AENC_CHN AeChn);

/**
 * @brief         发送音频编码前数据给编码器
 *
 * @param[in]     AeChn              音频编码通道号，0~AENC_MAX_CHN_NUM
 * @param[in]     pstFrmInfo         编码前数据结构体指针
 * @param[in]     timeout            超时时间， -1 阻塞/0 立即返回
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,aenc_errcode} 。
 *
 */
FH_SINT32 FH_AENC_SendFrame(AENC_CHN AeChn, AUDIO_FRAME_S *pstFrmInfo, FH_SINT32 timeout);

/**
 * @brief         释放编码数据结构体
 *
 * @param[in]     AeChn              音频编码通道号，0~AENC_MAX_CHN_NUM
 * @param[in]     pstFrmInfo         编码前数据结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,aenc_errcode} 。
 *
 */
FH_SINT32 FH_AENC_ReleaseFrame(AENC_CHN AeChn, AUDIO_FRAME_S *pstFrmInfo);

/**
 * @brief         音频编码器绑定初始化
 *
 * @param[in]     s32ModId           注册的模块ID
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,aenc_errcode} 。
 *
 * @note
 *    - 暂不支持绑定。
 *
 */
FH_SINT32 FH_AENC_Bind_Init(FH_SINT32 s32ModId);

/**
 * @brief         音频编码器绑定去初始化
 *
 * @param[in]     s32ModId           注册的模块ID
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,aenc_errcode} 。
 *
 * @note
 *    - 暂不支持绑定。
 *
 */
FH_SINT32 FH_AENC_Bind_Exit(FH_SINT32 s32ModId);

/**
 * @brief         音频编码器扩展API
 *
 * @param[in]     AeChn              音频编码通道号，0~AENC_MAX_CHN_NUM
 * @param[in]     pAtr               参数结构体指针
 *
 * @retval  0             成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,aenc_errcode} 。
 *
 *
 */
FH_SINT32 FH_AENC_ExtAPI(AENC_CHN AeChn, FH_VOID* pAtr);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __MPI_AENC_H__ */
