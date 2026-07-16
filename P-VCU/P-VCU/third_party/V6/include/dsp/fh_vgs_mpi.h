#ifndef __FH_VGS_MPI_H__
#define __FH_VGS_MPI_H__
/**VGS**/
#include "fh_vgs_mpipara.h"
#include "fh_tde_mpipara.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


/**
 * @brief         打开VGS(VPPU)设备
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 在进行VGS操作前应先调用此接口，保证VGS设备处于打开状态；
 *    - 此接口未主动调用时，调用其他非关闭接口，也会默认将设备打开；
 *    - 打开设备时会同时打开模块时钟，驱动加载后默认时钟为打开状态。
 *
 */
FH_SINT32 FH_VGS_Open(FH_VOID);

/**
 * @brief         关闭VGS(VPPU)设备
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 有一次FH_VGS_Open操作，就需要调一次本接口；
 *    - 在最后一次调用时实际关闭设备；
 *    - 关闭设备时会同时关闭模块时钟；
 *    - 若需要在驱动加载后直接关闭模块时钟，需先open再close。
 *
 */
FH_VOID FH_VGS_Close(FH_VOID);

/**
 * @brief         配置vgs模块参数
 *
 * @param[in]    pstModuleParam      模块参数结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"           失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *   - 需在驱动加载完后最早调用，且不支持动态设置；
 *   - 如果最初的vgsbuf和rotbuf都为0，后续的任意时刻在需要时可以设置一次。
 *
 */
FH_SINT32 FH_VGS_SetModuleParam(VGS_MODULE_PARAM_S *pstModuleParam);

/**
 * @brief         创建VGS任务
 *
 * @param[out]    phHandle           任务句柄指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 可以创建多个任务，但必须判断此接口返回成功后才能使用创建出的任务句柄；
 *    - phHandle 不能为空指针或非法指针。
 *
 */
FH_SINT32 FH_VGS_BeginJob(VGS_HANDLE *phHandle);

/**
 * @brief         提交任务，开始处理添加到该任务的操作。
 *
 * @param[in]     hHandle            任务句柄
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 句柄标识的任务必须是已经创建的任务。
 *
 */
FH_SINT32 FH_VGS_EndJob(VGS_HANDLE hHandle);

/**
 * @brief         取消指定的任务。
 *
 * @param[in]     hHandle            任务句柄
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 句柄标识的任务必须是已经创建的任务；
 *    - 已经提交的任务不能再取消；
 *    - 取消后的任务不再有效，不能再向其添加操作，也不能提交该任务；
 *    - 当添加操作出错时，必须调用此接口进行取消。
 *
 */
FH_SINT32 FH_VGS_CancelJob(VGS_HANDLE hHandle);

/**
 * @brief         向指定任务添加缩放操作
 *
 * @param[in]     hHandle            任务句柄
 * @param[in]     pstTask            操作属性指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 句柄标识的任务必须是已经创建的任务；
 *    - 若返回失败，必须调用FH_VGS_CancelJob取消任务。
 *
 */
FH_SINT32 FH_VGS_AddScaleTask(VGS_HANDLE hHandle, VGS_TASK_ATTR_S *pstTask);

/**
 * @brief         向指定任务添加打OSD操作
 *
 * @param[in]     hHandle            任务句柄
 * @param[in]     pstTask            操作属性指针
 * @param[in]     astVgsAddOsd       OSD属性结构体数组
 * @param[in]     u32ArraySize       OSD属性数组成员数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 句柄标识的任务必须是已经创建的任务；
 *    - 若返回失败，必须调用FH_VGS_CancelJob取消任务；
 *    - OSD属性数组成员数必须与叠osd的数目相同；
 *    - 最多一次可以叠10个Osd，超过部分无效；
 *    - OSD格式只支持ARGB1555,ARGB4444和ARGB8888三种。
 *
 */
FH_SINT32 FH_VGS_AddOsdTask(VGS_HANDLE hHandle, VGS_TASK_ATTR_S *pstTask, VGS_OSD_S astVgsAddOsd[], FH_UINT32 u32ArraySize);

/**
 * @brief         向指定任务添加打混合操作
 *
 * @param[in]     hHandle            任务句柄
 * @param[in]     pstImgA            上层(above)图像结构指针
 * @param[in]     pstImgB            下层(below)图像结构指针
 * @param[in]     pstImgOut          输出图像结构指针
 * @param[in]     pstOpt             混合选项结构指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 句柄标识的任务必须是已经创建的任务；
 *    - 若返回失败，必须调用 #FH_VGS_CancelJob 取消任务；
 *    - 输入图像可以进行crop，crop信息记录在video frame结构的4个offset参数中；
 *    - 下层图像可以进行4倍以内的缩放，缩放前为crop后的大小，缩放后为输出图像的大小,先缩放再混合；
 *    - 上层图像可以进行crop，但不支持缩放，实际混合的大小即为crop后的大小；
 *    - 上下层图像的globle alpha需记录在VGS_OPT_S结构中，该结构还可配置背景信息；
 *    - 图像实际参与混合的最大区域大小，上层为2048x2046，下层为4096x4094；
 *    - 输入输出图像均只支持sp420和sp422两种格式。
 *
 */
FH_SINT32 FH_VGS_AddBlendTask(VGS_HANDLE hHandle, VIDEO_FRAME_S *pstImgA, VIDEO_FRAME_S *pstImgB, VIDEO_FRAME_S *pstImgOut, VGS_OPT_S *pstOpt);

/**
 * @brief         向指定任务添加图像拼接任务
 *
 * @param[in]     hHandle            任务句柄
 * @param[in]     pstVFrmIn          原始图像帧结构指针
 * @param[in]     pstInRect          原始图像中用于拼接的区域
 * @param[in]     pstVFrmOut         目标图像帧结构指针
 * @param[in]     pstOutRect         拼接到目标图像的位置区域
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 句柄标识的任务必须是已经创建的任务；
 *    - 若返回失败，必须调用FH_VGS_CancelJob取消任务。
 *
 */
FH_SINT32 FH_VGS_AddCombineTask(VGS_HANDLE hHandle, VIDEO_FRAME_S *pstVFrmIn, RECT_S *pstInRect, VIDEO_FRAME_S *pstVFrmOut, RECT_S *pstOutRect);

/**
 * @brief         向指定任务添加格式转换操作
 *
 * @param[in]     hHandle            任务句柄
 * @param[in]     pstFrmIn           原始图像结构指针
 * @param[in]     pstFrmOut          目标图像结构指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 句柄标识的任务必须是已经创建的任务；
 *    - 若返回失败，必须调用FH_VGS_CancelJob取消任务；
 *    - 支持nv12/nv16/argb1555/argb4444/argb8888格式间的互转；
 *    - 输入另外还单独支持422 package格式。
 *
 */
FH_SINT32 FH_VGS_AddFmtConvertTask(VGS_HANDLE hHandle, VIDEO_FRAME_S* pstFrmIn, VIDEO_FRAME_S* pstFrmOut);

/**
 * @brief         设置单帧任务的缩放模式
 *
 * @param[in]     enScaleMode        缩放模式枚举值
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 设置之后的单帧任务都将使用此缩放模式。
 *
 */
FH_SINT32  FH_VGS_SetTaskScaleMode(VGS_SCALE_MODE_E enScaleMode);

/**
 * @brief         单帧拷贝操作
 *
 * @param[in]     pstTask            操作属性指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 该接口不支持多cmd方式，没有句柄；
 *    - 设置VGS_TASK_ATTR_S.reserved可以完成弧角拷贝，该值指定的为弧角半径；
 *    - 支持nv12/nv16/yuv400/yuv package/argb格式的拷贝；
 *    - 设置格式为PIXEL_FMT_RGB_8BPP时，宽x高结果作为Byte数拷贝，
 *    - 原始图像及目标图像接口中的u32PrivateData字段作为返回值，分别记录设置的拷贝长度及实际的拷贝长度。
 *
 */
FH_SINT32  FH_VGS_DoCopy(VGS_TASK_ATTR_S *pstTask);

/**
 * @brief         单帧打Cover操作
 *
 * @param[in]     pstTask            操作属性指针
 * @param[in]     astVgsAddCover     Cover属性结构体数组
 * @param[in]     u32ArraySize       Cover属性数组成员数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 该接口不支持多cmd方式，没有句柄；
 *    - Cover属性数组成员数必须与Cover数目相同；
 *    - 最多一次可以画10个Cover，超过部分无效；
 *    - 只支持矩形Cover,不支持任意四边形的Cover,Cover颜色为RGB888形式。
 *
 */
FH_SINT32 FH_VGS_DoCover(VGS_TASK_ATTR_S *pstTask, VGS_COVER_S astVgsAddCover[], FH_UINT32 u32ArraySize);

/**
 * @brief         单帧画线操作
 *
 * @param[in]     pstTask            操作属性指针
 * @param[in]     astVgsDrawLine     画线属性结构体数组
 * @param[in]     u32ArraySize       画线属性数组成员数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 该接口不支持多cmd方式，没有句柄；
 *    - 线条直接画在目标图像上，传入的颜色值为YUV值；
 *    - 画线属性数组成员数必须与画线的数目相同；
 *    - 最多一次可以画16条线，超过部分无效。
 *
 */
FH_SINT32 FH_VGS_DoDrawLine(VGS_TASK_ATTR_S *pstTask, VGS_LINE_S astVgsDrawLine[], FH_UINT32 u32ArraySize);

/**
 * @brief         向指定任务添加画角框操作
 *
 * @param[in]     pstTask            操作属性指针
 * @param[in]     astConerRect       角框属性结构体数组
 * @param[in]     u32ArraySize       角框属性结构体数组成员数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 该接口不支持多cmd方式，没有句柄；
 *    - 若返回失败，必须调用FH_VGS_CancelJob取消任务；
 *    - 角框直接画在目标图像上，传入的颜色值为YUV值；
 *    - 角框属性数组成员数必须与角标的数目相同；
 *    - 最多可以一次画24个角框，超过部分无效；
 *    - 角框起始位置的坐标以及角框的宽高均需要2对齐；
 *    - 线宽范围[2, 16]，需2对齐。水平线长不大于框的宽度，竖直线长不大于框的高度；
 *    - 角框需在图像范围内，超出图像范围时将自动调整到图像之内；
 *    - 当水平线长大于框宽的一半且垂直线长大于框高的一半时，画出的即为线框；
 *
 */
FH_SINT32 FH_VGS_DoConerRectTask(VGS_TASK_ATTR_S *pstTask, VGS_CONER_RECT_S astConerRect[], FH_UINT32 u32ArraySize);

/**
 * @brief         单帧填色操作
 *
 * @param[in]     pstVFrm            图像帧结构指针
 * @param[in]     pstRect            图像的待填充区域
 * @param[in]     u32RGB             填充颜色的rgb888值
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 该接口不支持多cmd方式，没有句柄;
 *    - 实际填充的是YUV数据，内部会将RGB888转成YUV处理。
 *
 */
FH_SINT32 FH_VGS_DoFillColor(VIDEO_FRAME_INFO_S *pstVFrm, RECT_S *pstRect, FH_UINT32 u32RGB);

/**
 * @brief         单帧旋转操作
 *
 * @param[in]     pstTask            操作属性指针
 * @param[in]     enRotate           旋转角度
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 原始图像宽高需要8像素对齐。
 *    - 旋转不支支持stride，也不支持crop。
 *    - 该接口不支持多cmd方式，没有句柄。
 *    - 支持nv12/nv16/argb1555/argb4444/argb8888格式的旋转。
 *
 */
FH_SINT32 FH_VGS_DoRotate(VGS_TASK_ATTR_S *pstTask, VGS_ROTATE_E enRotate);

/**
 * @brief         创建VGS通道
 *
 * @param[in]     VgsChn             需创建的通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 *
 */
FH_SINT32 FH_VGS_CreateChn(VGS_CHN VgsChn);

/**
 * @brief         销毁VGS通道
 *
 * @param[in]     VgsChn             需销毁的通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 *
 */
FH_SINT32 FH_VGS_DestroyChn(VGS_CHN VgsChn);

/**
 * @brief         设置VGS与前级的处理模式
 *
 * @param[in]     VgsChn             VGS通道号
 * @param[in]     bAsync             与解码器的异步标志
 * @param[in]     u32BufDepth        VGS缓存队列长度
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 默认为异步方式，需要同步时只需将bAsync标志置为false；
 *    - 缓存队列长度默认为9，队列长度不小于前级总buffer数，就不会在vgs产生丢帧；
 *    - 此接口需在通道创建后，实际收到数据前调用。
 *
 */
FH_SINT32 FH_VGS_SetChnSyncMode(VGS_CHN VgsChn, FH_BOOL bAsync, FH_UINT32 u32BufDepth);

/**
 * @brief         设置VGS特定路径的旋转属性
 *
 * @param[in]     VgsChn             VGS通道号
 * @param[in]     VgsPth             VGS路径号
 * @param[in]     enRotate           旋转角度
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 *
 */
FH_SINT32 FH_VGS_SetRotate(VGS_CHN VgsChn, VGS_PTH VgsPth, VGS_ROTATE_E enRotate);

/**
 * @brief         设置VGS特定路径的模式属性
 *
 * @param[in]     VgsChn             VGS通道号
 * @param[in]     VgsPth             VGS路径号
 * @param[in]     pstVgsMode         路径模式结构指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 模式属性和VGS通道的输出buf直接相关，只有在后级为vou时才支持auto模式；
 *    - 若该ptah后续还需要做旋转，则user模式的宽高为旋转前的宽高；
 *    - vgs的绑定操作，需要在此接口调用后进行，否则会出现绑定报错；
 *
 */
FH_SINT32 FH_VGS_SetPthMode(VGS_CHN VgsChn, VGS_PTH VgsPth, VGS_PTH_PARA_S *pstVgsMode);


/**
 * @brief         获取VGS特定路径的模式属性
 *
 * @param[in]     VgsChn             VGS通道号
 * @param[in]     VgsPth             VGS路径号
 * @param[out]    pstVgsMode         路径模式结构指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 *
 */
FH_SINT32 FH_VGS_GetPthMode(VGS_CHN VgsChn, VGS_PTH VgsPth, VGS_PTH_PARA_S *pstVgsMode);

/**
 * @brief         设置VGS通道裁剪属性
 *
 * @param[in]     VgsChn             VGS通道号
 * @param[in]     pstCropInfo        裁剪信息结构指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 支持动态的设置裁剪属性。
 *
 */
FH_SINT32 FH_VGS_SetChnCrop(VGS_CHN VgsChn, VGS_CROP_PARA_S *pstCropInfo);

/**
 * @brief         获取VGS通道裁剪属性
 *
 * @param[in]     VgsChn             VGS通道号
 * @param[out]    pstCropInfo        裁剪信息结构指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 *
 */
FH_SINT32 FH_VGS_GetChnCrop(VGS_CHN VgsChn, VGS_CROP_PARA_S *pstCropInfo);

/**
 * @brief         设置输出路径的有效区域
 *
 * @param[in]     VgsChn             VGS通道号
 * @param[in]     VgsPth             VGS路径号
 * @param[in]     pstArea            有效区域结构指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 支持动态的设置有效区域属性；
 *    - 有效区域设置后，对应path输出会缩放到有效区域的位置及尺寸；
 *    - 设置后输出buf仍为FH_VGS_SetPthMode设置的buf大小；
 *    - 有效区域的设置只在PthMode为user模式时有效，auto模式时的有效区域只取决于后级模块。
 *
 */
FH_SINT32 FH_VGS_SetPthArea(VGS_CHN VgsChn, VGS_PTH VgsPth, VGS_VALID_AREA_S *pstArea);

/**
 * @brief         获取路径的裁剪属性
 *
 * @param[in]     VgsChn             VGS通道号
 * @param[in]     VgsPth             VGS路径号
 * @param[out]    pstArea            有效区域结构指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 */
FH_SINT32 FH_VGS_GetPthArea(VGS_CHN VgsChn, VGS_PTH VgsPth, VGS_VALID_AREA_S *pstArea);

/**
 * @brief         设置路径的缩放模式
 *
 * @param[in]     VgsChn             VGS通道号
 * @param[in]     VgsPth             VGS路径号
 * @param[in]     enScaleMode        缩放模式枚举值
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 只对对应的通道的路径生效。
 *    - 只支持pth 0和1设置，pth2不支持缩放故不支持。(MC635x只支持pth 0)
 *
 */
FH_SINT32 FH_VGS_SetPthScaleMode(VGS_CHN VgsChn, VGS_PTH VgsPth, VGS_SCALE_MODE_E enScaleMode);

/**
 * @brief         向VGS指定通道发送图像数据
 *
 * @param[in]     VgsChn             VGS通道号
 * @param[in]     pstVFrm            发送的图像结构指针
 * @param[in]     bPlayMode          播放模式
 * @param[in]     u32TimeOut         获取输出buffer的延时
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 播放模式为FALSE时，若获取输出buffer失败，会直接做丢帧处理。
 *
 */
FH_SINT32 FH_VGS_SendFrame(VGS_CHN VgsChn, const VIDEO_FRAME_INFO_S *pstVFrm, FH_BOOL bPlayMode, FH_UINT32 u32TimeOut);

/**
 * @brief         获取VGS指定路径的下一帧输出图像
 *
 * @param[in]     VgsChn             VGS通道号
 * @param[in]     VgsPth             VGS路径号
 * @param[out]    pstVideoFrame      输出图像帧结构指针
 * @param[in]     s32MilliSec        获取图像的延时
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 该帧获取后会占住对应的buffer，当不再需要时，
 *    - 一定要配合FH_VGS_ReleasePthFrame接口进行释放。
 *
 */
FH_SINT32 FH_VGS_GetPthFrame(VGS_CHN VgsChn, VGS_PTH VgsPth, VIDEO_FRAME_INFO_S *pstVideoFrame, FH_SINT32 s32MilliSec);

/**
 * @brief         释放获取到的一帧图像
 *
 * @param[in]     VgsChn             VGS通道号
 * @param[in]     VgsPth             VGS路径号
 * @param[in]     pstVideoFrame      释放的图像帧结构指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 *
 */
FH_SINT32 FH_VGS_ReleasePthFrame(VGS_CHN VgsChn, VGS_PTH VgsPth, VIDEO_FRAME_INFO_S *pstVideoFrame);

/**
 * @brief         查询VGS指定通道的数据处理情况
 *
 * @param[in]     VgsChn             VGS通道号
 * @param[out]    pstChnSts          处理信息结构指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 *
 */
FH_SINT32 FH_VGS_ChnQuery(VGS_CHN VgsChn, VGS_CHN_STS_S *pstChnSts);

/**
 * @brief         检查VGS输入buffer是否还在使用
 *
 * @param[in]     chan               VGS通道号
 * @param[in]     addr               待查询地址
 * @param[in]     size               查询大小
 * @param[in]     timeout_ms         超时时间(ms)
 * @param[out]    isidle             是否正在被使用
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 *
 */
FH_SINT32 FH_VGS_CheckAddr(FH_UINT32 chan, FH_PHYADDR addr, FH_UINT32 size, FH_UINT32 timeout_ms, FH_UINT32 *isidle);

/**
 * @brief         视频冻结,之后将不断发送最后一帧数据给到后级，直到调用FH_VGS_UnfreezeVideo解冻。
 *
 * @param[in]     VgsChn             VGS通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 此功能开启后会默认开启备份帧功能，用于存放缓存数据；
 *    - 在FH_VGS_UnfreezeVideo或者通道销毁后会关闭备份帧。
 *
 */
FH_SINT32 FH_VGS_FreezeVideo(FH_UINT32 VgsChn);

/**
 * @brief         视频解冻。
 *
 * @param[in]     VgsChn               VGS通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 此接口调用后会关闭冻结时开启的备份帧功能。
 *
 */
FH_SINT32 FH_VGS_UnfreezeVideo(FH_UINT32 VgsChn);

/**
 * @brief         设置VGS指定路径的缓存深度
 *
 * @param[in]     VgsChn             VGS通道号
 * @param[in]     VgsPth             VGS路径号
 * @param[in]     u32PthDepth        需设置的路径缓存深度
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 此接口调用时需保证通道已经创建成功；
 *    - 最大缓存深度为3，当超过3个时会按3设置，
 *    - 深度>0时只能用FH_VGS_GetPthFrmEx获取数据；
 *    - 若深度设为0，则相当于不做输出缓存,只能用FH_VGS_GetPthFrame获取数据。
 *
 */
FH_SINT32 FH_VGS_SetPthDepth(VGS_CHN VgsChn, VGS_PTH VgsPth, FH_UINT32 u32PthDepth);

/**
 * @brief         获取VGS指定路径的缓存深度
 *
 * @param[in]     VgsChn             VGS通道号
 * @param[in]     VgsPth             VGS路径号
 * @param[out]    pPthDepth          当前的路径缓存深度指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 *
 */
FH_SINT32 FH_VGS_GetPthDepth(VGS_CHN VgsChn, VGS_PTH VgsPth, FH_UINT32 *pPthDepth);

/**
 * @brief         获取VGS指定路径的输出图像
 *
 * @param[in]     VgsChn             VGS通道号
 * @param[in]     VgsPth             VGS路径号
 * @param[out]    pstVideoFrame      输出图像帧结构指针
 * @param[in]     s32MilliSec        获取图像的延时
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vgs_errcode} 。
 *
 * @note
 *    - 此接口调用前需通过FH_VGS_SetPthDepth设置缓存深度；
 *    - 此接口不可与FH_VGS_GetPthFrame同时使用；
 *    - 该帧获取后会占住对应的buffer，当不再需要时，
 *    - 一定要配合FH_VGS_ReleasePthFrame接口进行释放；
 *    - 此接口只有在后级没有绑定模块时可以使用。
 *
 */

FH_SINT32 FH_VGS_GetPthFrmEx(VGS_CHN VgsChn, VGS_PTH VgsPth, VIDEO_FRAME_INFO_S *pstVideoFrame, FH_SINT32 s32MilliSec);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __FH_VGS_MPI_H__ */

