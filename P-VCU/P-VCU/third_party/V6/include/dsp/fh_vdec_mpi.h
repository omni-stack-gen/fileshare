#ifndef  __FH_VDEC_MPI_H__
#define  __FH_VDEC_MPI_H__

#include "fh_vdec_mpipara.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

/**
 * @brief         创建视频解码通道
 *
 * @param[in]    VdChn          视频解码通道号
 * @param[in]    pstAttr        解码通道属性指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 * @note
 *    - 通道号不能超出最大的通道号范围。
 *
 */
FH_SINT32 FH_VDEC_CreateChn(VDEC_CHN VdChn, const VDEC_CHN_ATTR_S *pstAttr);

/**
 * @brief         销毁视频解码通道
 *
 * @param[in]    VdChn      视频解码通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 * @note
 *    - 销毁的通道必须已经创建。
 *
 */
FH_SINT32 FH_VDEC_DestroyChn(VDEC_CHN VdChn);

/**
 * @brief         获取视频解码通道属性
 *
 * @param[in]    VdChn          视频解码通道号
 * @param[out]   pstAttr        解码通道属性指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 * @note
 *    - 通道必须已经创建。
 *
 */
FH_SINT32 FH_VDEC_GetChnAttr(VDEC_CHN VdChn, VDEC_CHN_ATTR_S *pstAttr);

/**
 * @brief         解码器开始接收用户发送的码流
 *
 * @param[in]    VdChn      视频解码通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 * @note
 *    - 启动接收码流前必须保证通道已创建。
 *    - 启动接收码流之后，才能发送码流数据。
 *
 */
FH_SINT32 FH_VDEC_StartRecvStream(VDEC_CHN VdChn);

/**
 * @brief         解码器停止接收用户发送的码流
 *
 * @param[in]    VdChn      视频解码通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 * @note
 *    - 停止接收码流前必须保证通道已创建。
 *
 */
FH_SINT32 FH_VDEC_StopRecvStream(VDEC_CHN VdChn);

/**
 * @brief         查询解码通道状态
 *
 * @param[in]    VdChn          视频解码通道号
 * @param[out]   pstStat        视频解码通道状态结构体指
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 * @note
 *    - 查询通道状态前必须保证通道已创建。
 *
 */
FH_SINT32 FH_VDEC_Query(VDEC_CHN VdChn,VDEC_CHN_STAT_S *pstStat);

/**
 * @brief         扩展查询解码通道状态
 *
 * @param[in]    VdChn          视频解码通道号
 * @param[out]   pstStatEx      视频解码通道扩展状态结构体指
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 * @note
 *    - 查询通道状态前必须保证通道已创建。
 *    - 此接口可扩展查询出解码通道的图像宽高。
 *
 */
FH_SINT32 FH_VDEC_QueryEx(VDEC_CHN VdChn, VDEC_CHN_STAT_EX_S *pstStatEx);

/**
 * @brief         获取视频解码通道对应的文件描述符
 *
 * @param[in]    VdChn      视频解码通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 *
 */
FH_SINT32 FH_VDEC_GetFd(VDEC_CHN VdChn);

/**
 * @brief         关闭视频解码通道对应的文件描述符
 *
 * @param[in]    VdChn      视频解码通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 *
 */
FH_SINT32 FH_VDEC_CloseFd(VDEC_CHN VdChn);

/**
 * @brief         复位视频解码通道
 *
 * @param[in]    VdChn      视频解码通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 * @note
 *    - 复位前必须保证通道已创建。
 *
 */
FH_SINT32 FH_VDEC_ResetChn(VDEC_CHN VdChn);

/**
 * @brief         设置解码通道参数
 *
 * @param[in]    VdChn          视频解码通道号
 * @param[in]    pstParam       解码通道属性指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 * @note
 *    - 通道必须已经创建。
 *    - JPEG/MJPEG 不支持该接口。
 *
 */
FH_SINT32 FH_VDEC_SetChnParam(VDEC_CHN VdChn, VDEC_CHN_PARAM_S* pstParam);

/**
 * @brief         获取解码通道参数
 *
 * @param[in]    VdChn          视频解码通道号
 * @param[out]   pstParam       解码通道属性指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 * @note
 *    - 通道必须已经创建。
 *    - JPEG/MJPEG 不支持该接口。
 *
 */
FH_SINT32 FH_VDEC_GetChnParam(VDEC_CHN VdChn, VDEC_CHN_PARAM_S* pstParam);

/**
 * @brief         向视频解码通道发送码流数据
 *
 * @param[in]    VdChn              视频解码通道号
 * @param[in]    pstStream          解码码流结构指针
 * @param[in]    s32MilliSec        送码流方式标志
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 * @note
 *    - 通道必须已经创建。
 *    - 通过改变送码流方式标志可实现，
 *    - 阻塞方式、非阻塞方式、超时方式发送码流。
 *    - 不能发送 bEndOfStream 为0的空码流包（码流长度为0或码流地址为空）。
 *    - 以非阻塞方式发送码流，如果码流缓冲区已满，会立刻返回错误码。
 *
 */
FH_SINT32 FH_VDEC_SendStream(VDEC_CHN VdChn, const VDEC_STREAM_S *pstStream, FH_SINT32 s32MilliSec);

/**
 * @brief         获取视频解码通道的解码图像
 *
 * @param[in]    VdChn             视频解码通道号
 * @param[out]   pstFrameInfo      获取的解码图像结构指针
 * @param[in]    s32MilliSec       获取图像方式标志
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 * @note
 *    - 通道必须已经创建。
 *    - 通过改变获取图像方式标志可实现，
 *    - 阻塞方式、非阻塞方式、超时方式获取图像。
 *    - 以非阻塞方式获取解码图像，如果缓冲区内无图像，会立刻返回错误码。
 *
 */
FH_SINT32 FH_VDEC_GetImage(VDEC_CHN VdChn, VIDEO_FRAME_INFO_S *pstFrameInfo,FH_SINT32 s32MilliSec);

/**
 * @brief         释放视频解码通道的图像
 *
 * @param[in]    VdChn              视频解码通道号
 * @param[out]   pstFrameInfo       释放的解码图像结构指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 * @note
 *    - 通道必须已经创建。
 *    - 需要与获取接口配对使用，获取的数据使用完后需立即释放。
 *    - 释放的必须是同一个通道获取的数据，且图像数据结构不可修改。
 *
 */
FH_SINT32 FH_VDEC_ReleaseImage(VDEC_CHN VdChn, VIDEO_FRAME_INFO_S *pstFrameInfo);


/**
 * @brief         设置通道用户图片
 *
 * @param[in]    VdChn             视频解码通道号
 * @param[out]   pstUsrPic         用户图片结构指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 *
 */
FH_SINT32 FH_VDEC_SetUserPic(VDEC_CHN VdChn, VIDEO_FRAME_INFO_S *pstUsrPic);


/**
 * @brief         使能通道用户图片
 *
 * @param[in]    VdChn         视频解码通道号
 * @param[in]    bInstant      是否即时标志
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 *
 */
FH_SINT32 FH_VDEC_EnableUserPic(VDEC_CHN VdChn, FH_BOOL bInstant);

/**
 * @brief         关闭通道用户图片
 *
 * @param[in]    VdChn      视频解码通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 *
 */
FH_SINT32 FH_VDEC_DisableUserPic(VDEC_CHN VdChn);

/**
 * @brief         设置显示模式
 *
 * @param[in]    VdChn              视频解码通道号
 * @param[in]    enDisplayMode      显示模式枚举
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 * @note
 *    - 通道必须已经创建。
 *
 */
FH_SINT32 FH_VDEC_SetDisplayMode(VDEC_CHN VdChn, VIDEO_DISPLAY_MODE_E enDisplayMode);


/**
 * @brief         获取显示模式
 *
 * @param[in]    VdChn                视频解码通道号
 * @param[out]   penDisplayMode       显示模式枚举指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 * @note
 *    - 通道必须已经创建。
 *
 */
FH_SINT32 FH_VDEC_GetDisplayMode(VDEC_CHN VdChn, VIDEO_DISPLAY_MODE_E *penDisplayMode);

/**
 * @brief         设置解码通的帧buffer个数
 *
 * @param[in]    VdChn          视频解码通道号
 * @param[in]    u32BlkCnt      帧buffer个数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 * @note
 *    - 须在通道创建之前调用，不调用缺省值为4。
 *    - 需要确保帧buffer个数，不小于码流参考帧个数+1。
 *
 */
FH_SINT32 FH_VDEC_SetChnVBCnt(VDEC_CHN VdChn, FH_UINT32 u32BlkCnt);

/**
 * @brief         获取解码通的帧buffer个数
 *
 * @param[in]     VdChn               视频解码通道号
 * @param[out]    pu32BlkCnt          帧buffer个数指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdec_errcode} 。
 *
 *
 */
FH_SINT32 FH_VDEC_GetChnVBCnt(VDEC_CHN VdChn, FH_UINT32 *pu32BlkCnt);




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef  __FH_VDEC_MPI_H__ */

