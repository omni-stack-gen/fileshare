#ifndef __FH_VGS2_MPI_H__
#define __FH_VGS2_MPI_H__
/**VGS**/

#include "types/type_def.h"
#include "fh_common.h"
#include "fh_vgs2_mpipara.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */


/**
 * @brief         模块初始化
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs2_errcode} 。
 *
 * @note
 *    - 当vpu内部使用到vgs2模块时(比如vpu扩展通道实现旋转或缩放)，
 *    - 且在此之前没有调用到任何vgs2的接口时，需要显示调用此接口进行模块初始化。
 *
 */
FH_SINT32 FH_VGS_V2_Init(FH_VOID);

/**
 * @brief         创建图像马赛克遮挡任务
 *
 * @param[out]    handle             VGS任务句柄
 * @param[in]     ctrl               遮挡任务控制参数
 * @param[in]     instant            接口是否即时返回
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs2_errcode} 。
 *
 * 参数instant为0时表明接口同步处理，返回时数据已处理完成。
 *
 */
FH_SINT32 FH_VGS_V2_PM(VGS2_HANDLE *handle,VGS_V2_PM_CTRL *ctrl,FH_BOOL instant);


/**
 * @brief         创建YUV格式转换任务
 *
 * @param[out]    handle             VGS任务句柄
 * @param[in]     ctrl               转换任务控制参数
 * @param[in]     instant            接口是否即时返回
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs2_errcode} 。
 *
 * 参数instant为0时表明接口同步处理，返回时数据已处理完成。
 *
 */
FH_SINT32 FH_VGS_V2_CVT(VGS2_HANDLE *handle,VGS_V2_CVT_CTRL *ctrl,FH_BOOL instant);


/**
 * @brief         创建旋转任务
 *
 * @param[out]    handle             VGS任务句柄
 * @param[in]     ctrl               旋转任务控制参数
 * @param[in]     instant            接口是否即时返回
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs2_errcode} 。
 *
 * 参数instant为0时表明接口同步处理，返回时数据已处理完成。
 *
 */
FH_SINT32 FH_VGS_V2_ROT(VGS2_HANDLE *handle,VGS_V2_ROT_CTRL *ctrl,FH_BOOL instant);

/**
 * @brief         创建图像拼接任务
 *
 * @param[out]    handle             VGS任务句柄
 * @param[in]     ctrl               拼接任务控制参数
 * @param[in]     instant            接口是否即时返回
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs2_errcode} 。
 *
 * 参数instant为0时表明接口同步处理，返回时数据已处理完成。
 *
 */
FH_SINT32 FH_VGS_V2_JNT(VGS2_HANDLE *handle,VGS_V2_JNT_CTRL *ctrl,FH_BOOL instant);

/**
 * @brief         创建缩放任务
 *
 * @param[out]    handle             VGS任务句柄
 * @param[in]     ctrl               缩放任务控制参数
 * @param[in]     instant            接口是否即时返回
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs2_errcode} 。
 *
 * 参数instant为0时表明接口同步处理，返回时数据已处理完成。
 *
 */
FH_SINT32 FH_VGS_V2_SCL(VGS2_HANDLE *handle,VGS_V2_SCL_CTRL *ctrl,FH_BOOL instant);

/**
 * @brief         创建腐蚀膨胀任务
 *
 * @param[out]    handle             VGS任务句柄
 * @param[in]     ctrl               腐蚀膨胀任务控制参数
 * @param[in]     instant            接口是否即时返回
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs2_errcode} 。
 *
 * 参数instant为0时表明接口同步处理，返回时数据已处理完成。
 *
 */
FH_SINT32 FH_VGS_V2_CNE(VGS2_HANDLE *handle,VGS_V2_CNE_CTRL *ctrl,FH_BOOL instant);

/**
 * @brief         创建积分图任务
 *
 * @param[out]    handle             VGS任务句柄
 * @param[in]     ctrl               积分图任务控制参数
 * @param[in]     instant            接口是否即时返回
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs2_errcode} 。
 *
 * 参数instant为0时表明接口同步处理，返回时数据已处理完成。
 *
 */
FH_SINT32 FH_VGS_V2_INTG(VGS2_HANDLE *handle,VGS_V2_INTG_CTRL *ctrl,FH_BOOL instant);

/**
 * @brief         创建加减运算任务
 *
 * @param[out]    handle             VGS任务句柄
 * @param[in]     ctrl               加减运算任务控制参数
 * @param[in]     instant            接口是否即时返回
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs2_errcode} 。
 *
 * 参数instant为0时表明接口同步处理，返回时数据已处理完成。
 *
 */
FH_SINT32 FH_VGS_V2_ADD(VGS2_HANDLE *handle,VGS_V2_ADD_CTRL *ctrl,FH_BOOL instant);


/**
 * @brief         创建图像融合任务
 *
 * @param[out]    handle             VGS任务句柄
 * @param[in]     ctrl               融合任务控制参数
 * @param[in]     instant            接口是否即时返回
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs2_errcode} 。
 *
 * 参数instant为0时表明接口同步处理，返回时数据已处理完成。
 *
 */
FH_SINT32 FH_VGS_V2_MRG(VGS2_HANDLE *handle,VGS_V2_MRG_CTRL *ctrl,FH_BOOL instant);

/**
 * @brief         创建OSD格式转换任务
 *
 * @param[out]    handle             VGS任务句柄
 * @param[in]     ctrl               转换任务控制参数
 * @param[in]     instant            接口是否即时返回
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs2_errcode} 。
 *
 * 参数instant为0时表明接口同步处理，返回时数据已处理完成。
 *
 */
FH_SINT32 FH_VGS_V2_OSD_CVT(VGS2_HANDLE *handle,VGS_V2_OSD_CVT_CTRL *ctrl,FH_BOOL instant);


/**
 * @brief         创建OSD叠加任务
 *
 * @param[out]    handle             VGS任务句柄
 * @param[in]     ctrl               OSD叠加任务控制参数
 * @param[in]     instant            接口是否即时返回
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs2_errcode} 。
 *
 * 参数instant为0时表明接口同步处理，返回时数据已处理完成。
 *
 */
FH_SINT32 FH_VGS_V2_OSD_OL(VGS2_HANDLE *handle,VGS_V2_OSD_OL_CTRL *ctrl,FH_BOOL instant);

/**
 * @brief         查询VGS2(VGS)任务是否完成
 *
 * @param[in]     handle             VGS任务句柄
 * @param[out]    isfinish           VGS任务是否完成标志
 * @param[in]     isblock            查询接口是否阻塞标志
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs2_errcode} 。
 *
 * @note
 *    - 当句柄中硬件模块标记相同时，如果任务标记大的任务已完成，代表比其小的任务也都已经完成。
 *    - 硬件模块标记不同时，两个任务标记相互独立，不能推导出另外不同模块标记之间任务是否完成。
 *
 */
FH_SINT32 FH_VGS_Query(VGS2_HANDLE handle,FH_BOOL *isfinish,FH_BOOL isblock);

/**
 * @brief         初始化VGS2(VGS)
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs2_errcode} 。
 *
 *
 */
FH_SINT32 FH_VGS_MEM_INIT(FH_VOID);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
#endif /*__FH_VGS2_MPI_H__ */
