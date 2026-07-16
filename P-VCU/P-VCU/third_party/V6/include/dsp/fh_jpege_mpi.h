#ifndef __FH_JPEGE_MPI_H__
#define __FH_JPEGE_MPI_H__

#include "fh_jpege_mpipara.h"
#include "fh_venc_mpipara.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


/**
 * @brief         创建JPEG编码通道
 *
 * @param[in]    JpegeChn      JPEG编码通道号
 * @param[in]    pstAttr       JPEG编码通道属性指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 * @note
 *    - 通道号不能超出最大的通道号范围。
 *
 */
FH_SINT32 FH_JPEGE_CreateChn(JPEGE_CHN JpegeChn, const JPEGE_CHN_ATTR_S *pstAttr);

/**
 * @brief         销毁JPEG编码通道
 *
 * @param[in]    JpegeChn      JPEG编码通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 * @note
 *    - 销毁的通道必须已经创建。
 *
 */
FH_SINT32 FH_JPEGE_DestroyChn(JPEGE_CHN JpegeChn);

/**
 * @brief         复位JPEG编码通道
 *
 * @param[in]    JpegeChn      JPEG编码通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 * @note
 *    - 复位前必须保证通道已创建。
 *
 */
FH_SINT32 FH_JPEGE_ResetChn(JPEGE_CHN JpegeChn);

/**
 * @brief         JPEG编码开始接收待编码视频
 *
 * @param[in]    JpegeChn       JPEG编码通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 * @note
 *    - 启动接收码流前必须保证通道已创建。
 *    - 启动接收码流之后，才能发送码流数据。
 *
 */
FH_SINT32 FH_JPEGE_StartRecvPic(JPEGE_CHN JpegeChn);

/**
 * @brief         JPEG编码开始接收待编码视频
 *
 * @param[in]    JpegeChn      JPEG编码通道号
 * @param[in]    pstRecvParam  JPEG编码连续编码帧数(然后不接收视频帧数据)指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 * @note
 *    - 启动接收码流前必须保证通道已创建。
 *    - 启动接收码流之后，才能发送码流数据。
 *
 */
FH_SINT32 FH_JPEGE_StartRecvPicEx(JPEGE_CHN JpegeChn, JPEGE_RECV_PIC_PARAM_S *pstRecvParam);

/**
 * @brief         JPEG编码通停止接收用户发送的码流
 *
 * @param[in]    JpegeChn       JPEG编码通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 * @note
 *    - 停止接收码流前必须保证通道已创建。
 *
 */
FH_SINT32 FH_JPEGE_StopRecvPic(JPEGE_CHN JpegeChn);

/**
 * @brief         查询解码通道状态
 *
 * @param[in]    JpegeChn       JPEG编码通道号
 * @param[out]   pstStat        JPEG编码通道状态结构体指
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 * @note
 *    - 查询通道状态前必须保证通道已创建。
 *
 */
FH_SINT32 FH_JPEGE_Query(JPEGE_CHN JpegeChn, JPEGE_CHN_STAT_S *pstStat);

/**
 * @brief         设置JPEG编码通道参数
 *
 * @param[in]    JpegeChn          JPEG编码通道号
 * @param[in]    pstAttr           JPEG编码通道属性指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 * @note
 *    - 通道必须已经创建。
 *
 */
FH_SINT32 FH_JPEGE_SetChnAttr(JPEGE_CHN JpegeChn, const JPEGE_CHN_ATTR_S *pstAttr);

/**
 * @brief         获取JPEG编码通道参数
 *
 * @param[in]     JpegeChn          JPEG编码通道号
 * @param[out]    pstAttr           JPEG编码通道属性指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 * @note
 *    - 通道必须已经创建。
 *
 */
FH_SINT32 FH_JPEGE_GetChnAttr(JPEGE_CHN JpegeChn, JPEGE_CHN_ATTR_S *pstAttr);

/**
 * @brief         获取JPEG编码通道的编码数据
 *
 * @param[in]    JpegeChn          JPEG编码通道号
 * @param[out]   pstStream        获取的编码数据结构指针

 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 * @note
 *    - 通道必须已经创建。
 *    - 通过改变获取图像方式标志可实现，
 *    - 阻塞方式、非阻塞方式、超时方式获取图像。
 *    - 以非阻塞方式获取解码图像，如果缓冲区内无图像，会立刻返回错误码。
 *
 */
FH_SINT32 FH_JPEGE_GetStream(JPEGE_CHN JpegeChn, JPEGE_STREAM_S *pstStream);

/**
 * @brief         释放取JPEG编码通道的编码数据
 *
 * @param[in]    JpegeChn        JPEG编码通道号
 * @param[out]   pstStream       释放的编码数据结构指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 * @note
 *    - 通道必须已经创建。
 *    - 需要与获取接口配对使用，获取的数据使用完后需立即释放。
 *    - 释放的必须是同一个通道获取的数据，且图像数据结构不可修改。
 *
*/
FH_SINT32 FH_JPEGE_ReleaseStream(JPEGE_CHN JpegeChn, JPEGE_STREAM_S *pstStream);

/**
 * @brief         向JPEG编码通发送视频帧数据
 *
 * @param[in]    JpegeChn           JPEG编码通道号
 * @param[in]    pstFrame           编码的视频帧数据结构指针
 * @param[in]    s32MilliSec        超时(无效,不起作用)
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 * @note
 *    - 通道必须已经创建。
 *    - 如果视频帧缓冲区已满，会立刻返回错误码。
 *
 */
FH_SINT32 FH_JPEGE_SendFrame(JPEGE_CHN JpegeChn, VIDEO_FRAME_INFO_S *pstFrame ,FH_SINT32 s32MilliSec);


/**
 * @brief         设置JPEG编码通最多编码的缓存帧数
 *
 * @param[in]    JpegeChn           JPEG编码通道号
 * @param[in]    u32MaxStrmCnt      最多编码的缓存帧数(默认值200)
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 * @note
 *    - 通道必须已经创建。
 *    - 如果视频帧缓冲区已满，会立刻返回错误码。
 *
 */
FH_SINT32 FH_JPEGE_SetMaxStreamCnt(JPEGE_CHN JpegeChn, FH_UINT32 u32MaxStrmCnt);

/**
 * @brief         获得JPEG编码通最多编码的缓存帧数
 *
 * @param[in]    JpegeChn           JPEG编码通道号
 * @param[out]    pu32MaxStrmCnt      最多编码的缓存帧数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 * @note
 *    - 通道必须已经创建。
 *    - 如果视频帧缓冲区已满，会立刻返回错误码。
 *
 */
FH_SINT32 FH_JPEGE_GetMaxStreamCnt(JPEGE_CHN JpegeChn, FH_UINT32 *pu32MaxStrmCnt);

/**
 * @brief         设置JPEG编码通的QP值等
 *
 * @param[in]    JpegeChn           JPEG编码通道号
 * @param[in]    pstJpegParam       JPEG编码QP值等参数的指针,u32Qfactor[0,99], 100-使用u8YQt/u8CbQt
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 * @note
 *    - 通道必须已经创建。
 *
 */
FH_SINT32 FH_JPEGE_SetJpegParam(JPEGE_CHN JpegeChn, const JPEGE_PARAM_S *pstJpegParam);

/**
 * @brief         获得JPEG编码通的QP值等
 *
 * @param[in]     JpegeChn           JPEG编码通道号
 * @param[out]    pstJpegParam       JPEG编码QP值等参数的指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 * @note
 *    - 通道必须已经创建。
 *
 */
FH_SINT32 FH_JPEGE_GetJpegParam(JPEGE_CHN JpegeChn, JPEGE_PARAM_S *pstJpegParam);

/**
 * @brief         设置JPEG编码通crop参数
 *
 * @param[in]     JpegeChn           JPEG编码通道号
 * @param[in]     pstCropCfg         JPEG编码core配置的指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 * @note
 *    - 通道必须已经创建。
 */
FH_SINT32 FH_JPEGE_SetCrop(JPEGE_CHN JpegeChn, const JPEGE_CROP_CFG_S *pstCropCfg);

/**
 * @brief         获得JPEG编码通crop参数
 *
 * @param[in]     JpegeChn           JPEG编码通道号
 * @param[out]    pstCropCfg         JPEG编码core配置的指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 * @note
 *    - 通道必须已经创建。
 */
FH_SINT32 FH_JPEGE_GetCrop(JPEGE_CHN JpegeChn, JPEGE_CROP_CFG_S *pstCropCfg);


/**
 * @brief        设置JPEG编码通rotate参数
 *
 * @param[in]     JpegeChn           JPEG编码通道号
 * @param[out]    enRotate           JPEG编码rotate配置的指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 * @note
 *    - 通道必须已经创建。
 */
FH_SINT32 FH_JPEGE_SetRotate(JPEGE_CHN JpegeChn, ROTATE_E enRotate);

/**
 * @brief        获得JPEG编码通rotate参数
 *
 * @param[in]     JpegeChn           JPEG编码通道号
 * @param[out]    penRotate           JPEG编码rotate配置的指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 * @note
 *    - 通道必须已经创建。
 */
FH_SINT32 FH_JPEGE_GetRotate(JPEGE_CHN JpegeChn, ROTATE_E *penRotate);


/**
 * @brief        获得JPEG编码通的文件句柄
 *
 * @param[in]     JpegeChn           JPEG编码通道号
 *
 * @retval  >0             成功
 * @retval  "否者"         失败
 *
 * @note
 *    - 通道必须已经创建。
 */
FH_SINT32 FH_JPEGE_GetFd(JPEGE_CHN JpegeChn);

/**
 * @brief        设置JPEG编码器码流buffer百分比阀值
 *
 * @param[in]     u32Threshold          JPEG编码buffe阀值
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 * @note
 * - 有效值(15~100),默认值是25
 * - 各通道的空闲码流buffer size大于等于宽x高x u32Threshold/100，就可以继续编码，否者停止编码
 */
FH_SINT32 FH_JPEGE_SetBufThreshod(FH_UINT32 u32Threshold);

/**
 * @brief        获得JPEG编码器码流buffer百分比阀值
 *
 * @param[out]    pu32Threshold      JPEG编码buffer阀值
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,jpege_errcode} 。
 *
 */
FH_SINT32 FH_JPEGE_GetBufThreshod(FH_UINT32 *pu32Threshold);


FH_SINT32 FH_JPEGE_SetRCAttr(JPEGE_CHN JpegeChn,const FH_VENC_RC_ATTR *pstVencrcattr);

FH_SINT32 FH_JPEGE_GetRCAttr(JPEGE_CHN JpegeChn,FH_VENC_RC_ATTR *pstVencrcattr);




FH_SINT32 FH_JPEGE_PRG(JPEGE_PRG_CFG_S *prg);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __FH_JPEGE_MPI_H__ */

