#ifndef _FH_MPI_DSP_H_
#define _FH_MPI_DSP_H_

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include "types/type_def.h"
#include "fh_vdsp_mpipara.h"

/**
 * @brief         DSP上电
 *
 * @param[in]     enDspId            DSP ID号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdsp_errcode} 。
 *
 * @note
 *    - 在加载DSP镜像前，必须先调用此接口使DSP上电。
 */
FH_SINT32 FH_MPI_SVP_DSP_PowerOn(SVP_DSP_ID_E enDspId);

/**
 * @brief         DSP下电
 *
 * @param[in]     enDspId            DSP ID号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdsp_errcode} 。
 *
 * @note
 *    无
 */
FH_SINT32 FH_MPI_SVP_DSP_PowerOff(SVP_DSP_ID_E enDspId);

/**
 * @brief         加载DSP Bin
 *
 * @param[in]     pszBinFileName            模型文件路径
 * @param[in]     enMemType                 DSP 内存类型
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdsp_errcode} 。
 *
 * @note
 *    - 本接口在DSP上电和使能DSP核后调用。
 */
FH_SINT32 FH_MPI_SVP_DSP_LoadBin(const FH_CHAR *pszBinFileName,SVP_DSP_MEM_TYPE_E enMemType);

/**
 * @brief         使能DSP核，使其工作
 *
 * @param[in]     enDspId            DSP ID号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdsp_errcode} 。
 *
 * @note
 *    - 本接口在 #FH_MPI_SVP_DSP_PowerOn 后调用。
 */
FH_SINT32 FH_MPI_SVP_DSP_EnableCore(SVP_DSP_ID_E enDspId);

/**
 * @brief         去使能DSP核，使其停止工作
 *
 * @param[in]     enDspId            DSP ID号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdsp_errcode} 。
 *
 * @note
 *    无
 */
FH_SINT32 FH_MPI_SVP_DSP_DisableCore(SVP_DSP_ID_E enDspId);

/**
 * @brief         远程处理任务
 *
 * @param[in]     phHandle           Handle指针, 预留, 暂未使用
 * @param[in]     pstSrcImg          输入图像信息结构体指针
 * @param[in]     pstDstImg          输出图像信息结构体指针
 * @param[in]     enDspId            DSP ID号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdsp_errcode} 。
 *
 * @note
 *   - 本接口在 #FH_MPI_SVP_DSP_LoadBin 后调用。
 */
FH_SINT32 FH_MPI_SVP_DSP_RPC(SVP_DSP_HANDLE *phHandle,SVP_DSP_SRC_IMAGE_S *pstSrcImg, SVP_DSP_DST_IMAGE_S *pstDstImg, SVP_DSP_ID_E enDspId);

/**
 * @brief         远程处理任务，超时返回
 *
 * @param[in]     phHandle           Handle指针, 预留, 暂未使用
 * @param[in]     pstSrcImg          输入图像信息结构体指针
 * @param[in]     pstDstImg          输出图像信息结构体指针
 * @param[in]     enDspId            DSP ID号
 * @param[in]     u32TimeoutMs       超时时间(单位ms)
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdsp_errcode} 。
 *
 * @note
 *   - 本接口在 #FH_MPI_SVP_DSP_LoadBin 后调用。
 */
FH_SINT32 FH_MPI_SVP_DSP_RPC_Timeout(SVP_DSP_HANDLE *phHandle,SVP_DSP_SRC_IMAGE_S *pstSrcImg,
    SVP_DSP_DST_IMAGE_S *pstDstImg, SVP_DSP_ID_E enDspId, FH_UINT32 u32TimeoutMs);

/**
 * @brief         远程处理任务扩展接口
 *
 * @param[in]         phHandle           Handle指针, 预留, 暂未使用
 * @param[in,out]     pstRequest         远程任务参数结构体指针
 * @param[in]         enDspId            DSP ID号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdsp_errcode} 。
 *
 * @note
 *   - 本接口在 #FH_MPI_SVP_DSP_LoadBin 后调用。
 */
FH_SINT32 FH_MPI_SVP_DSP_RPC_Ext(SVP_DSP_HANDLE *phHandle, const SVP_DSP_REQ_PARAMS_S *pstRequest, SVP_DSP_ID_E enDspId);

/**
 * @brief         任意角度图像旋转
 *
 * @param[in]     phHandle           Handle指针, 预留, 暂未使用
 * @param[in]     pstSrcImg          输入图像信息结构体指针
 * @param[in]     pstDstImg          输出图像信息结构体指针
 * @param[in]     pMatrix            旋转矩阵参数指针
 * @param[in]     u32Size            旋转矩阵参数大小
 * @param[in]     u32TimeoutMs       超时时间(单位ms)
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdsp_errcode} 。
 *
 * @note
 *    本接口在 #FH_MPI_SVP_DSP_LoadBin 后调用。
 */
FH_SINT32 FH_MPI_SVP_DSP_PerspTrans(SVP_DSP_HANDLE *phHandle, SVP_DSP_SRC_IMAGE_S *pstSrcImg,
    SVP_DSP_DST_IMAGE_S *pstDstImg, FH_FLOAT *pMatrix, FH_UINT32 u32Size, FH_UINT32 u32TimeoutMs);

/**
 * @brief         矩形转换成梯形
 *
 * @param[in]     phHandle           Handle指针, 预留, 暂未使用
 * @param[in]     pstSrcImg          输入图像信息结构体指针
 * @param[in]     pstDstImg          输出图像信息结构体指针
 * @param[in]     pVertex            梯形顶点指针
 * @param[in]     u32Size            梯形顶点个数
 * @param[in]     u32TimeoutMs       超时时间(单位ms)
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdsp_errcode} 。
 *
 * @note
 *    本接口在 #FH_MPI_SVP_DSP_LoadBin 后调用。
 */
FH_SINT32 FH_MPI_SVP_DSP_PerspTransVertex(SVP_DSP_HANDLE *phHandle, SVP_DSP_SRC_IMAGE_S *pstSrcImg,
    SVP_DSP_DST_IMAGE_S *pstDstImg, SVP_DSP_VERTEX_S *pVertex, FH_UINT32 u32Size, FH_UINT32 u32TimeoutMs);

/**
 * @brief         计算双目图像的深度信息
 *
 * @param[in]     phHandle           Handle指针, 预留, 暂未使用
 * @param[in]     pstSrcImg          输入图像信息结构体指针
 * @param[in]     pstSrcInfo         输入图像人脸框及五点关键点信息结构体指针
 * @param[in]     pstDstImg          输出图像信息结构体指针
 * @param[out]    pstDstInfo         输出图像人脸框及五点关键点信息结构体指针
 * @param[in]     u32TimeoutMs       超时时间(单位ms)
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdsp_errcode} 。
 *
 * @note
 *    本接口在 #FH_MPI_SVP_DSP_LoadBin 后调用。
 */
FH_SINT32 FH_MPI_SVP_DSP_ProcessDepth(SVP_DSP_HANDLE *phHandle, SVP_DSP_SRC_IMAGE_S *pstSrcImg,
    SVP_DSP_BBOX_S *pstSrcInfo, SVP_DSP_DST_IMAGE_S *pstDstImg, SVP_DSP_BBOX_S *pstDstInfo, FH_UINT32 u32TimeoutMs);


/**
 * @brief         查询任务完成情况
 *
 * @param[in]     enDspId            DSP ID号
 * @param[in]     hHandle            Handle指针, 预留, 暂未使用
 * @param[out]    pbFinish           任务完成状态指针。 不能为空
 * @param[in]     bBlock             是否阻塞查询标志
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdsp_errcode} 。
 *
 * @note
 *   无
 */
FH_SINT32 FH_MPI_SVP_DSP_Query(SVP_DSP_ID_E enDspId,SVP_DSP_HANDLE hHandle,FH_BOOL *pbFinish,FH_BOOL bBlock);


/**
 * @brief         设置模型文件加载地址
 *
 * @param[in]     u32BaseAddr        模型基地址
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdsp_errcode} 。
 *
 * @note
 *   - 本接口只用于RTOS系统
 */
FH_SINT32 FH_MPI_SVP_DSP_SetSramAddr(FH_UINT32 u32BaseAddr);

/**
 * @brief         获取模型文件版本号
 *
 * @param[out]    pu32Version        版本号描述指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdsp_errcode} 。
 *
 * @note
 *    - 本接口在 #FH_MPI_SVP_DSP_LoadBin 后调用。
 */
FH_SINT32 FH_MPI_SVP_DSP_GetModelVersion(FH_UINT32 *pu32Version);

/**
 * @brief         设置视差转深度系数
 *
 * @param[in]     u32DepthCoef        视差转深度系数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vdsp_errcode} 。
 *
 * @note
 *    - 视差转深度系数在计算双目图像深度信息时使用。
 */
FH_SINT32 FH_MPI_SVP_DSP_SetDepthCoef(FH_UINT32 u32DepthCoef);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif/*_FH_MPI_DSP_H_*/

