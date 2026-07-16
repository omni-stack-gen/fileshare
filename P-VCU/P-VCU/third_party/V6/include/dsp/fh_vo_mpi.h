#ifndef __FH_VO_MPI_H__
#define __FH_VO_MPI_H__
/**VOU**/

#include "fh_vo_mpipara.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/**
 * @brief         配置视频输出设备的公共属性
 *
 * @param[in]     VoDev              视频输出设备号
 * @param[in]     pstPubAttr         视频输出设备公共属性结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 视频输出设备属性为静态属性，必须在执行FH_VO_Enable前配置。
 *
 */
FH_SINT32 FH_VO_SetPubAttr(VO_DEV_ID_E VoDev, const VO_PUB_ATTR_S *pstPubAttr);

/**
 * @brief         获取视频输出设备的公共属性
 *
 * @param[in]     VoDev              视频输出设备号
 * @param[out]    pstPubAttr         视频输出设备公共属性结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 在设置设备公共属性前先获取属性，就可以只设置需要改变的配置项。
 *
 */
FH_SINT32 FH_VO_GetPubAttr(VO_DEV_ID_E VoDev, VO_PUB_ATTR_S *pstPubAttr);

/**
 * @brief         启用视频输出设备
 *
 * @param[in]     VoDev              视频输出设备号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 在使用视频输出功能前必须先进行设备使能操作。
 *    - 在调用设备使能前，必须对设备公共属性进行配置，否则返回设备未配置错误。
 *    - 如果希望更改VO的时序配置，则需要先调用FH_VO_Disable接口，强制关闭VO硬件后再使能。
 *
 */
FH_SINT32 FH_VO_Enable(VO_DEV_ID_E VoDev);

/**
 * @brief         禁用视频输出设备
 *
 * @param[in]     VoDev              视频输出设备号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 设备禁止前必须先禁止该设备上的视频层。
 *    - 设备禁止前，如果有使能WBC，则必须关闭该使能。
 *    - 调用设备使能接口后，如果未调用该接口进行禁止，则VO设备将一直保持使能状态，并且下次设置设备属性时不会生效。
 *    - 设备禁止后需要重新设置设备公共属性，才可使能设备。
 *
 */
FH_SINT32 FH_VO_Disable(VO_DEV_ID_E VoDev);

/**
 * @brief         关闭视频输出模块所有占用的 Fd
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 本接口用于关闭VO设备文件句柄，建议在设备都关闭的情况下使用。
 *
 */
FH_SINT32 FH_VO_CloseFd(FH_VOID);
/**
 * @brief         设置视频层属性
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     pstLayerAttr       视频层属性结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 设置视频层属性必须在视频层禁止的情况下进行。
 *    - 设置视频层属性时，必须保证视频层的绑定关系存在,也需要确保该视频层绑定的设备已经使能。
 *
 */
FH_SINT32 FH_VO_SetVideoLayerAttr(VO_LAYER_ID_E VoLayer, const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr);

/**
 * @brief         获取视频层属性
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[out]    pstLayerAttr       视频层属性结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 不调用FH_VO_EnableVideoLayer和FH_VO_SetVideoLayerAttr也可获取视频层属性。此时获取的为默认值。
 *    - 建议在设置视频层属性前先调用此接口获取视频层属性。
 *
 */
FH_SINT32 FH_VO_GetVideoLayerAttr(VO_LAYER_ID_E VoLayer, VO_VIDEO_LAYER_ATTR_S *pstLayerAttr);

/**
 * @brief         设置视频层压缩属性
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     pstCompressAttr    视频层压缩属性结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 设置视频层压缩属性必须在视频层禁止的情况下进行。
 *    - 设置视频层属性时，必须保证视频层的绑定关系存在,也需要确保该视频层绑定的设备已经使能。
 *
 */
FH_SINT32 FH_VO_SetVideoLayerCompressAttr(VO_LAYER_ID_E VoLayer, const VO_COMPRESS_ATTR_S *pstCompressAttr);

/**
 * @brief         获取视频层压缩属性
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[out]    pstCompressAttr    视频层压缩属性结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 不调用FH_VO_EnableVideoLayer和FH_VO_SetVideoLayerCompressAttr也可获取视频层压缩属性。此时获取的为默认值。
 *    - 建议在设置视频层压缩属性前先调用此接口获取视频层压缩属性。
 *
 */
FH_SINT32 FH_VO_GetVideoLayerCompressAttr(VO_LAYER_ID_E VoLayer, VO_COMPRESS_ATTR_S *pstCompressAttr);

/**
 * @brief         使能视频层
 *
 * @param[in]     VoLayer            视频输出视频层号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 视频层使能前必须保证该视频层所绑定的设备处于使能状态。
 *    - 视频层使能前必须保证该视频层已经配置。
 *
 */
FH_SINT32 FH_VO_EnableVideoLayer (VO_LAYER_ID_E VoLayer);

/**
 * @brief         禁止视频层
 *
 * @param[in]     VoLayer            视频输出视频层号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 视频层禁止前必须保证其上的通道全部禁止。
 *
 */
FH_SINT32 FH_VO_DisableVideoLayer(VO_LAYER_ID_E VoLayer);

/**
 * @brief         设置视频层的分割模式
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     enPartMode         视频层分割模式
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 视频层分割模式指配置硬件显示时，按照什么方式配置区域。多分割即多区域图像按区域数目在硬件上配
 *    - 同等数目的显示信息，单分割即不管多少区域图像，在硬件处理时，只会按照单个区域的图像进行配置。
 *    - 默认VHD和视频层PIP默认为多分割模式，VSD则默认为单分割模式。
 *    - 单分割模式下的多区域拼接是有VGS实现的。
 *    - 调用前必须确保该视频层没有使能。
 *
 */
FH_SINT32 FH_VO_SetVideoLayerPartitionMode(VO_LAYER_ID_E VoLayer, VO_PART_MODE_E enPartMode);

/**
 * @brief         获取视频层的分割模式
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[out]    penPartMode        视频层分割模式
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 */
FH_SINT32 FH_VO_GetVideoLayerPartitionMode(VO_LAYER_ID_E VoLayer, VO_PART_MODE_E *penPartMode);

/**
 * @brief         设置视频层上的通道的设置属性开始
 *
 * @param[in]     VoLayer            视频输出视频层号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - BEGIN和END接口要配对使用，否则BEGIN后设置的通道属性不会生效。
 *    - 此接口可对通道的动态操作进行批处理，例如设置通道属性、通道显示、道隐藏、使能通道、禁止通道等。
 *    - 当VO切换画面大小或显示隐藏时，需要重新准备内存并保证切换的及时性，因此建议的调用顺序为：
 *    - 在 BEGIN 和END 接口之间调用设置通道属性、通道显示、通道隐藏等接口，对设备的多个通道进行批处理操作。
 *
 */
FH_SINT32 FH_VO_SetAttrBegin(VO_LAYER_ID_E VoLayer);

/**
 * @brief         设置视频层上的通道的设置属性结束
 *
 * @param[in]     VoLayer            视频输出视频层号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 调用前需保证FH_VO_SetAttrBegin已经调用。
 *    - BEGIN和END接口要配对使用，否则BEGIN后设置的通道属性不会生效。
 *    - 在调用END接口时，必须要保证此接口及相应的批处理操作的正确性。
 *
 */
FH_SINT32 FH_VO_SetAttrEnd  (VO_LAYER_ID_E VoLayer);

/**
 * @brief         获取输出屏幕图像数据
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     s32MilliSec        超时参数 s32MilliSec 设为-1 时，为阻塞接口；0 时为非阻塞接口；大于 0 时为超时等待时间。超时时间的单位为毫秒（ms）
 * @param[out]    pstVFrame          获取的输出屏幕图像数据信息结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 调用前需保证视频层已经使能。
 *    - 可以多次调用，获取后应保证及时的释放。对于物理VO设备需要立即释放。
 *    - 当视频层上通道没有数据时，获取到屏幕图像为背景色。
 *    - 该接口获取的图像含有时间戳，该时间戳表示图像拼接时的时间信息，如果用户不希望采用该时间戳进行编码，可以自行修改时间戳信息。
 *
 */
FH_SINT32 FH_VO_GetScreenFrame(VO_LAYER_ID_E VoLayer, VIDEO_FRAME_INFO_S *pstVFrame, FH_SINT32 s32MilliSec);

/**
 * @brief         释放输出屏幕图像数据
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     pstVFrame          释放的输出屏幕图像数据信息结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 禁止视频层前要释放已经获取到的屏幕图像。
 *    - 可以多次调用，获取操作应保证与释放操作配对。
 *
 */
FH_SINT32 FH_VO_ReleaseScreenFrame(VO_LAYER_ID_E VoLayer, VIDEO_FRAME_INFO_S *pstVFrame);

/**
 * @brief         设置显示缓冲的长度
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     u32BufLen          显示缓冲的长度
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 调用前需保证视频输出视频层未使能;
 *    - 显示缓冲长度范围为[0,8]，默认值是3;
 *    - 若长度设为0，则不会分配vo的private vb,
 *    - 此时若前级绑定了vgs,则vgs只能设成user模式。
 *
 */
FH_SINT32 FH_VO_SetDispBufLen(VO_LAYER_ID_E VoLayer, FH_UINT32 u32BufLen);

/**
 * @brief         获取显示缓冲的长度
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[out]    pu32BufLen         u32 类型的指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 */
FH_SINT32 FH_VO_GetDispBufLen(VO_LAYER_ID_E VoLayer, FH_UINT32 *pu32BufLen);

/**
 * @brief         启用指定的视频输出通道
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 调用前必须保证视频层绑定关系存在，否则将返回失败。
 *    - 调用前必须使能相应设备上的视频层。
 *    - 通道使能前必须进行通道配置，否则返回通道未配置的错误。
 *    - 允许重复使能同一视频输出通道，不返回失败。
 *
 */
FH_SINT32 FH_VO_EnableChn(VO_LAYER_ID_E VoLayer, VO_CHN VoChn);

/**
 * @brief         禁用指定的视频输出通道
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 允许重复禁用同一视频输出通道，不返回失败。
 *
 */
FH_SINT32 FH_VO_DisableChn(VO_LAYER_ID_E VoLayer, VO_CHN VoChn);

/**
 * @brief         配置指定视频输出通道的属性
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 * @param[in]     pstChnAttr         视频通道属性指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 通道显示区域不能超过视频层属性中设定的画布大小(stImageSize大小)。
 *    - 该接口为动态设置接口，可在 VO 设备使能且相应视频层已配置的情况下调用。
 *
 */
FH_SINT32 FH_VO_SetChnAttr(VO_LAYER_ID_E VoLayer, VO_CHN VoChn, const VO_CHN_ATTR_S *pstChnAttr);

/**
 * @brief         获取指定视频输出通道的属性
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 * @param[out]    pstChnAttr         视频通道属性指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 */
FH_SINT32 FH_VO_GetChnAttr(VO_LAYER_ID_E VoLayer, VO_CHN VoChn, VO_CHN_ATTR_S *pstChnAttr);

/**
 * @brief         配置指定视频输出通道的参数
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 * @param[in]     pstChnParam        视频通道参数指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 该接口为动态设置接口，可在VO设备使能且相应视频层和通道属性已配置的情况下调用。
 *
 */
FH_SINT32 FH_VO_SetChnParam(VO_LAYER_ID_E VoLayer, VO_CHN VoChn, const VO_CHN_PARAM_S *pstChnParam);

/**
 * @brief         获取指定视频输出通道的参数
 *

 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 * @param[out]    pstChnParam        视频通道参数指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 */
FH_SINT32 FH_VO_GetChnParam(VO_LAYER_ID_E VoLayer, VO_CHN VoChn, VO_CHN_PARAM_S *pstChnParam);

/**
 * @brief         设置指定视频输出通道的显示位置
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 * @param[in]     pstDispPos         通道显示位置
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 该接口用于设置通道的实际显示位置，仅适用于VPIP层通道
 *    - 默认情况下，VO 设备从视频层的左上角开始显示视频层画布上的通道，设置通道属性时设定了通道在画布上的位置，通道在画布上的位置即为默认的显示位置。
 *    - 希望节省视频层内存时，可以将视频层的画布大小（stImageSize）设置为实际显示图像需要的内存大小，显示区域大小（stDispRect）设置为 VO 设备输出分辨率大小。这样设置对视频层而言，不会将画布放大到整个显示区域（一般情况下，高清设备的普通视频层这样设置时会将画布放大到整个显示区域。标清和虚拟设备的画布大小和显示区域大小必须一致）。
 *    - 当调用该接口设置通道的显示位置时，新的显示位置将取代默认值，成为 VO 设备上通道的实际显示位置。
 *    - 通道显示位置不能超过视频层属性中设定的显示区域 stDispRect 的范围。
 *    - 调用该接口前必须先设置通道属性。
 *    - 通道显示位置的（x，y）必须 2 对齐。
 *    - 该接口可以动态设置，并支持批处理。建议用户使用批处理方式设置通道的实际显示位置。
 *    - 通道显示位置的（x，y）设置为（-1，-1）时表示恢复通道原有的默认显示位置，用户需注意恢复通道原有的默认显示位置时不要与其他通道的区域产出重叠。建议用户使用批处理方式恢复通道的默认显示位置。
 *    - 该接口设置的显示位置（x,y）在通道 disable 之后仍然有效，用户再使能通道，该通道的显示位置仍然为（x,y）；若用户需要恢复默认的显示位置，需要手动的去设置显示位置为（-1，-1）。
 *    - 该接口在视频层 VHD上面使用时，要配置视频层属性VO_VIDEO_LAYER_ATTR_S 的 bClusterMode 进行使用。
 *
 */
FH_SINT32 FH_VO_SetChnDispPos(VO_LAYER_ID_E VoLayer, VO_CHN VoChn, const POINT_S *pstDispPos);

/**
 * @brief         获取指定视频输出通道的参数
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 * @param[out]    pstDispPos         视频通道显示位置属性指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 *
 */
FH_SINT32 FH_VO_GetChnDispPos(VO_LAYER_ID_E VoLayer, VO_CHN VoChn, POINT_S *pstDispPos);

/**
 * @brief         设置指定视频输出通道的帧场显示策略
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 * @param[in]     enField            视频通道显示帧场策略
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 如果不显式调用此接口，VO默认为两场显示（即 VO_FIELD_BOTH）。
 *    - 如果送VO显示的图像为帧图像（例如单场CIF图像），则调用此接口对图像显示无影响。
 *    - 该接口仅对标清、虚拟设备有效，高清设备调用该接口无效。
 *
 */
FH_SINT32 FH_VO_SetChnField(VO_LAYER_ID_E VoLayer, VO_CHN VoChn, const VO_DISPLAY_FIELD_E enField);

/**
 * @brief         获取指定视频输出通道的帧场显示策略
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 * @param[out]    pField             视频通道显示帧场策略指针

 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 如未设置显示策略则返回系统默认策略 VO_FIELD_BOTH。
 *
 */
FH_SINT32 FH_VO_GetChnField(VO_LAYER_ID_E VoLayer, VO_CHN VoChn, VO_DISPLAY_FIELD_E *pField);

/**
 * @brief         设置指定视频输出通道的显示帧率
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 * @param[in]     s32ChnFrmRate      视频通道显示帧率
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 需先设置VO相应视频输出通道属性，才能调用此接口，否则返回失败。
 *    - 帧率可设置为Nx（其中N为[-64，+64]的任意整数，x为P制或N制下的帧率）。负数的倍数用于倒放操作，此时需要用户来倒序送图像到 VO，即送时间戳递减的图像。
 *    - 在通道禁用的情况下，调用FH_VO_SetChnAttr 接口会将通道显示帧率重置为视频显示帧率。通道显示帧率为通道的输入的图像的帧率，视频显示帧率为通道最终要显示的帧率，视频显示帧率可以在视频层属性处设置。
 *
 */
FH_SINT32 FH_VO_SetChnFrameRate(VO_LAYER_ID_E VoLayer, VO_CHN VoChn, FH_SINT32 s32ChnFrmRate);

/**
 * @brief         获取指定视频输出通道的显示帧率
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 * @param[out]    ps32ChnFrmRate     视频通道显示帧率数据指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 获取的为用户配置的帧率，如未设置则返回满帧率。
 *
 */
FH_SINT32 FH_VO_GetChnFrameRate(VO_LAYER_ID_E VoLayer, VO_CHN VoChn, FH_SINT32 *ps32ChnFrmRate);

/**
 * @brief         获取通道帧
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 * @param[in]     s32MilliSec        超时参数 s32MilliSec 设为-1 时，为阻塞接口；0 时为非阻塞接口；大于 0 时为超时等待时间。超时时间的单位为毫秒（ms）
 * @param[out]    pstFrame           获取的输出屏幕图像数据信息结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 */
FH_SINT32 FH_VO_GetChnFrame(VO_LAYER_ID_E VoLayer, VO_CHN VoChn, VIDEO_FRAME_INFO_S *pstFrame, FH_SINT32 s32MilliSec);

/**
 * @brief         释放输出通道图像数据
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 * @param[in]     pstFrame           获取的输出屏幕图像数据信息结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 获取操作应保证与释放操作配对。
 *    - 不允许重复释放。
 *
 */
FH_SINT32 FH_VO_ReleaseChnFrame(VO_LAYER_ID_E VoLayer, VO_CHN VoChn, const VIDEO_FRAME_INFO_S *pstFrame);


/**
 * @brief         暂停指定的视频输出通道
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - VI-VO 预览时不推荐使用此接口。
 *    - 允许重复暂停同一通道，不返回失败。
 *
 */
FH_SINT32 FH_VO_PauseChn (VO_LAYER_ID_E VoLayer, VO_CHN VoChn);

/**
 * @brief         恢复指定的视频输出通道
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 允许重复恢复同一通道，不返回失败。
 *
 */
FH_SINT32 FH_VO_ResumeChn(VO_LAYER_ID_E VoLayer, VO_CHN VoChn);

/**
 * @brief         单帧播放指定的视频输出通道
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 恢复正常播放使用FH_VO_ResumeChn接口。
 *    - VI-VO 预览时不推荐使用此接口。
 *
 */
FH_SINT32 FH_VO_StepChn(VO_LAYER_ID_E VoLayer, VO_CHN VoChn);

/**
 * @brief         显示指定通道
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 调用前需保证视频输出通道所在的视频层已经使能。
 *    - 默认情况下通道处于显示状态。
 *
 */
FH_SINT32 FH_VO_ShowChn(VO_LAYER_ID_E VoLayer, VO_CHN VoChn);

/**
 * @brief         隐藏指定通道
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 调用前需保证视频输出通道所在的视频层已经使能。
 *    - 默认情况下通道处于显示状态。
 *
 */
FH_SINT32 FH_VO_HideChn(VO_LAYER_ID_E VoLayer, VO_CHN VoChn);

/**
 * @brief         设置播放容忍度
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     u32Toleration      播放容忍度
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 调用前需保证视频层已使能。
 *    - 播放容忍度是以毫秒为单位，默认值为 600000。
 *    - 当两帧解码数据的时间戳之差的绝对值超过容忍度时，系统将重置时间戳，以当前帧的时间戳为起点，对解码数据进行帧率控制。播放容忍度仅对解码图像的播放控制生效，对预览的播放控制无效。
 *
 */
FH_SINT32 FH_VO_SetPlayToleration(VO_LAYER_ID_E VoLayer, FH_UINT32 u32Toleration);

/**
 * @brief         获取播放容忍度
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[out]    pu32Toleration     播放容忍度指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 调用前需保证视频层已使能。
 *
 */
FH_SINT32 FH_VO_GetPlayToleration(VO_LAYER_ID_E VoLayer, FH_UINT32 *pu32Toleration);

/**
 * @brief         将视频图像送入指定输出通道显示
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道的通道号
 * @param[in]     pstVFrame          视频数据信息指针
 * @param[in]     s32MilliSec        超时参数 s32MilliSec 设为-1 时，为阻塞接口；0 时为非阻塞接口；大于 0 时为超时等待时间。超时时间的单位为毫秒（ms）
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 调用该接口前必须保证通道已经使能。
 *    - s32MilliSec 需要设置非0值时，由于需要等硬件还回上一帧的buffer，第一帧建议设置时间为0，
 *    - 否则第一帧会等到超时或者阻塞住。
 *
 */
FH_SINT32 FH_VO_SendFrame(VO_LAYER_ID_E VoLayer, VO_CHN VoChn, VIDEO_FRAME_INFO_S *pstVFrame, FH_SINT32 s32MilliSec);

/**
 * @brief         设置回写通路的回写源
 *
 * @param[in]     VoWbc              视频回写通路号
 * @param[in]     pstWbcSource       视频回写源结构体
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 设置回写源之前不能对回写属性进行设置。
 *
 */
FH_SINT32 FH_VO_SetWbcSource(VO_WBC VoWbc, const VO_WBC_SOURCE_S *pstWbcSource);

/**
 * @brief         获取回写通路的回写源
 *
 * @param[in]     VoWbc              视频回写通路号
 * @param[out]    pstWbcSources      视频回写源结构体
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 未设置回写源时，返回默认的回写源。
 *
 */
FH_SINT32 FH_VO_GetWbcSource(VO_WBC VoWbc, VO_WBC_SOURCE_S *pstWbcSources);

/**
 * @brief         使能视频回写
 *
 * @param[in]     VoWbc              视频回写通路号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 设备必须先使能。
 *    - 必须先设置视频回写属性，再使能视频回写功能。
 *    - 重复使能视频回写返回成功。
 *    - 建议使能设备后，等待该设备显示图像数据后再使能WBC回写功能。
 *
 */
FH_SINT32 FH_VO_EnableWbc(VO_WBC VoWbc);

/**
 * @brief         禁用视频回写
 *
 * @param[in]     VoWbc              视频回写通路号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 重复禁用视频回写返回成功。
 *    - 禁用视频回写后，会把视频回写源，视频回写属性和视频回写模式恢复为默认值，再次使能视频回写时，需要重新设置。
 *
 */
FH_SINT32 FH_VO_DisableWbc(VO_WBC VoWbc);

/**
 * @brief         设置视频回写属性
 *
 * @param[in]     VoWbc              视频回写通路号
 * @param[in]     pstWbcAttr         视频回写属性指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 属性中的目标大小不能超过回写设备设定的显示大小，且确保需要缩小的倍数在
 *    - WBC 允许的缩小倍数范围内。SPYCbCr422 格式图像最大支持16倍（不含16倍）缩小，SPYCbCr420 格式最大支持8倍（不含8倍）缩小。
 *    - WBC 回写模式设为VO_WBC_MODE_PROG_TO_INTL时，SPYCbCr422 格式的图像最大支持缩放8倍（不含8倍）缩小，SPYCbCr420格式最大支持 4 倍（不含4倍）缩小。
 *    - 该接口为动态设置接口，可在WBC视频回写使能的情况下调用。
 *    - 视频回写属性中，宽高必须大于32，且2对齐。
 *    - 设置视频回写属性时必须先设置视频回写源。
 *
 */
FH_SINT32 FH_VO_SetWbcAttr(VO_WBC VoWbc, const VO_WBC_ATTR_S *pstWbcAttr);

/**
 * @brief         获取视频回写属性
 *
 * @param[in]     VoWbc              视频回写通路号
 * @param[out]    pstWbcAttr         视频回写属性指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 *
 */
FH_SINT32 FH_VO_GetWbcAttr(VO_WBC VoWbc, VO_WBC_ATTR_S *pstWbcAttr);

/**
 * @brief         设置视频回写模式
 *
 * @param[in]     VoWbc              视频回写通路号
 * @param[in]     enWbcMode          视频回写模式
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 可调用该接口设置回写模式，达到最佳的回写显示效果。
 *    - 该接口为静态接口，必须先禁用 WBC 后才能改变模式。
 *
 */
FH_SINT32 FH_VO_SetWbcMode(VO_WBC VoWbc, VO_WBC_MODE_E enWbcMode);

/**
 * @brief         获取视频回写模式
 *
 * @param[in]     VoWbc              视频回写通路号
 * @param[out]    penWbcMode         视频回写模式
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 *
 */
FH_SINT32 FH_VO_GetWbcMode(VO_WBC VoWbc, VO_WBC_MODE_E *penWbcMode);

/**
 * @brief         设置视频回写图像的缓存深度
 *
 * @param[in]     VoWbc              视频回写通路号
 * @param[in]     u32Depth           视频回写图像的缓存深度
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 用户可以获取回写图像用于编码、或标清显示、或其他目的，设置视频回写图像的缓存深度可以让用户获取回写图像时不丢失视频帧。
 *
 */
FH_SINT32 FH_VO_SetWbcDepth(VO_WBC VoWbc, FH_UINT32 u32Depth);

/**
 * @brief         获取视频回写图像的缓存深度
 *
 * @param[in]     VoWbc              视频回写通路号
 * @param[out]    pu32Depth           视频回写图像的缓存深度指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 *
 */
FH_SINT32 FH_VO_GetWbcDepth(VO_WBC VoWbc, FH_UINT32 *pu32Depth);

/**
 * @brief         获取视频回写图像
 *
 * @param[in]     VoWbc              视频回写通路号
 * @param[in]     s32MilliSec        超时参数 s32MilliSec 设为-1 时，为阻塞接口；0 时为非阻塞接口；大于 0 时为超时等待时间。超时时间的单位为毫秒（ms）
 * @param[out]    pstVFrame          获取的视频回写图像数据信息结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 调用前应先设置视频回写缓存深度为大于 0 的值，否则获取不到图像。
 *    - 调用前应该视频回写使能，否则获取不到图像。
 *    - 获取后应保证及时的释放。
 *
 */
FH_SINT32 FH_VO_GetWbcFrame(VO_WBC VoWbc, VIDEO_FRAME_INFO_S *pstVFrame, FH_SINT32 s32MilliSec);

/**
 * @brief         释放视频回写图像
 *
 * @param[in]     VoWbc              视频回写通路号
 * @param[in]     pstVFrame          释放的输出通道图像数据信息结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 获取操作应保证与释放操作配对。
 *
 */
FH_SINT32 FH_VO_ReleaseWbcFrame(VO_WBC VoWbc, VIDEO_FRAME_INFO_S *pstVFrame);

/**
 * @brief         获取回写需要的Buffer尺寸
 *
 * @param[in]     VoWbc              视频回写通路号
 * @param[in]     pNeedsize          回写所需要分配的Buffer大小
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 需要在FH_VO_SetWbcAttr和FH_VO_SetWbcDepth之后调用。
 *    - 仅在不支持VB的情况下有效。
 *
 */
FH_SINT32 FH_VO_GetWbcFrameBufferSize(VO_WBC VoWbc, FH_UINT32 *pNeedsize);


/**
 * @brief         分配给驱动回写需要的Buffer
 *
 * @param[in]     VoWbc              视频回写通路号
 * @param[in]     pstBuf             Buffer结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 需要在FH_VO_GetWbcFrameBufferSize之后，FH_VO_EnableWbc之前调用。
 *    - 仅在不支持VB的情况下有效。
 *
 */
FH_SINT32 FH_VO_SetWbcFrameBuffer(VO_WBC VoWbc, FH_MEM_INFO *pstBuf);

/**
 * @brief         绑定图形层到 VO 设备
 *
 * @param[in]     GraphicLayer       图形层号
 * @param[in]     VoDev              视频设备号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 调用前需保证待绑定的图形层处于关闭状态。
 *    - 不允许重复绑定，需要先解绑定才能重新绑定。
 *
 */
FH_SINT32 FH_VO_BindGraphicLayer(GRAPHIC_LAYER GraphicLayer, VO_DEV_ID_E VoDev);

/**
 * @brief         解除图形层与 VO 设备的绑定关系
 *
 * @param[in]     GraphicLayer       图形层号
 * @param[in]     VoDev              视频设备号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 调用前需保证待绑定的图形层处于关闭状态。
 *    - VoDev目前没有什么意义，将其赋值为0即可。
 *    - 对未绑定的图形层进行解绑定会返回成功，即允许对同一个图形层解绑定多次。
 *
 */
FH_SINT32 FH_VO_UnBindGraphicLayer(GRAPHIC_LAYER GraphicLayer, VO_DEV_ID_E VoDev);

/**
 * @brief         挂起 VO 设备
 *
 * @param[in]     VoDev              视频设备号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 *
 */
FH_SINT32 FH_VO_Suspend(VO_DEV_ID_E VoDev);

/**
 * @brief         释放 VO 设备
 *
 * @param[in]     VoDev              视频设备号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 *
 */
FH_SINT32 FH_VO_Resume(VO_DEV_ID_E VoDev);

/**
 * @brief         设置通道输出图像效果
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 * @param[in]     pstVideoCSC        图像输出效果结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 该接口主要用于调节图像的输出效果，包括亮度、对比度、色调、饱和度，其取值范围为[0, 100]。
 *
 */
FH_SINT32 FH_VO_SetChnCSC(VO_LAYER_ID_E VoLayer, VO_CHN VoChn, const VO_CSC_S *pstVideoCSC);

/**
 * @brief         获取通道输出图像效果
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 * @param[out]    pstVideoCSC        图像输出效果结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 该接口主要用于获取通道的输出效果，包括亮度、对比度、色调、饱和度，其取值范围为[0, 100]。
 *
 */
FH_SINT32 FH_VO_GetChnCSC(VO_LAYER_ID_E VoLayer, VO_CHN VoChn, VO_CSC_S *pstVideoCSC);

/**
 * @brief         启用图像层
 *
 * @param[in]     GraphicLayer       图形层号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 */
FH_SINT32 FH_VO_EnableGraphicLayer(GRAPHIC_LAYER GraphicLayer);
/**
 * @brief         禁用图像层
 *
 * @param[in]     GraphicLayer       图形层号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 *
 */
FH_SINT32 FH_VO_DisableGraphicLayer(GRAPHIC_LAYER GraphicLayer);

/**
 * @brief         显示图像层
 *
 * @param[in]     GraphicLayer       图形层号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 默认情况下通道处于显示状态。
 *
 */
FH_SINT32 FH_VO_ShowGraphicLayer(GRAPHIC_LAYER GraphicLayer);

/**
 * @brief         隐藏图像层
 *
 * @param[in]     GraphicLayer       图形层号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 *
 */
FH_SINT32 FH_VO_HideGraphicLayer(GRAPHIC_LAYER GraphicLayer);

/**
 * @brief         设置图像层位置
 *
 * @param[in]     GraphicLayer       图形层号
 * @param[in]     pstPos             图像位置结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 *
 */
FH_SINT32 FH_VO_SetGraphicLayerPos(GRAPHIC_LAYER GraphicLayer, const POINT_S *pstPos);

/**
 * @brief         设置图像层尺寸
 *
 * @param[in]     GraphicLayer       图形层号
 * @param[in]     pstSize            图像尺寸结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 *
 */
FH_SINT32 FH_VO_SetGraphicLayerSize(GRAPHIC_LAYER GraphicLayer, const FH_SIZE *pstSize);

/**
 * @brief         设置图像层透明度
 *
 * @param[in]     GraphicLayer       图形层号
 * @param[in]     pstAlpha           图像透明度结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 */
FH_SINT32 FH_VO_SetGraphicLayerAlpha(GRAPHIC_LAYER GraphicLayer, const ALPHA_S *pstAlpha);

/**
 * @brief         更新图像层
 *
 * @param[in]     GraphicLayer       图形层号
 * @param[in]     pstVFrame          视频帧信息结构体指针
 * @param[in]     s32MilliSec        超时参数 s32MilliSec 设为-1 时，为阻塞接口；0 时为非阻塞接口；大于 0 时为超时等待时间。超时时间的单位为毫秒（ms）
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 */
FH_SINT32 FH_VO_UpdateGraphicLayer(GRAPHIC_LAYER GraphicLayer, VIDEO_FRAME_INFO_S *pstVFrame, FH_SINT32 s32MilliSec);
/**
 * @brief         获取输出图像层图像数据
 *
 * @param[in]     GraphicLayer       图形层号
 * @param[in]     pstVFrame          视频帧信息结构体指针
 * @param[in]     s32MilliSec        超时参数 s32MilliSec 设为-1 时，为阻塞接口；0 时为非阻塞接口；大于 0 时为超时等待时间。超时时间的单位为毫秒（ms）
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 获取后应保证及时的释放。
 *
 */
FH_SINT32 FH_VO_GetGraphicLayerFrame(GRAPHIC_LAYER GraphicLayer, VIDEO_FRAME_INFO_S *pstVFrame, FH_SINT32 s32MilliSec);
/**
 * @brief         释放输出图像层图像数据
 *
 * @param[in]     GraphicLayer       图形层号
 * @param[in]     pstVFrame          视频帧信息结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 获取操作应保证与释放操作配对。
 *
 */
FH_SINT32 FH_VO_ReleaseGraphicLayerFrame(GRAPHIC_LAYER GraphicLayer, VIDEO_FRAME_INFO_S *pstVFrame);

/**
 * @brief         设置HDMI显示分辨率
 *
 * @param[in]     VoDev              视频设备号
 * @param[in]     pstHdmiFmt         HDMI显示分辨率结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 仅FHYG2支持
 *
 */
FH_SINT32 FH_VO_SetHdmiFmt(VO_DEV_ID_E VoDev, VO_HDMI_FMT_S *pstHdmiFmt);

/**
 * @brief         获取HDMI显示分辨率
 *
 * @param[out]    pstHdmiFmt         HDMI显示分辨率结构体指针
 * @param[in]     VoDev              视频设备号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 仅FHYG2支持
 *
 */
FH_SINT32 FH_VO_GetHdmiFmt(VO_DEV_ID_E VoDev, VO_HDMI_FMT_S *pstHdmiFmt);

/**
 * @brief         填充视频输出通道
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 * @param[in]     u32RGB             RGB颜色
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 */
FH_SINT32 FH_VO_FillChn(VO_LAYER_ID_E VoLayer, VO_CHN VoChn, FH_UINT32 u32RGB);

/**
 * @brief         设置测试模式
 *
 * @param[in]     VoDev              视频设备号
 * @param[in]     pstTestParam       测试模式结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 *
 */
FH_SINT32 FH_VO_SetTestMode(VO_DEV_ID_E VoDev, VO_TEST_PARAM_S* pstTestParam);

/**
 * @brief         查询视频输出通道状态
 *
 * @param[out]    pstStatus          通道状态结构体指针
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 调用前需保证视频设备使能。
 *    - 调用前需要保证所要查询的通道已经使能。
 *    - 可以多次调用获取通道状态接口。
 *
 */
FH_SINT32 FH_VO_QueryChnStat(VO_LAYER_ID_E VoLayer, VO_CHN VoChn, VO_QUERY_STATUS_S *pstStatus);

/**
 * @brief         设置视频层输出图像效果
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     pstVideoCSC        图像输出效果结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 该接口主要用于调节图像的输出效果，包括亮度、对比度、色调、饱和度，其取值范围均为[0, 100]。
 *    - 禁止视频层时会将亮度、对比度、色调、饱和度的值恢复为默认值，CSC 转换矩阵恢复为默认值。
 *
 */
FH_SINT32 FH_VO_SetVideoLayerCSC(VO_LAYER_ID_E VoLayer, const VO_CSC_S *pstVideoCSC);

/**
 * @brief         获取设备输出图像效果
 *
 * @param[out]    pstVideoCSC        图像输出效果结构体指针
 * @param[in]     VoLayer            视频输出视频层号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 该接口主要用于获取图形层的输出效果，包括亮度、对比度、色调、饱和度，其取值范围为[0, 100]。
 *
 */
FH_SINT32 FH_VO_GetVideoLayerCSC(VO_LAYER_ID_E VoLayer, VO_CSC_S *pstVideoCSC);

/**
 * @brief         清空指定输出通道的缓存buffer数据
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 * @param[in]     bClrAll            是否将通道buffer中的数据全部清空
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 用户可以调用此接口实现清除VO通道buffer的功能。
 *
 */
FH_SINT32 FH_VO_ClearChnBuffer(VO_LAYER_ID_E VoLayer, VO_CHN VoChn, FH_BOOL bClrAll);

/**
 * @brief         检查vo的通道buffer占用情况
 *
 * @param[in]     chan               通道号
 * @param[in]     addr               检查的物理地址
 * @param[in]     size               检查的buffer尺寸
 * @param[in]     timeout_ms         超时时间
 * @param[out]    isidle             buffer的占用状态
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 *
 */
FH_SINT32 FH_VO_CheckAddr(FH_UINT32 chan, FH_PHYADDR addr, FH_UINT32 size, FH_UINT32 timeout_ms, FH_UINT32 *isidle);

/**
 * @brief         获取通道占用的buf个数
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 * @param[out]    pNum               通道占用的buf个数指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 */
FH_SINT32 FH_VO_GetChnBufNum(VO_LAYER_ID_E VoLayer, VO_CHN VoChn, FH_UINT32 *pNum);

/**
 * @brief         设置缓存队列深度
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[out]    u32NodeDepth       缓存队列深度
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 只在FH_VO_SetDispBufLen设为0时生效,非0时和BufLen一致；
 *    - 需在FH_VO_SetDispBufLen前调用,为保证显示流畅,需不小于3个。
 *
 */
FH_SINT32 FH_VO_SetNodeDepth(VO_LAYER_ID_E VoLayer, FH_UINT32 u32NodeDepth);

/**
 * @brief         获取通道需要的buffer尺寸
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 * @param[out]    pNeedsize          通道需要bufferSize
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vou_errcode} 。
 *
 * @note
 *    - 仅在不支持VB的情况下有效。
 *
 */
FH_SINT32 FH_VO_GetChnBufferSize(VO_LAYER_ID_E VoLayer, VO_CHN VoChn, FH_UINT32 *pNeedsize);

/**
 * @brief         设置通道需要的buffer
 *
 * @param[in]     VoLayer            视频输出视频层号
 * @param[in]     VoChn              视频输出通道号
 * @param[in]     pstBuf             通道需要buffer结构体指针
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,xx_errcode} 。
 *
 * @note
 *    - 仅在不支持VB的情况下有效。
 *
 */

FH_SINT32 FH_VO_SetChnBuffer(VO_LAYER_ID_E VoLayer, VO_CHN VoChn, FH_MEM_INFO *pstBuf);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /*__FH_VO_MPI_H__ */

