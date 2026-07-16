#ifndef __FH_VENC_MPI_H__
#define __FH_VENC_MPI_H__
/**VENC**/
#include "fh_venc_mpipara.h"
#include "types/type_def.h"
#include "fh_common.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */


/**
 * @brief         编码通道初始化
 *
 * @param[in]    pstInitAttr          编码通道初始化参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 */
FH_SINT32 FH_VENC_Init(FH_VENC_DTS_CLK_ATTR *pstInitAttr);

/**
 * @brief         编码通道去初始化
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 */
FH_SINT32 FH_VENC_DeInit(void);

/**
 * @brief         编码通道exit
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,veu_errcode} 。
 *
 * @note
 * - 仅在linux上支持，RT-Thread上不支持
 */
FH_SINT32 FH_VENC_Exit(void);

/**
 * @brief         创建编码通道，指定通道支持的最大编码幅面，和编码格式。
 *
 * @param[in]    chan            通道号
 * @param[in]    stVencChnAttr   通道属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *    - 需要按需要处理的最大能力创建，创建之后无法销毁。
 *    - 可以指明一个通道同时支持多种不同的编码类型。
 *    - 支持多种类型，分配内存时会按各类型种最大的一块内存进行内存分配，在设置通道属性时将这块内存给相应驱动使用。
 *
 */
FH_SINT32 FH_VENC_CreateChn(FH_UINT32 chan,const FH_VENC_CHN_CAP *stVencChnAttr);

/**
 * @brief         销毁编码通道
 *
 * @param[in]    chan          通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *    - 需要VMM支持释放后才支持此接口调用。
 */
FH_SINT32 FH_VENC_DestroyChn(FH_UINT32 chan);

/**
 * @brief         设置编码通道属性
 *
 * @param[in]    chan             通道号
 * @param[in]    pstVencChnAttr   编码通道属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 */
FH_SINT32 FH_VENC_SetChnAttr(FH_UINT32 chan,const FH_VENC_CHN_CONFIG *pstVencChnAttr);

/**
 * @brief         设置编码Online模式属性
 *
 * @param[in]     pstVencSetAttr     编码通道属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,veu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VENC_SetOnlineModeAttr(const FH_VENC_SET_CHN_ATTR *pstVencSetAttr);

/**
* @brief         获取编码通道属性
*
* @param[in]    chan            通道号
* @param[out]   pstVencChnAttr  编码通道属性
*
* @retval  0      成功
* @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode}。
*
*/
FH_SINT32 FH_VENC_GetChnAttr(FH_UINT32 chan, FH_VENC_CHN_CONFIG *pstVencChnAttr);

/**
* @brief         通道接收图像数据
*
* @param[in]  chan   通道号
*
* @retval  0      成功
* @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode}。
* @note
*    - 通道为JPEG抓图模式时，不需要调用 #FH_VENC_StartRecvPic / #FH_VENC_StopRecvPic 。
*/
FH_SINT32 FH_VENC_StartRecvPic(FH_UINT32 chan);

/**
* @brief         通道停止接收图像数据
*
* @param[in]  chan   通道号
*
* @retval  0      成功
* @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode}。
* @note
*    - 通道为JPEG抓图模式时，不需要调用 #FH_VENC_StartRecvPic / #FH_VENC_StopRecvPic 。
*/
FH_SINT32 FH_VENC_StopRecvPic(FH_UINT32 chan);

/*弃用接口,请调用FH_VENC_Submit_ENC_Ex*/
FH_SINT32 FH_VENC_Submit_ENC(FH_UINT32 chan,const FH_ENC_FRAME *pstVencsubmitframe);


/**
 * @brief         向编码通道提交图像数据进行编码。
 *
 * @param[in]    chan                   通道号
 * @param[in]    pstVencsubmitframe     图像数据信息
 * @param[in]    datamode               图像数据格式
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *    - 调用此接口时通道不能处于绑定状态。
 */
FH_SINT32 FH_VENC_Submit_ENC_Ex(FH_UINT32 chan,const FH_ENC_FRAME *pstVencsubmitframe,FH_VPU_VO_MODE datamode);

/**
 * @brief         向编码通道提交图像数据进行编码
 *
 * @param[in]    chan                通道号
 * @param[in]    pstVencsendframe    图像数据信息
 * @param[in]    timeout_ms          超时时间,如提交数据时ENC输入队列满,会阻塞到队列空完成入队.此超时仅作为入队成功的阻塞，不代表该帧编码完成.
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *    - 调用此接口时通道不能处于绑定状态。
 */
FH_SINT32 FH_VENC_SendFrame(FH_UINT32 chan,const FH_VIDEO_FRAME *pstVencsendframe,FH_UINT32 timeout_ms);

/**
 * @brief         检查VENC输入缓存地址是否被使用
 *
 * @param[in]    chan       通道号
 * @param[in]    addr       查询地址
 * @param[in]    size       查询大小
 * @param[in]    timeout_ms 接口阻塞时间(ms)
 * @param[out]   isidle     是否正在被使用
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 */
FH_SINT32 FH_VENC_CheckAddr(FH_UINT32 chan,FH_PHYADDR addr,FH_UINT32 size,FH_UINT32 timeout_ms,FH_UINT32 *isidle);

/**
 * @brief         查询通道状态信息
 *
 * @param[in]     chan               通道号
 * @param[out]    pstVencStatus      通道状态信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,veu_errcode} 。
 *
 * @note
 *    - JPEG/MJPEG通道不支持该接口
 *
 */
FH_SINT32 FH_VENC_GetChnStatus(FH_UINT32 chan, FH_CHN_STATUS *pstVencStatus);

/**
 * @brief         获取编码通道码流，非阻塞
 *
 * @param[in]    request_type        需要获取的码流类型(支持的类型见枚举FH_STREAM_TYPE，可以支持查询多种类型的码流)
 * @param[in]    pstVencstreamAttr   码流信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *    - 可以同时查询两者码流(例如FH_STREAM_JPEG|FH_STREAM_MJPEG)或直接查询所有编码模块(FH_STREAM_ALL)
 *    - 该函数查询不同类型码流可以并发处理，但同一类型不可以并发。
 *    - 例如：
 *    - 正确：线程1：查询FH_STREAM_JPEG|FH_STREAM_MJPEG，线程2：查询FH_STREAM_H264
 *    - 错误：线程1：查询FH_STREAM_JPEG|FH_STREAM_MJPEG，线程2：查询FH_STREAM_MJPEG
 *    - 注意:MJPEG/JPEG在编码码流超过分配的缓冲区大小时会返回大小为0的码流,此时依然需要调用 #FH_VENC_ReleaseStream 释放码流。
 */
FH_SINT32 FH_VENC_GetStream (FH_UINT32 request_type,FH_VENC_STREAM *pstVencstreamAttr);

/**
 * @brief         获取编码通道码流，阻塞时间1s。
 *
 * @param[in]     request_type        需要获取的码流类型(支持的类型见枚举FH_STREAM_TYPE，可以支持查询多种类型的码流)
 * @param[out]    pstVencstreamAttr   码流信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 * @note
 *    - 同FH_VENC_GetStream
 */
FH_SINT32 FH_VENC_GetStream_Block(FH_UINT32 request_type,FH_VENC_STREAM *pstVencstreamAttr);

/**
 * @brief         获取编码通道码流，自定义超时时间。
 *
 * @param[in]     request_type        需要获取的码流类型(支持的类型见枚举FH_STREAM_TYPE，可以支持查询多种类型的码流)
 * @param[out]    pstVencstreamAttr   码流信息
 * @param[in]     timeout_ms          超时时间,ms
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 * @note
 *    - 同FH_VENC_GetStream
 */
FH_SINT32 FH_VENC_GetStream_Timeout(FH_UINT32 request_type,FH_VENC_STREAM *pstVencstreamAttr,FH_UINT32 timeout_ms);

/**
 * @brief         获取指定编码通道的码流，自定义超时时间。
 *
 * @param[in]    chan                需要获取的编码通道
 * @param[out]   pstVencstreamAttr   码流信息
 * @param[in]    timeout_ms          超时时间,ms
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 * @note
 *    - 只有用户通过Proc设置编码通道使用独立输出缓存的通道才可以通过此接口获取到码流.
 *    - 设置使用独立输出缓存的通道必须使用此接口获取，不能通过 #FH_VENC_GetStream 系列获取
 *    - 释放码流依然使用 #FH_VENC_ReleaseStream
 *    - 支持并发获取不同通道，但不能并发获取同一通道
 */
FH_SINT32 FH_VENC_GetChnStream_Timeout(FH_UINT32 chan,FH_VENC_STREAM *pstVencstreamAttr,FH_UINT32 timeout_ms);

/**
 * @brief         释放码流
 *
 * @param[in]    pstVencstreamAttr   码流结构体参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 */
FH_SINT32 FH_VENC_ReleaseStream(FH_VENC_STREAM *pstVencstreamAttr);

/**
* @brief         获取编码码流缓存池信息
*
* @param[in]    chan                通道号
* @param[out]   pstVencstreamCfg    码流缓存池信息
*
* @retval  0      成功
* @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode}。
*
* @note
*    - 如果缓冲池模式为公共缓存池模式时，只有当通道设置通道属性配置编码类型后才能获取缓存池的大小和缓存帧数量。
*/
FH_SINT32 FH_VENC_GetStreamBufConfig(FH_UINT32 chan,FH_VENC_STM_CONFIG *pstVencstreamCfg);

/**
 * @brief         设置编码码流缓存池大小
 *
 * @param[in]     chan               通道号
 * @param[in]     bufSize            码流缓存池大小
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,veu_errcode} 。
 *
 * @note
 *    - 如果缓冲池模式为公共缓存池模式时，只有当通道设置通道属性配置编码类型后才能获取缓存池的大小和缓存帧数量。
 *
 */
FH_SINT32 FH_VENC_SetStreamBufSize(FH_SINT32 chan,FH_UINT32 bufSize);

/**
 * @brief         设置编码旋转属性
 *
 * @param[in]     chan               通道号
 * @param[in]     pstVencrotateinfo  旋转属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,veu_errcode} 。
 *
 * @note
 *    - 各平台支持功能有所差异,详见《视频函数开发参考手册》。
 *
 */
FH_SINT32 FH_VENC_SetRotate(FH_UINT32 chan, FH_ROTATE_OPS pstVencrotateinfo);

/**
* @brief         获取编码旋转属性
*
* @param[in]    chan               通道号
* @param[out]   pstVencrotateinfo  旋转属性
*
* @retval  0      成功
* @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode}。
*
*/
FH_SINT32 FH_VENC_GetRotate(FH_UINT32 chan, FH_ROTATE_OPS *pstVencrotateinfo);

/**
 * @brief         设置ROI Map
 *
 * @param[in]    chan           通道号
 * @param[in]    pstVencroimap  roi map参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *    - JPEG、MJPEG通道不支持此接口。
 *    - ROI通过位图的形式设置，因此不支持获取ROI配置。
 */
FH_SINT32 FH_VENC_SetRoiMap(FH_UINT32 chan,const FH_ROI_MAP *pstVencroimap);

/**
 * @brief         设置ROI属性
 *
 * @param[in]    chan            通道号
 * @param[in]    pstVencroiinfo  ROI配置
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *    - JPEG、MJPEG通道不支持此接口。
 *    - ROI通过位图的形式设置，因此不支持获取ROI配置。
 */
FH_SINT32 FH_VENC_SetRoiCfg(FH_UINT32 chan,const FH_ROI *pstVencroiinfo);

/**
 * @brief         清除之前设置的ROI配置
 *
 * @param[in]    chan            通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 * @note
 *    - JPEG、MJPEG通道不支持此接口。
 */
FH_SINT32 FH_VENC_ClearRoi(FH_UINT32 chan);

/**
 * @brief         申请强制I帧
 *
 * @param[in]    chan            通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 * @note
 *    - JPEG、MJPEG通道不支持此接口。
 */
FH_SINT32 FH_VENC_RequestIDR(FH_UINT32 chan);

/**
 * @brief         获取当前时间戳
 *
 * @param[out]    Systemcurpts   时间戳
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 */
FH_SINT32 FH_VENC_GetCurPts(FH_UINT64 *Systemcurpts);

/**
 * @brief         设置码率控制参数
 *
 * @param[in]    chan              通道号
 * @param[in]    pstVencrcattr     码率控制参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *    - 推荐用户使用此接口来更新码控参数，和 #FH_VENC_SetChnAttr 比可以减少重新绑定，设置通道属性等参数。
 */
FH_SINT32 FH_VENC_SetRCAttr(FH_UINT32 chan,const FH_VENC_RC_ATTR *pstVencrcattr);

/**
 * @brief         获取码控参数
 *
 * @param[in]    chan               通道号
 * @param[out]    pstVencrcattr     码率控制参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 */
FH_SINT32 FH_VENC_GetRCAttr(FH_UINT32 chan,FH_VENC_RC_ATTR *pstVencrcattr);

/**
 * @brief         实时调整部分码控参数，通过该接口设置时会保留之前的码率情况，不会重置码控和强制I帧
 *
 * @param[in]     chan               通道号
 * @param[in]     rcparam            码控参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 * @note
 *    - 该接口可以用作一些对码率比较敏感的应用根据应用的反馈实时进行码率调整。
 *    - 但调整时应注意尽可以的避免码率频繁震荡的来回设置，以免出现码率控制不准的情况。
 *    - MJPEG/JPEG通道不支持该接口。
 *
 */
FH_SINT32 FH_VENC_SetRcChangeParam(FH_UINT32 chan,const FH_VENC_RC_CHANGEPARAM *rcparam);

/**
 * @brief         获取码控参数
 *
 * @param[in]     chan               通道号
 * @param[in]     rcparam            码控参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 * @note
 *    - MJPEG/JPEG通道不支持该接口。
 *
 */
FH_SINT32 FH_VENC_GetRcChangeParam(FH_UINT32 chan, FH_VENC_RC_CHANGEPARAM *rcparam);

/**
 * @brief         设置丢帧策略参数
 *
 * @param[in]     chan               通道号
 * @param[in]     pstVencdropattr    丢帧策略参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *    目前支持以下3种模式的丢帧策略：
 *    1. 用户设定的主动丢帧
 *       - 作为用户自身丢帧策略的接口
 *       - 作为在YUV通道一绑多情况下,不同通道的2次帧率控制
 *    2. QP门限触发的丢帧(FixQP下不生效)
 *       - 在码控QP调整到某个值(可能未到MAXQP时),优先降低帧率来降低码率,以帧率换取图像质量
 *    3. 瞬时码率的丢帧(FixQP下不生效)
 *       - 瞬时码率超过预设峰值时进行丢帧
 *       - 用户可选择丢帧时是否按照pskip的方式编码。当非pskip编码丢帧时, 会出现时间戳的跳跃。
 *
 */
FH_SINT32 FH_VENC_SetLostFrameAttr(FH_UINT32 chan,const FH_DROP_FRAME *pstVencdropattr);

/**
 * @brief         获取丢帧策略参数
 *
 * @param[in]     chan               通道号
 * @param[out]    pstVencdropattr    丢帧策略参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,veu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VENC_GetLostFrameAttr(FH_UINT32 chan,FH_DROP_FRAME *pstVencdropattr);

/**
 * @brief         设置背景QP配置
 *
 * @param[in]     chan               通道号
 * @param[in]     bkg_delta_qp       背景QP与前景QP之间的差值
 * @param[in]     bkg_max_qp         背景QP的最大值
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *    - MJPEG/JPEG通道不支持该接口。
 *    - 在智能模式下生效.背景QP与前景QP之间的差值和背景QP的最大值,
 *    - 当码控QP大于背景QP时，在QP分配上将退化为和普通编码一致;
 */
FH_SINT32 FH_VENC_SetBkgQP(FH_UINT32 chan,FH_UINT32 bkg_delta_qp,FH_UINT32 bkg_max_qp);

/**
 * @brief         获取背景QP配置
 *
 * @param[in]     chan               通道号
 * @param[out]    bkg_delta_qp       背景QP与前景QP之间的差值
 * @param[out]    bkg_max_qp         背景QP的最大值
 *
 * @retval  0              成功
 * @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode}。
 *
 */
FH_SINT32 FH_VENC_GetBkgQP(FH_UINT32 chan,FH_UINT32 *bkg_delta_qp,FH_UINT32 *bkg_max_qp);

/**
 * @brief         配置一些编码参数
 *
 * @param[in]    chan       通道号
 * @param[in]    cmd        命令号
 * @param[in]    param      命令号对应结构体
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *    - Cmd命令不适用于所有类型编码。
 *    - 此函数主要定义一些不常用的参数设置。以减少SDK的API数量和变动。
 *    - 正常流程中不需要使用此接口,主要用户效果调试或特殊需求等。
 */
FH_SINT32 FH_VENC_SetEncParam(FH_UINT32 chan,FH_ENCPARAM_CMD cmd,const FH_ENC_PARAM_UNION *param);

/**
* @brief         获取一些编码参数
*
* @param[in]    chan          通道号
* @param[in]    cmd           命令号
* @param[out]   param         命令号对应结构体
*
* @retval  0      成功
* @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode}。
*
*/
FH_SINT32 FH_VENC_GetEncParam(FH_UINT32 chan,FH_ENCPARAM_CMD cmd,FH_ENC_PARAM_UNION *param);

/**
 * @brief         设置加密密钥种子
 *
 * @param[in]     hash_seed0         加密密钥种子(和支持人员确定)
 * @param[in]     hash_seed1         加密密钥种子(和支持人员确定)
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,veu_errcode} 。
 *
 * @note
 *    - MJPEG/JPEG通道不支持该接口。
 *    - 一般用户无需设置该接口，除非富瀚支持人员要求。
 *
 */
FH_SINT32 FH_VENC_SetEncryptSeed(FH_UINT32 hash_seed0,FH_UINT32 hash_seed1);

/**
 * @brief         设置编码跳帧参考
 *
 * @param[in]    chan       通道号
 * @param[in]    pVencrefmode       跳帧属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。\
 *
 * @note
 *    - MJPEG/JPEG通道不支持该接口。
 */
FH_SINT32 FH_VENC_SetEncRefMode(FH_UINT32 chan,const FH_VENC_REF_MODE *pVencrefmode);

/**
* @brief         获取编码跳帧参考
*
* @param[in]    chan           通道号
* @param[out]   pVencrefmode   跳帧属性
*
* @retval  0      成功
* @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode}。
*
* @note
*    - MJPEG/JPEG通道不支持该接口。
*/
FH_SINT32 FH_VENC_GetEncRefMode(FH_UINT32 chan, FH_VENC_REF_MODE *pVencrefmode);

/**
 * @brief         设置熵编码类型
 *
 * @param[in]    chan                 通道号
 * @param[in]    pstVencentropy       熵编码属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *    - MC635X不支持此接口。
 */
FH_SINT32 FH_VENC_SetH264Entropy(FH_UINT32 chan,const FH_H264_ENTROPY *pstVencentropy);

/**
* @brief         获取熵编码类型
*
* @param[in]    chan             通道号
* @param[out]   pstVencentropy   熵编码属性
*
* @retval  0      成功
* @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode}。
*
* @note
*    - MC635X不支持此接口。
*/
FH_SINT32 FH_VENC_GetH264Entropy(FH_UINT32 chan,FH_H264_ENTROPY *pstVencentropy);

/**
 * @brief         设置Deblocking参数
 *
 * @param[in]     chan               通道号
 * @param[in]     pstVencdblk        Deblocking参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,veu_errcode} 。
 *
 * @note
 *    - MC635X不支持此接口。
 */
FH_SINT32 FH_VENC_SetH264Dblk(FH_UINT32 chan,const FH_H264_DBLK *pstVencdblk);

/**
 * @brief         获取Deblocking参数
 *
 * @param[in]     chan               通道号
 * @param[out]    pstVencdblk        Deblocking参数
 *
 * @retval  0              成功
 * @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode}。
 *
 * @note
 *    - MC635X不支持此接口。
 */
FH_SINT32 FH_VENC_GetH264Dblk(FH_UINT32 chan,FH_H264_DBLK *pstVencdblk);

/**
 * @brief         设置多slice属性
 *
 * @param[in]     chan               通道号
 * @param[in]     pstVencslicesplit   多slice属性
 *
 * @retval  0              成功
 * @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode}。
 *
 * @note
 *    - 仅H264编码支持该接口。
 *    - slice数量不能超过16。
 */
FH_SINT32 FH_VENC_SetH264SliceSplit(FH_UINT32 chan,const FH_H264_SLICE_SPLIT *pstVencslicesplit);

/**
 * @brief         获取多slice属性
 *
 * @param[in]     chan               通道号
 * @param[out]    pstVencslicesplit   多slice属性
 *
 * @retval  0              成功
 * @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode}。
 *
 * @note
 *    - 仅H264编码支持该接口。
 */
FH_SINT32 FH_VENC_GetH264SliceSplit(FH_UINT32 chan,FH_H264_SLICE_SPLIT *pstVencslicesplit);

/**
 * @brief         设置帧内刷新属性
 *
 * @param[in]     chan               通道号
 * @param[in]     pstVencintrafresh   帧内刷新属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *    - 仅H264编码支持该接口。
 */
FH_SINT32 FH_VENC_SetH264IntraFresh(FH_UINT32 chan,const FH_H264_INTRA_FRESH *pstVencintrafresh);

/**
 * @brief         获取帧内刷新属性
 *
 * @param[in]     chan               通道号
 * @param[out]    pstVencintrafresh   帧内刷新属性
 *
 * @retval  0              成功
 * @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode}。
 *
 * @note
 *    - 仅H264编码支持该接口。
 */
FH_SINT32 FH_VENC_GetH264IntraFresh(FH_UINT32 chan, FH_H264_INTRA_FRESH *pstVencintrafresh);

/**
 * @brief         设置H264 VUI语法参数
 *
 * @param[in]     chan               通道号
 * @param[in]     pstVencvui         VUI语法参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 * @note
 *    - 仅H264编码支持该接口。
 *    - 设置后下一个I帧生效，建议先获取默认参数后再修改对应参数。
 *
 */
FH_SINT32 FH_VENC_SetH264VUI(FH_UINT32 chan,const FH_H264_VUI *pstVencvui);

/**
 * @brief         获取H264 VUI语法参数
 *
 * @param[in]     chan               通道号
 * @param[out]    pstVencvui         VUI语法参数
 *
 * @retval  0              成功
 * @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode}。
 *
 * @note
 *    - 仅H264编码支持该接口。
 */
FH_SINT32 FH_VENC_GetH264VUI(FH_UINT32 chan,FH_H264_VUI *pstVencvui);

/**
 * @brief         设置熵编码类型
 *
 * @param[in]     chan               通道号
 * @param[in]     pstVencentropy     熵编码属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *    - MC635X不支持此接口。
 */
FH_SINT32 FH_VENC_SetH265Entropy(FH_UINT32 chan,const FH_H265_ENTROPY *pstVencentropy);

/**
 * @brief         获取熵编码类型
 *
 * @param[in]     chan               通道号
 * @param[out]    pstVencentropy     熵编码属性
 *
 * @retval  0              成功
 * @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode}。
 *
 * @note
 *    - MC635X不支持此接口。
 */
FH_SINT32 FH_VENC_GetH265Entropy(FH_UINT32 chan,FH_H265_ENTROPY *pstVencentropy);

/**
 * @brief         设置Deblocking参数
 *
 * @param[in]     chan               通道号
 * @param[in]     pstVencdblk        Deblocking参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 * @note
 *    - MC635X不支持此接口。
 */
FH_SINT32 FH_VENC_SetH265Dblk(FH_UINT32 chan,const FH_H265_DBLK *pstVencdblk);

/**
 * @brief         获取Deblocking参数
 *
 * @param[in]     chan               通道号
 * @param[out]    pstVencdblk        Deblocking参数
 *
 * @retval  0              成功
 * @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode}。
 *
 * @note
 *    - MC635X不支持此接口。
 */
FH_SINT32 FH_VENC_GetH265Dblk(FH_UINT32 chan,FH_H265_DBLK *pstVencdblk);

/**
 * @brief         设置多slice属性
 *
 * @param[in]     chan               通道号
 * @param[in]     pstVencslicesplit   多slice属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 * @note
 *    - 仅H265编码支持该接口。
 */
FH_SINT32 FH_VENC_SetH265SliceSplit(FH_UINT32 chan,const FH_H265_SLICE_SPLIT *pstVencslicesplit);

/**
 * @brief         获取多slice属性
 *
 * @param[in]     chan               通道号
 * @param[out]    pstVencslicesplit   多slice属性
 *
 * @retval  0              成功
 * @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode}。
 *
 * @note
 *    - 仅H265编码支持该接口。
 */
FH_SINT32 FH_VENC_GetH265SliceSplit(FH_UINT32 chan,FH_H265_SLICE_SPLIT *pstVencslicesplit);

/**
 * @brief         设置帧内刷新参数
 *
 * @param[in]     chan               通道号
 * @param[in]     pstVencintrafresh   帧内刷新参数
 *
 * @retval  0              成功
 * @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode}。
 *
 * @note
 *    - 仅H265编码支持该接口。
 */
FH_SINT32 FH_VENC_SetH265IntraFresh(FH_UINT32 chan,const FH_H265_INTRA_FRESH *pstVencintrafresh);

/**
 * @brief         获取帧内刷新参数
 *
 * @param[in]     chan               通道号
 * @param[out]    pstVencintrafresh   帧内刷新参数
 *
 * @retval  0              成功
 * @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode}。
 *
 * @note
 *    - 仅H265编码支持该接口。
 *
 */
FH_SINT32 FH_VENC_GetH265IntraFresh(FH_UINT32 chan, FH_H265_INTRA_FRESH *pstVencintrafresh);

/**
 * @brief         设置H265 VUI语法参数
 *
 * @param[in]     chan               通道号
 * @param[in]     pstVencvui         VUI语法参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 * @note
 *    - 仅H265编码支持该接口。
 *    - 设置后下一个I帧生效，建议先获取默认参数后再修改对应参数。
 *
 */
FH_SINT32 FH_VENC_SetH265VUI(FH_UINT32 chan,const FH_H265_VUI *pstVencvui);

/**
 * @brief         获取H265 VUI语法参数
 *
 * @param[in]     chan               通道号
 * @param[out]    pstVencvui         VUI语法参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 * @note
 *    - 仅H265编码支持该接口。
 *
 */
FH_SINT32 FH_VENC_GetH265VUI(FH_UINT32 chan,FH_H265_VUI *pstVencvui);

/**
 * @brief         设置去呼吸效应
 *
 * @param[in]     chan               通道号
 * @param[in]     pstDeBreath        是否使能去呼吸效应功能
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 * @note
 *    - MC635X不支持此接口。
 */
FH_SINT32 FH_VENC_SetDeBreathEffect(FH_UINT32 chan,const FH_DEBREATH *pstDeBreath);

/**
 * @brief         获取去呼吸效应使能状态
 *
 * @param[in]     chan               通道号
 * @param[out]    pstDeBreath        是否使能去呼吸效应
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 * @note
 *    - MC635X不支持此接口。
 */
FH_SINT32 FH_VENC_GetDeBreathEffect(FH_UINT32 chan, FH_DEBREATH *pstDeBreath);

/**
 * @brief         清空编码yuv队列
 *
 * @param[in]    chan       通道号
 *
 * @retval  0      成功
 * @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *    - 调用此接口必须先关闭编码通道, 并且编码通道码流缓存必须工作在独立缓存模式下，否则会返回不支持．
 *
 */
FH_SINT32 FH_VENC_ClearYuvQueue(FH_UINT32 chan);


/**
 * @brief      清空编码 yuv 队列 & 输出码流队列
 *
 * @param[in]  chan  通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,veu_errcode} 。
 */
FH_SINT32 FH_VENC_ResetChn(FH_UINT32 chan);

/**
 * @brief         设置用户SEI信息,data设置为NULL或len为0时关闭SEI信息添加
 *
 * @param[in]     chan               通道号
 * @param[in]     data               用户数据指针
 * @param[in]     len                用户数据长度,最大长度65536,单位byte.
 *
 * @retval  0              成功
 * @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 * @note
 *    - 仅编码支持该接口
 *
 */
FH_SINT32 FH_VENC_InsertUserData(FH_UINT32 chan,FH_UINT8 *data,FH_UINT32 len);

/**
* @brief         设置编码通道同步状态
*
* @param[in]  chan   通道号
* @param[in]  bias   倾向性参数
*
* @retval  0      成功
* @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode}。
*
*/
FH_SINT32 FH_VENC_SetBias(FH_UINT32 chan,FH_ENC_BIAS *bias);

/**
* @brief         获取编码模式倾向性参数
*
* @param[in]    chan     通道号
* @param[out]   bias     倾向性参数
*
* @retval  0       成功
* @retval  "非0"    失败，其值参见 @namelink{错误码,venc_errcode}。
*
*/
FH_SINT32 FH_VENC_GetBias(FH_UINT32 chan,FH_ENC_BIAS *bias);

/**
 * @brief         设置默认编码参数
 *
 * @param[in]    pstDrvCfg   编码默认参数
 *
 * @retval  0      成功
 * @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode} 。
 *
 *
 */
FH_SINT32 FH_VENC_SetDrvCfg(FH_VENC_SET_DRV_CONFIG *pstDrvCfg);

/**
 * @brief         设置切换编码参数
 *
 * @param[in]     pstVencSwitchAttr               三通道编码参数
 *
 * @retval  0              成功
 * @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *   - 只适用同步编码，非同步编码无法使用
 */
FH_SINT32 FH_VENC_SwitchChnAttr(const FH_VENC_SET_SWITCH_ATTR *pstVencSwitchAttr);

/**
 * @brief         设置dump参数
 *
 * @param[in]     chan         通道号
 * @param[in]     pstDump       dump 输入(vpu/user)地址参数
 *
 * @retval  0              成功
 * @retval  "非0"            失败，其值参见 @namelink{错误码,veu_errcode} 。
 * @note
 *   - 只用于调试 debug
 */
FH_SINT32 FH_VENC_SetDumpAttr(FH_UINT32 chan,FH_INPUT_DUMP *pstDump);

/**
 * @brief         设置编码通道参数
 *
 * @param[in]    chan           通道号
 * @param[in]    pstchnparam      编码通道参数
 *
 * @retval  0      成功
 * @retval  "非0"   失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *    - 必须在未创建对应编码通道前调用此接口配置相关属性,否则配置参数无效。

 */
FH_SINT32 FH_VENC_SetChnParam(FH_UINT32 chan,const FH_VENC_CHN_PARAM *pstchnparam);

/**
 * @brief         获取编码通道参数
 *
 * @param[in]    chan           通道号
 * @param[out]    pstchnparam      编码通道参数
 *
 * @retval  0       成功
 * @retval  "非0"    失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note

 */
FH_SINT32 FH_VENC_GetChnParam(FH_UINT32 chan,FH_VENC_CHN_PARAM *pstchnparam);
/**
 * @brief         设置通道属性参数
 *
 * @param[in]     chan               通道号
 * @param[in]     pstVencChnAttrEx   输入地址宽高参数
 *
 * @retval  0              成功
 * @retval  "非0"            失败，其值参见 @namelink{错误码,veu_errcode} 。
 * @note
 *    - 只适用于Online 编码模式
 */
FH_SINT32 FH_VENC_SetChnAttr_Ex(FH_UINT32 chan,const FH_VENC_EX_CONF *pstVencChnAttrEx);

/**
 * @brief         设置编码通道同步状态
 *
 * @param[in]    chan           通道号
 * @param[in]    syncStatus      编码通道同步状态
 *
 * @retval  0       成功
 * @retval  "非0"    失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *   - 仅在RT-Thread上支持，linux上不支持
 */
FH_SINT32 FH_VENC_SetChnSyncStatus(FH_UINT32 chan,const FH_SINT32 syncStatus);

/**
 * @brief         设置编码模块属性
 *
 * @param[in]    pstModAttr      模块属性输入
 *
 * @retval  0       成功
 * @retval  "非0"    失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *   - 仅在RT-Thread上支持，linux上不支持
 */
FH_SINT32 FH_VENC_SetModParam(const FH_VENC_MOD_ATTR *pstModAttr);

/**
 * @brief         获取编码模块属性
 *
 * @param[out]    pstModAttr      模块属性输出
 *
 * @retval  0       成功
 * @retval  "非0"    失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *   - 仅在RT-Thread上支持，linux上不支持
 */
FH_SINT32 FH_VENC_GetModParam(FH_VENC_MOD_ATTR *pstModAttr);

/**
 * @brief         设置回卷配置
 *
 * @param[in]    pstRdCfg      Rounding 配置参数
 *
 * @retval  0       成功
 * @retval  "非0"    失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *   -需要在 FH_VENC_CreateChn 之前调用，否则返回错误
 */
FH_SINT32 FH_VENC_SetRoundConfig(const FH_VENC_ROUND_CONFIG *pstRdCfg);

/**
 * @brief         设置LIGHT DIFF 使能及level
 *
 * @param[in]    psltg      LIGHT 配置参数
 *
 * @retval  0       成功
 * @retval  "非0"    失败，其值参见 @namelink{错误码,venc_errcode} 。
 * @note
 *   -需要在 FH_VENC_CreateChn 之后调用，否则返回错误，接口非阻塞,仅在RT-Thread上支持，linux上不支持。
 */
FH_SINT32 FH_VENC_SetLightConfig(const FH_VENC_LIGHT_CONFIG *psltg);

/**
 * @brief         VEU进入休眠状态
 *
 * @retval  0       成功
 * @retval  "非0"    失败，其值参见错误码。
 * @note
 *   - 仅在RT-Thread上支持，linux上不支持
 */
FH_SINT32 FH_VENC_Suspend(void);

/**
 * @brief         VEU退出休眠状态，进入工作状态
 *
 * @retval  0       成功
 * @retval  "非0"    失败，其值参见错误码。
 * @note
 *   - 仅在RT-Thread上支持，linux上不支持
 */
FH_SINT32 FH_VENC_Resume(void);

/**
 * @brief         VEU是否有一帧编码正在等待中断完成。
 *
 * @retval  0       没有
 * @retval  1       正在工作。
 * @note
 *   - 仅在RT-Thread上支持，linux上不支持
 */

FH_SINT32 FH_VENC_Working(void);

/**
 * @brief         VEU进入休眠状态
 *
 * @retval  0        成功
 * @retval  "非0"     失败，其值参见错误码。
 * @note
 *   - 仅在RT-Thread上支持，linux上不支持
 */
FH_SINT32 FH_VENC_Suspend_Ex(void);

/**
 * @brief         VEU退出休眠状态，进入工作状态
 *
 * @retval  0      成功
 * @retval  "非0"   失败，其值参见错误码。
 * @note
 *   - 仅在RT-Thread上支持，linux上不支持
 */
FH_SINT32 FH_VENC_Resume_Ex(void);

/**
 * @brief         VEU 编码状态查询
 *
 * @retval  0       非工作
 * @retval  "非0"    正在工作。
 * @note
 *   - 仅在RT-Thread上支持，linux上不支持
 */
FH_SINT32 FH_VENC_Working_Ex(void);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
#endif /*__MPI_VO_H__ */



