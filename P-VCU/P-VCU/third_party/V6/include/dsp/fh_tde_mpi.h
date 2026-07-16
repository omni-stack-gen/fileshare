#ifndef __FH_TDE_MPI_H__
#define __FH_TDE_MPI_H__
/**TDE**/
#include "fh_tde_mpipara.h"

#ifdef __cplusplus
 #if __cplusplus
extern "C" {
 #endif /* __cplusplus */
#endif  /* __cplusplus */


/**
 * @brief         打开TDE(G2D)设备
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 * @note
 *    - 在进行TDE操作前应先调用此接口，保证TDE设备处于打开状态；
 *    - 允许重复调用此接口，实际打开只发生在首次调用；
 *    - 打开设备时会同时打开模块时钟，驱动加载后默认时钟为打开状态。
 *
 */
FH_SINT32 FH_TDE2_Open(FH_VOID);

/**
 * @brief         关闭TDE(G2D)设备
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 * @note
 *    - 有一次FH_TDE2_Open操作，就需要调一次本接口；
 *    - 在最后一次调用时实际关闭设备；
 *    - 关闭设备时会同时关闭模块时钟；
 *    - 若需要在驱动加载后直接关闭模块时钟，需先open再close。
 *
 */
FH_VOID FH_TDE2_Close(FH_VOID);

/**
 * @brief         配置tde模块参数
 *
 * @param[in]    pstModuleParam      模块参数结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"           失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 * @note
 *   - 需在驱动加载完后最早调用，且不支持动态设置；
 *   - 如果最初的wkbuf为0，后续的任意时刻在需要时可以设置一次。
 *
 */
FH_SINT32 FH_TDE2_SetModuleParam(TDE2_MODULE_PARAM_S *pstModuleParam);

/**
 * @brief         创建TDE任务
 *
 *
 * @retval  "非-1"         成功
 * @retval  -1            失败。
 *
 * @note
 *    - 需确保TDE设备已经被打开。
 *
 */
TDE_HANDLE FH_TDE2_BeginJob(FH_VOID);

/**
 * @brief         提交TDE任务
 *
 * @param[in]     s32Handle          任务句柄
 * @param[in]     bSync              同步标志
 * @param[in]     bBlock             阻塞标志
 * @param[in]     u32TimeOut         阻塞超时时间
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 * @note
 *    - 需确保TDE设备已经被打开;
 *    - bSync设1为同步模式，数据处理完成接口才返回;
 *    - bSync设0，bBlock设1为超时模式，超时后表明该任务已失败;
 *    - bSync设0，bBlock设0为异步模式，接口立即返回，任务是否完成需要用FH_TDE2_WaitForDone查询;
 *
 */
FH_SINT32 FH_TDE2_EndJob(TDE_HANDLE s32Handle, FH_BOOL bSync, FH_BOOL bBlock, FH_UINT32 u32TimeOut);

/**
 * @brief         取消指定的TDE任务
 *
 * @param[in]     s32Handle          任务句柄
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 * @note
 *    - 已经提交的任务不能再取消。
 *    - 当添加操作出错时，必须调用此接口取消任务。
 *    - 取消任务后，包括已加入到该任务中的操作都会无效。
 *    - 取消后的任务不再有效，不能再向其添加操作，也不能提交该任务。
 *
 */
FH_SINT32 FH_TDE2_CancelJob(TDE_HANDLE s32Handle);

/**
 * @brief         等待指定TDE任务完成
 *
 * @param[in]     s32Handle          任务句柄
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 * @note
 *    - 一般用于异步endjob后确保对应任务完成;
 *    - 此接口为同步接口，返回时数据已处理完成。
 *
 */
FH_SINT32 FH_TDE2_WaitForDone(TDE_HANDLE s32Handle);

/**
 * @brief         等待所有TDE任务的完成
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 * @note
 *    - 一般用于存在异步endjob时，确保所有任务完成;
 *    - 此接口为同步接口，返回时已提交过的任务均已处理完成。
 *
 */
FH_SINT32 FH_TDE2_WaitAllDone(FH_VOID);

/**
 * @brief         复位TDE所有状态
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 * @note
 *    - 此接口一般用于待机唤醒软硬件不匹配异常，用于复位软硬件。
 *
 */
FH_SINT32 FH_TDE2_Reset(FH_VOID);

/**
 * @brief         设置抗闪烁级别
 *
 * @param[in]     enDeflickerLevel   抗闪烁级别值
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 * @note
 *    - 默认为最高等级，等级3。
 *
 */
FH_SINT32 FH_TDE2_SetDeflickerLevel(TDE_DEFLICKER_LEVEL_E enDeflickerLevel);

/**
 * @brief         获取抗闪烁级别
 *
 * @param[out]    pDeflickerLevel    抗闪烁级别指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 *
 */
FH_SINT32 FH_TDE2_GetDeflickerLevel(TDE_DEFLICKER_LEVEL_E *pDeflickerLevel);

/**
 * @brief         设置alpha判决阈值
 *
 * @param[in]     u8ThresholdValue   Alpha判决阈值
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 * @note
 *    - 默认值为50。
 *
 */
FH_SINT32 FH_TDE2_SetAlphaThresholdValue(FH_UINT8 u8ThresholdValue);

/**
 * @brief         获取alpha判决阈值
 *
 * @param[out]    pu8ThresholdValue   Alpha判决阈值指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 *
 */
FH_SINT32 FH_TDE2_GetAlphaThresholdValue(FH_UINT8 *pu8ThresholdValue);

/**
 * @brief         设置alpha判决状态
 *
 * @param[in]     bEnAlphaThreshold   Alpha判决开关
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 * @note
 *    - 默认为打开。
 *
 */
FH_SINT32 FH_TDE2_SetAlphaThresholdState(FH_BOOL bEnAlphaThreshold);

/**
 * @brief		  设置缩放模式,即选择不同的缩放算法
 *
 * @param[in]	  enResizeMode   缩放模式枚举值
 *
 * @retval	0			   成功
 * @retval	"非0"			失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 * @note
 *	  - 设置之后的任务都将使用此缩放模式。。
 *
 */
FH_SINT32  FH_TDE2_SetResizeMode(TDE_RESIZE_MODE_E enResizeMode);

/**
 * @brief         获取alpha判决阈值
 *
 * @param[out]    p_bEnAlphaThreshold   Alpha判决状态指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 *
 */
FH_SINT32 FH_TDE2_GetAlphaThresholdState(FH_BOOL * p_bEnAlphaThreshold);

/**
 * @brief         向指定任务添加快速拷贝操作
 *
 * @param[in]     s32Handle          任务句柄
 * @param[in]     pstSrc             原始图像结构指针
 * @param[in]     pstSrcRect         原始图像搬移区域结构指针
 * @param[in]     pstDst             目标图像结构指针
 * @param[in]     pstDstRect         搬移到目标图像区域的结构指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 * @note
 *    - 快速拷贝操作不支持格式转换，原始图像和目标位图格式必须一致。
 *    - 若返回失败，必须将该任务取消。
 *    - 快速拷贝不支持缩放功能，因此原始和目标的操作区域尺寸需不一致。
 *    - 原始图像和目标图像只能二选一，剩余的需配成空。
 *
 */
FH_SINT32 FH_TDE2_QuickCopy(TDE_HANDLE s32Handle,
                                TDE2_SURFACE_S* pstSrc, TDE2_RECT_S  *pstSrcRect,
                                TDE2_SURFACE_S* pstDst, TDE2_RECT_S *pstDstRect);

/**
 * @brief         向指定任务添加快速填充操作
 *
 * @param[in]     s32Handle          任务句柄
 * @param[in]     pstDst             目标图像结构指针
 * @param[in]     pstDstRect         目标图像填充区域的结构指针
 * @param[in]     u32FillData        填充颜色值
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 * @note
 *    - 直接将填充颜色值填充在位图的指定区域内，
 *    - 故填充的颜色值必须与位图格式对应。
 *
 */
FH_SINT32 FH_TDE2_QuickFill(TDE_HANDLE s32Handle,
                                TDE2_SURFACE_S* pstDst, TDE2_RECT_S *pstDstRect,
                                FH_UINT32 u32FillData);

/**
 * @brief         向指定任务添加画角框操作
 *
 * @param[in]     s32Handle          任务句柄
 * @param[in]     pstDst             目标图像结构指针
 * @param[in]     astConerRect       角框属性结构体数组
 * @param[in]     u32ArraySize       角框属性结构体数组成员数
 *
 * @retval  0               成功
 * @retval  "非0"            失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 * @note
 *    - 角框直接画在目标图像上，传入的颜色值为ARGB值；
 *    - 角框属性数组成员数必须与角标的数目相同；
 *    - 最多可以一次画24个角框，超过部分无效；
 *    - 角框起始位置的坐标以及角框的宽高均需要2对齐；
 *    - 线宽范围[2, 16]，需2对齐。水平线长不大于框的宽度，竖直线长不大于框的高度；
 *    - 角框需在图像范围内，超出图像范围时将自动调整到图像之内；
 *    - 当水平线长大于框宽的一半且垂直线长大于框高的一半时，画出的即为线框；
 *
 */
FH_SINT32 FH_TDE2_QuickConerRect(TDE_HANDLE s32Handle,
                                TDE2_SURFACE_S* pstDst, TDE2_CONER_RECT_S astConerRect[],
                                FH_UINT32 u32ArraySize);

/**
 * @brief         向指定任务添加快速缩放操作
 *
 * @param[in]     s32Handle          任务句柄
 * @param[in]     pstSrc             原始图像结构指针
 * @param[in]     pstSrcRect         原始图像缩放区域结构指针
 * @param[in]     pstDst             目标图像结构指针
 * @param[in]     pstDstRect         缩放到目标图像区域的结构指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 * @note
 *    - 缩小倍数水平/垂直小于256倍，放大倍数水平/垂直小于256倍。
 *    - 缩放时源位图和目标位图可以为同一位图，但操作区域不能有重叠。
 *    - 如果源位图和目标位图的格式不相同，则自动进行格式转换。
 *
 */
FH_SINT32 FH_TDE2_QuickResize(TDE_HANDLE s32Handle,
                                TDE2_SURFACE_S* pstSrc, TDE2_RECT_S  *pstSrcRect,
                                TDE2_SURFACE_S* pstDst, TDE2_RECT_S  *pstDstRect);


/**
 * @brief         向指定任务添加快速抗闪烁操作
 *
 * @param[in]     s32Handle          任务句柄
 * @param[in]     pstSrc             原始图像结构指针
 * @param[in]     pstSrcRect         原始图像抗闪区域结构指针
 * @param[in]     pstDst             目标图像结构指针
 * @param[in]     pstDstRect         抗闪到目标图像区域的结构指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 * @note
 *    - 抗闪烁只按垂直方向进行滤波。
 *    - 抗闪烁源位图和目标位图可以为同一位图，但操作区域不能有重叠。
 *    - 如果指定的输入区域与输出不一致，则会进行缩放处理。
 *    - 如果源位图和目标位图的格式不相同，则会进行格式转换处理。
 *
 */
FH_SINT32 FH_TDE2_QuickDeflicker(TDE_HANDLE s32Handle,
                                TDE2_SURFACE_S* pstSrc, TDE2_RECT_S  *pstSrcRect,
                                TDE2_SURFACE_S* pstDst, TDE2_RECT_S  *pstDstRect);

/**
 * @brief         向指定任务添加带附加功能的搬移操作
 *
 * @param[in]     s32Handle          任务句柄
 * @param[in]     pstBackGround      背景图像结构指针
 * @param[in]     pstBackGroundRect   背景图像叠加区域结构指针
 * @param[in]     pstForeGround      前景图像结构指针
 * @param[in]     pstForeGroundRect   前景图像区域的结构指针
 * @param[in]     pstDst             目标图像结构指针
 * @param[in]     pstDstRect         搬移到目标图像区域的结构指针
 * @param[in]     pstOpt             操作属性结构指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 * @note
 *    - 当前景源位图与目标位图尺寸不一致时，
 *    - 如果设置了缩放则按照设定的区域进行缩放，
 *    - 否则按照设置公共区域的最小值进行裁减搬移。
 *    - 背景位图可以与目标位图为同一位图。
 *    - 启用镜像的同时也可以进行缩放。
 *    - 当只需要使用单源搬移操作时（比如只对前景图进行搬移），
 *    - 可以将背景或背景位图的结构信息和操作区域结构指针设置为空。
 *
 */
FH_SINT32 FH_TDE2_Bitblit(TDE_HANDLE s32Handle,
                                TDE2_SURFACE_S* pstBackGround, TDE2_RECT_S  *pstBackGroundRect,
                                TDE2_SURFACE_S* pstForeGround, TDE2_RECT_S  *pstForeGroundRect,
                                TDE2_SURFACE_S* pstDst, TDE2_RECT_S  *pstDstRect,
                                TDE2_OPT_S* pstOpt);

/**
 * @brief         向指定任务添加画点画线操作
 *
 * @param[in]     s32Handle          任务句柄
 * @param[in]     pstForeGround      前景图像结构指针
 * @param[in]     pstForeGroundRect   前景图像搬移区域结构指针
 * @param[in]     pstDst             目标图像结构指针
 * @param[in]     pstDstRect         搬移到目标图像区域的结构指针
 * @param[in]     pstFillColor       目标图像结构指针
 * @param[in]     pstOpt             操作属性结构指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 * @note
 *    - 可以实现在位图上画点、画线等功能。
 *    - 当前景和前景区域为空时，其实现单纯的色彩填充功能。
 *    - 当前景位图不为空时，操作属性一定不能为空。
 *
 */
FH_SINT32 FH_TDE2_SolidDraw(TDE_HANDLE s32Handle,
                                TDE2_SURFACE_S* pstForeGround, TDE2_RECT_S  *pstForeGroundRect,
                                TDE2_SURFACE_S *pstDst, TDE2_RECT_S  *pstDstRect,
                                TDE2_FILLCOLOR_S *pstFillColor, TDE2_OPT_S *pstOpt);

/**
 * @brief         向指定任务添加数据压缩操作
 *
 * @param[in]     s32Handle          任务句柄
 * @param[in]     pstSrc             原始图像结构指针
 * @param[in]     pstDst             目标图像结构指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 * @note
 *    - 只添加单纯的压缩数据操作，原始图像必须与目标图像尺寸一样。
 *    - 缺省了操作区域相关参数，操作区域是整幅位图像。
 *
 */
FH_SINT32 FH_TDE2_DoCompress(TDE_HANDLE s32Handle, TDE2_SURFACE_S* pstSrc, TDE2_SURFACE_S* pstDst);

/**
 * @brief         向指定任务添加数据解压缩操作
 *
 * @param[in]     s32Handle          任务句柄
 * @param[in]     pstSrc             原始图像结构指针
 * @param[in]     pstDst             目标图像结构指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,tde_errcode} 。
 *
 * @note
 *    - 只添加单纯的解压缩数据操作，原始图像必须与目标图像尺寸一样。
 *    - 缺省了操作区域相关参数，操作区域是整幅位图像。
 *
 */
FH_SINT32 FH_TDE2_DeCompress(TDE_HANDLE s32Handle, TDE2_SURFACE_S* pstSrc, TDE2_SURFACE_S* pstDst);




#ifdef __cplusplus
 #if __cplusplus
}
 #endif /* __cplusplus */
#endif  /* __cplusplus */

#endif  /* __FH_TDE_MPI_H__ */
