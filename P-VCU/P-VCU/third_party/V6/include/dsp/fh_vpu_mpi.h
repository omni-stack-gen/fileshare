#ifndef __FH_VPU_MPI_H__
#define __FH_VPU_MPI_H__

#include "types/type_def.h"
#include "fh_vpu_mpipara.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/**
 * @brief         设置视频输入属性
 *
 * @param[in]    grpidx           GROUP号
 * @param[in]    pstViconfig      视频输入幅面
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 输入幅面必须和ISP输出幅面完全一致,否则会导致图像错误或者无法完成。
 *
 */
FH_SINT32 FH_VPSS_SetViAttr(FH_UINT32 grpidx,FH_VPU_SIZE *pstViconfig);


/**
 * @brief         获取视频输入属性
 *
 * @param[in]    grpidx           GROUP号
 * @param[out]   pstViconfig      视频输出幅面
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_GetViAttr(FH_UINT32 grpidx,FH_VPU_SIZE *pstViconfig);

/**
 * @brief         创建视频输入
 *
 * @param[in]    grpidx            GROUP号
 * @param[in]    pstChnconfig      视频输入幅面和ycmean相关信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_CreateGrp(FH_UINT32 grpidx,FH_VPU_SET_GRP_INFO *pstChnconfig);

/**
 * @brief         销毁视频输入
 *
 * @param[in]    grpidx      GROUP号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_DestroyGrp(FH_UINT32 grpidx);


/**
 * @brief         创建通道
 *
 * @param[in]    grpidx        GROUP号
 * @param[in]    chan          通道号
 * @param[in]    chn_info      创建通道需要用到的相关信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - MC635X不支持BGM功能，所以bgm_enable和bgm_ds参数对驱动无效。
 *    - CPY和SAD在本平台上对编码无用，除非APP有特殊用途，可以不打开这几个功能。
 *    - 如果开启CPY功能，通道的宽度必须要对齐到16。
 *
 */
FH_SINT32 FH_VPSS_CreateChn(FH_UINT32 grpidx,FH_UINT32 chan,FH_VPU_CHN_INFO *chn_info);

/**
 * @brief         销毁VPU通道
 *
 * @param[in]    grpidx      GROUP号
 * @param[in]    chan        通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_DestroyChn(FH_UINT32 grpidx,FH_UINT32 chan);

/**
 * @brief         获取视频处理模块中供BGM使用的图像数据
 *
 * @param[in]    grpidx          GROUP号
 * @param[out]   pstBGMData      供BGM使用图像数据信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - MC635X由于无BGM功能，所以此接口无效。返回功能不支持错误码
 *
 */
FH_SINT32 FH_VPSS_GetBGMData(FH_UINT32 grpidx,FH_PIC_DATA *pstBGMData);

/**
 * @brief         获取视频处理模块中纹理复杂度数据
 *
 * @param[in]    grpidx          GROUP号
 * @param[out]   pstCPYData      纹理复杂度数据信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 阻塞接口,超时时间为1s。
 *
 */
FH_SINT32 FH_VPSS_GetCPYData(FH_UINT32 grpidx,FH_PIC_DATA *pstCPYData);

/**
 * @brief         使能视频处理模块
 *
 * @param[in]    grpidx     GROUP号
 * @param[in]    mode      视频处理模块工作模式
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 更换工作模式时，需先调用FH_VPSS_Disable,然后再调用此函数。
 *
 */
FH_SINT32 FH_VPSS_Enable(FH_UINT32 grpidx,FH_VPU_VI_MODE mode);

/**
 * @brief         关闭视频处理模块
 *
 * @param[in]    grpidx      GROUP号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_Disable(FH_UINT32 grpidx);

/**
 * @brief         视频冻结,视频处理模块将会向后面数据处理模块,不断发送最后一帧的数据.直到调用 #FH_VPSS_UnfreezeVideo 。
 *
 * @param[in]    grpidx      GROUP号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 当通道的输出缓存数量为1帧或者特殊工作模式时,此接口无效。
 *
 */
FH_SINT32 FH_VPSS_FreezeVideo(FH_UINT32 grpidx);

/**
 * @brief         视频解冻
 *
 * @param[in]    grpidx      GROUP号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_UnfreezeVideo(FH_UINT32 grpidx);

/**
 * @brief         获取视频处理模块通道输出的图像数据
 *
 * @param[in]    grpidx               GROUP号
 * @param[in]    chan                 通道号
 * @param[out]   pstVpuframeinfo      图像数据信息
 * @param[in]    timeout_ms           接口阻塞超时时间(ms)
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_GetChnFrameAdv(FH_UINT32 grpidx,FH_UINT32 chan,FH_VPU_STREAM_ADV *pstVpuframeinfo,FH_UINT32 timeout_ms);

/**
 * @brief         获取视频处理模块通道输出的图像数据,并锁定此缓存不被硬件继续使用.建议至少分配3buf及以上才使用此接口．
 *
 * @param[in]    grpidx                GROUP号
 * @param[in]    chan                  通道号
 * @param[out]   pstVpuframeinfo       图像数据信息
 * @param[in]    timeout_ms            接口阻塞超时时间(ms)
 * @param[out]   handle_lock           通道缓存的lock句柄
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 使用Lock接口且该通道绑定后续编码模块时建议需要至少配置3个buf以上.
 *    - 且建议需要保持非lock的buf数量至少为2个,只有1个可用buf时驱动将不再做内存保护(可能出现图像割线)．
 *    - 当底层所有buf都被lock住时，后续所有帧都会被丢弃．
 *
 */
FH_SINT32 FH_VPSS_LockChnFrameAdv(FH_UINT32 grpidx,FH_UINT32 chan,FH_VPU_STREAM_ADV *pstVpuframeinfo,FH_UINT32 timeout_ms,FH_UINT32 *handle_lock);

/**
 * @brief         获取视频处理模块通道输出的图像数据
 *
 * @param[in]    grpidx               GROUP号
 * @param[in]    chan                 通道号
 * @param[out]   pstVpuframeinfo      图像数据信息
 * @param[in]    timeout_ms           接口阻塞超时时间(ms)
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - timeout_ms为0时不返回已经已经被获取过的帧
 *    - timeout_ms不为0时若前一帧未被取走则直接返回前一帧，否则进入等待
 *    - 此接口不支持并发获取相同通道数据，否则多线程间会相互竞争资源
 *
 */
FH_SINT32 FH_VPSS_GetChnFrameAdv_NoRpt(FH_UINT32 grpidx,FH_UINT32 chan,FH_VPU_STREAM_ADV *pstVpuframeinfo,FH_UINT32 timeout_ms);

/**
 * @brief         获取视频处理模块通道输出的图像数据,并锁定此缓存不被硬件继续使用.建议至少分配3buf及以上才使用此接口．
 *
 * @param[in]    grpidx                GROUP号
 * @param[in]    chan                  通道号
 * @param[out]    pstVpuframeinfo      图像数据信息
 * @param[in]    timeout_ms            接口阻塞超时时间(ms)
 * @param[out]    handle_lock          通道缓存的lock句柄
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 使用Lock接口且该通道绑定后续编码模块时建议需要至少配置3个buf以上.
 *    - 且建议需要保持非lock的buf数量至少为2个,只有1个可用buf时驱动将不再做内存保护(可能出现图像割线)．
 *    - 当底层所有buf都被lock住时，后续所有帧都会被丢弃．
 *    - timeout_ms为0时不返回已经已经被获取过的帧
 *    - timeout_ms不为0时若前一帧未被取走则直接返回前一帧，否则进入等待
 *    - 此接口不支持并发获取相同通道数据，否则多线程间会相互竞争资源
 *
 */
FH_SINT32 FH_VPSS_LockChnFrameAdv_NoRpt(FH_UINT32 grpidx,FH_UINT32 chan,FH_VPU_STREAM_ADV *pstVpuframeinfo,FH_UINT32 timeout_ms,FH_UINT32 *handle_lock);

/**
 * @brief         释放被 #FH_VPSS_LockChnFrameAdv 锁定的缓存
 *
 * @param[in]    grpidx               GROUP号
 * @param[in]    chan                 通道号
 * @param[in]    pstVpuframeinfo      图像数据信息
 * @param[in]    lock_handle          通道缓存的lock句柄
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_UnlockChnFrameAdv(FH_UINT32 grpidx,FH_UINT32 chan,FH_VPU_STREAM_ADV *pstVpuframeinfo,FH_UINT32 lock_handle);

/**
 * @brief      获取视频处理模块通道输出的图像数据
 *
 * @param[in]  grpidx           GROUP号
 * @param[in]  chan             通道号
 * @param[out] pstVpuframeinfo  图像数据信息
 * @param[in]  timeout_ms       接口阻塞超时时间(ms)
 * @param[out] handle_lock      通道缓存的lock句柄
 * @param[in]  is_lock          是否阻塞
 * @param[in]  is_norpt         是否使用NoRpt模式
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 */
FH_SINT32 FH_VPSS_GetChnFrame(FH_UINT32 grpidx,FH_UINT32 chan,FH_VIDEO_FRAME *pstVpuframeinfo,
	FH_UINT32 timeout_ms,FH_UINT32 *handle_lock,FH_UINT32 is_lock,FH_UINT32 is_norpt);

/**
 * @brief      释放被 FH_VPSS_GetChnFrame 锁定的缓存
 *
 * @param[in]  grpidx           GROUP 号
 * @param[in]  chan             通道号
 * @param[in]  pstVpuframeinfo  图像数据信息
 * @param[in]  lock_handle      通道缓存的 lock 句柄
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 需要在 FH_VPSS_SetChnAttr 后调用
 */
FH_SINT32 FH_VPSS_ReleaseChnFrame(FH_UINT32 grpidx,FH_UINT32 chan,FH_VIDEO_FRAME *pstVpuframeinfo,FH_UINT32 lock_handle);

/**
 * @brief         提交用户图像给视频处理模块进行处理,仅在处于非直通模式下有效。
 *
 * @param[in]    grpidx          GROUP号
 * @param[in]    pstUserPic      用户图像信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_SendUserPic(FH_UINT32 grpidx,const FH_VPU_USER_PIC *pstUserPic);
/**
 * @brief         提交用户图像给视频处理模块进行处理,仅在处于非直通模式下有效。
 *
 * @param[in]    grpidx           GROUP号
 * @param[in]    blkidx           BLK号
 * @param[in]    pstUserPic       用户图像信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_SendUserPicAdv(FH_UINT32 grpidx,FH_UINT32 blkidx,const FH_VPU_USER_PIC *pstUserPic);


/**
 * @brief     提交用户图像给视频处理模块进行处理,仅在处于非直通模式下有效。
 *
 * @param[in]  grpidx      GROUP号
 * @param[in]  blkidx      BLK号
 * @param[in]  timeout_ms  提交图像入队列阻塞超时时间(ms)
 * @param[in]  pstUserPic  用户图像信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *     需要在 #FH_VPSS_SetViAttr 后调用
 */
FH_SINT32 FH_VPSS_SendFrame(FH_UINT32 grpidx,FH_UINT32 blkidx,FH_UINT32 timeout_ms,const FH_VIDEO_FRAME *pstUserPic);


/**
 * @brief         设置视频处理模块通道属性。修改通道属性时不需要停止视频处理模块。但建议先停止该通道的使能。
 *
 * @param[in]    grpidx            GROUP号
 * @param[in]    chan              通道号
 * @param[in]    pstChnconfig      通道属性信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 在设置通道属性后,包含帧率控制,视频冻结等通道配置会失效,需要重新设置。
 *
 */
FH_SINT32 FH_VPSS_SetChnAttr(FH_UINT32 grpidx,FH_UINT32  chan,const FH_VPU_CHN_CONFIG *pstChnconfig);

/**
 * @brief         获取视频处理模块通道属性。
 *
 * @param[in]    grpidx            GROUP号
 * @param[in]    chan              通道号
 * @param[out]   pstChnconfig      通道属性信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_GetChnAttr(FH_UINT32 grpidx,FH_UINT32  chan, FH_VPU_CHN_CONFIG *pstChnconfig);

/**
 * @brief         设置视频处理模块扩展通道属性。
 *
 * @param[in]    grpidx            GROUP号
 * @param[in]    chan              通道号
 * @param[in]    pstChnconfig      扩展通道属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 扩展通道只支持光栅和YUYV模式,要求绑定的通道输出模式与扩展通道相同。
 *    - 在设置扩展通道属性后,包含帧率控制,视频冻结等通道配置会失效,需要重新设置。
 *
 */
FH_SINT32 FH_VPSS_SetExtChnAttr(FH_UINT32 grpidx,FH_UINT32 chan,const FH_VPU_EXT_CHN_CONFIG *pstChnconfig);

/**
 * @brief         获取视频处理模块扩展通道属性。
 *
 * @param[in]    grpidx            GROUP号
 * @param[in]    chan              通道号
 * @param[in]    pstChnconfig      扩展通道属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 */
FH_SINT32 FH_VPSS_GetExtChnAttr(FH_UINT32 grpidx,FH_UINT32 chan,FH_VPU_EXT_CHN_CONFIG *pstChnconfig);

/**
 * @brief         设置视频处理模块通道输出组织格式
 *
 * @param[in]    grpidx      GROUP号
 * @param[in]    chan        通道号
 * @param[in]    mode        图像数据输出组织格式
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 各模块支持的组织格式在各平台有所不同,详见《视频函数开发参考手册》。
 *
 */
FH_SINT32 FH_VPSS_SetVOMode(FH_UINT32 grpidx,FH_UINT32  chan, FH_VPU_VO_MODE mode);

/**
 * @brief         根据后续编码模块处理的旋转角度设置视频处理模块通道输出组织方式
 *
 * @param[in]    grpidx      GROUP号
 * @param[in]    chan        通道号
 * @param[in]    rotate      后续编码模块需要进行的旋转角度
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 此接口不会对输出数据进行旋转，而是根据编码旋转角度的需求对数据存放位置做一些调整
 *    - 仅在Tile压缩时此接口才会对输出有影响，其他格式实际存放在配置不同角度时不变。但接口通用于各个格式。
 *
 */
FH_SINT32 FH_VPSS_SetVORotate(FH_UINT32 grpidx,FH_UINT32 chan, FH_ROTATE_OPS rotate);

/**
 * @brief         开启通道使能
 *
 * @param[in]    grpidx      GROUP号
 * @param[in]    chan        通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_OpenChn(FH_UINT32 grpidx,FH_UINT32  chan);

/**
 * @brief         关闭通道使能
 *
 * @param[in]    grpidx      GROUP号
 * @param[in]    chan        通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_CloseChn(FH_UINT32 grpidx,FH_UINT32  chan);

/**
 * @brief         设置视频处理模块隐私屏蔽属性
 *
 * @param[in]    grpidx              GROUP号
 * @param[in]    pstVpumaskinfo      隐私屏蔽属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 各平台支持功能有所差异,详见《视频函数开发参考手册》。
 *
 */
FH_SINT32 FH_VPSS_SetMask(FH_UINT32 grpidx,const FH_VPU_MASK *pstVpumaskinfo);

/**
 * @brief         获取视频处理模块隐私屏蔽属性
 *
 * @param[in]    grpidx               GROUP号
 * @param[out]    pstVpumaskinfo      隐私屏蔽属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_GetMask(FH_UINT32 grpidx,FH_VPU_MASK *pstVpumaskinfo);

/**
 * @brief         关闭对应隐私屏蔽区域
 *
 * @param[in]    grpidx             GROUP号
 * @param[in]    clearmaskarea      隐私屏蔽区域索引,范围0-7
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_ClearMask(FH_UINT32 grpidx,FH_UINT32 clearmaskarea);

/**
 * @brief         开启均值统计
 *
 * @param[in]    grpidx      GROUP号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_EnableYCmean(FH_UINT32 grpidx);

/**
 * @brief         关闭均值统计
 *
 * @param[in]    grpidx      GROUP号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_DisableYCmean(FH_UINT32 grpidx);

/**
 * @brief         配置均值统计下采样模式
 *
 * @param[in]    grpidx      GROUP号
 * @param[in]    mode        下采样模式(4/8/16)
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_SetYCmeanMode(FH_UINT32 grpidx,FH_UINT32 mode);

/**
 * @brief         配置均值统计下采样模式
 *
 * @param[in]     grpidx      GROUP号
 * @param[out]    mode        下采样模式(4/8/16)
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_GetYCmeanMode(FH_UINT32 grpidx,FH_UINT32 *mode);

/**
 * @brief         获取均值统计结果
 *
 * @param[in]     grpidx                GROUP号
 * @param[out]    pstVpuycmeaninfo      均值统计结果
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_GetYCmean(FH_UINT32 grpidx,FH_VPU_YCMEAN *pstVpuycmeaninfo);

/**
 * @brief         设置帧率控制参数
 *
 * @param[in]    grpidx               GROUP号
 * @param[in]    chan                 通道号
 * @param[in]    pstVpuframectrl      帧率控制参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 通道处于特殊工作模式时不支持帧率控制。
 *    - 帧率控制时,图像时间戳将被修改为目标帧间隔。
 *    - 只有在通道缓存大于等于3个时,才支持增帧模式。
 *
 */
FH_SINT32 FH_VPSS_SetFramectrl(FH_UINT32 grpidx,FH_UINT32 chan,const FH_FRAMERATE *pstVpuframectrl);

/**
 * @brief         获取帧率控制参数
 *
 * @param[in]    grpidx               GROUP号
 * @param[in]    chan                 通道号
 * @param[out]   pstVpuframectrl      帧率控制参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_GetFramectrl(FH_UINT32 grpidx,FH_UINT32 chan, FH_FRAMERATE *pstVpuframectrl);

/**
 * @brief         获取当前通道帧率统计结果
 *
 * @param[in]    grpidx    GROUP号
 * @param[in]    chan      通道号
 * @param[out]   fps       帧率统计结果
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_GetFrameRate(FH_UINT32 grpidx,FH_UINT32 chan,FH_FRAMERATE *fps);

/**
 * @brief         设置通道裁剪属性
 *
 * @param[in]    grpidx              GROUP号
 * @param[in]    chan                通道号
 * @param[in]    sel                 位置
 * @param[in]    pstVpucropinfo      裁剪属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 各平台支持功能有所差异,详见《视频函数开发参考手册》。
 *
 */
FH_SINT32 FH_VPSS_SetChnCrop(FH_UINT32 grpidx,FH_UINT32 chan,FH_VPU_CROP_SEL sel,const FH_VPU_CROP *pstVpucropinfo);

/**
 * @brief         获取通道裁剪属性
 *
 * @param[in]    grpidx              GROUP号
 * @param[in]    chan                通道号
 * @param[in]    sel                 位置
 * @param[out]   pstVpucropinfo      裁剪属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_GetChnCrop(FH_UINT32 grpidx,FH_UINT32 chan,FH_VPU_CROP_SEL sel,FH_VPU_CROP * pstVpucropinfo);

/**
 * @brief         设置VI裁剪属性
 *
 * @param[in]    grpidx              GROUP号
 * @param[in]    sel                 位置
 * @param[in]    pstVpucropinfo      裁剪属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 各平台支持功能有所差异,详见《视频函数开发参考手册》。
 *
 */
FH_SINT32 FH_VPSS_SetGlbCrop(FH_UINT32 grpidx,FH_VPU_CROP_SEL sel,const FH_VPU_CROP *pstVpucropinfo);

/**
 * @brief         获取VI裁剪属性
 *
 * @param[in]     grpidx              GROUP号
 * @param[in]     sel                 位置
 * @param[out]    pstVpucropinfo      裁剪属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_GetGlbCrop(FH_UINT32 grpidx,FH_VPU_CROP_SEL sel,FH_VPU_CROP * pstVpucropinfo);

/**
 * @brief         设置图像缩放的滤波器系数,可以将图像效果从平滑到锐利。
 *                建议输入输出缩放比差不多或放大时，设的较强。反之设弱些。
 *                另外在编码压力较大时可以使用此接口对图像进行一定程度滤波降低编码压力。
 *
 * @param[in]    grpidx      GROUP号
 * @param[in]    chan        通道号
 * @param[in]    level       滤波等级，0-15,越大越清晰
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_SetScalerCoeff(FH_UINT32 grpidx,FH_UINT32 chan, FH_UINT32 level);

/**
 * @brief      设置图像缩放相关信息
 *
 * @param[in]  grpidx  GROUP号
 * @param[in]  chan    通道号
 * @param[in]  attr    缩放相关信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 当图像缩放比比较大时会出现锯齿效应，可使用此接口调整参数改善。
 *    - Fix2ds是scaler输出目标幅面*2的大小后进行2x2的均值得到目标幅面。
 *    - 目标幅面宽度小于配置宽度且小于通道最大幅面一半，高度小于2048时开启Fix2ds。
 *    - 若选择自动模式，需要在 #FH_VPSS_SetChnAttr 后调用。
 *
 */
FH_SINT32 FH_VPSS_SetChnScalerAttr(FH_UINT32 grpidx,FH_UINT32 chan,const FH_VPU_SCALER_ATTR *attr);

/**
 * @brief      获取图像缩放相关信息。
 *
 * @param[in]  grpidx  GROUP号
 * @param[in]  chan    通道号
 * @param      attr    缩放相关信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 */
FH_SINT32 FH_VPSS_GetChnScalerAttr(FH_UINT32 grpidx,FH_UINT32 chan,FH_VPU_SCALER_ATTR *attr);


/**
 * @brief         获取对应通道设的内存块数量需要的内存大小。供快速启动应用增加临时输出缓存数量。
 *
 * @param[in]    grpidx          GROUP号
 * @param[in]    chan            通道号
 * @param[in]    bufnum          增加buf数量
 * @param[out]    needsize       需要分配的内存数量
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_GetFrameBufferSize(FH_UINT32 grpidx,FH_UINT32 chan,FH_UINT32 bufnum,FH_UINT32 *needsize);

/**
 * @brief         给通道输出注册临时内存
 *
 * @param[in]    grpidx      GROUP号
 * @param[in]    chan        通道号
 * @param[in]    bufnum      增加buf数量
 * @param[in]    buf         临时内存的信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_FrameBufferRegister(FH_UINT32 grpidx,FH_UINT32 chan,FH_UINT32 bufnum,FH_MEM_INFO buf);
/**
 * @brief         将通道输出临时内存注销
 *
 * @param[in]     grpidx      GROUP号
 * @param[in]     chan        通道号
 * @param[out]    buf         临时内存的信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 此函数需要等待内存释放,带有阻塞等待,请注意调用时机。
 *
 */
FH_SINT32 FH_VPSS_FrameBufferUnRegister(FH_UINT32 grpidx,FH_UINT32 chan,FH_MEM_INFO *buf);

/**
 * @brief         获取不同幅面和支持输出类型的内存大小
 *
 * @param[in]    width              宽度
 * @param[in]    height             高度
 * @param[in]    support_mode       支持的输出类型
 * @param[in]    blk_size           每块内存需要的Byte
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - VPU通道使用VBPool内存时，需在 #FH_VPSS_CreateChn 时将内存bufnum配置为0。
 *    - support_mode 的值是 1 << format 的格式，而不是直接赋值format
 *
 */
FH_SINT32 FH_VPSS_GetFrameBlkSize(FH_UINT32 width,FH_UINT32 height,FH_UINT32 support_mode,FH_UINT32 *blk_size);

/**
 * @brief         将VPSS的通道绑定到某个视频缓存VB池中
 *
 * @param[in]    grpidx      GROUP号
 * @param[in]    chan        通道号
 * @param[in]    vbpool      视频缓存VB池信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - VPU通道使用VBPool内存时，需在 #FH_VPSS_CreateChn 时将内存bufnum配置为0。
 *    - 可以通过 #FH_VPSS_GetFrameBlkSize 获取到不同幅面和格式的帧内存大小
 *    - 也可以在创建通道后通过 #FH_VPSS_FrameBufferRegister 获取当前通道每帧需要的内存大小.
 *    - 支持多个Grp的多个通道共用VB,但建议在内存大小相近时才进行共用
 *    - 如需切换绑定的VB只需重新调用 #FH_VPSS_AttachVbPool
 *    - 基于用户获取通道数据和帧率控制等需求，VPU会占用最新的一帧，请合理分配VB的BLK数量
 *
 */
FH_SINT32 FH_VPSS_AttachVbPool(FH_UINT32 grpidx,FH_UINT32 chan,FH_UINT32 vbpool);

/**
 * @brief         将VPSS的通道从某个视频缓存VB池中解绑
 *
 * @param[in]    grpidx      GROUP号
 * @param[in]    chan        通道号
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 调用此接口返回后,相关VB内存依然有可能被各模块使用,需要用户自行保证相关内存使用完毕后才能销毁
 *
 */
FH_SINT32 FH_VPSS_DetachVbPool(FH_UINT32 grpidx,FH_UINT32 chan);

/**
 * @brief         设置是否允许VPU通道使用CommonVB
 *
 * @param[in]    grpidx       GROUP号
 * @param[in]    chan         通道号
 * @param[in]    enable       使能,默认为FH_TRUE
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 开启VB模式必须在 #FH_VPSS_CreateChn 配置bufnum为0,否则内存将从VMM分配.
 *    - 使能CommVB后,会从CommVB获取内存，但如果调用 #FH_VPSS_AttachVbPool ，会使用指定的私有VB不再从CommVB中申请．
 *    - #FH_VPSS_DetachVbPool 后，如果使能CommVB会从私有VB切换到CommVB获取．如未使能CommVB则将触发丢帧，等待重新Attach新的私有VB．
 *
 */
FH_SINT32 FH_VPSS_SetCommVbEnable(FH_UINT32 grpidx,FH_UINT32 chan,FH_UINT32 enable);

/**
 * @brief         修改各个通道默认的缩放大小，必须在配置vi属性和通道属性之前，用于一些小幅面高帧率的场景。
 *                在设置通道属性时对应通道的默认大小会被修改。
 *
 * @param[in]    grpidx       GROUP号
 * @param[in]    picsize      默认输出幅面
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_SetDefaultScalerSize(FH_UINT32 grpidx,FH_SIZE *picsize);

/**
 * @brief         设置镜头畸变校正(LDC)属性
 *
 * @param[in]    grpidx        GROUP号
 * @param[in]    pLdcAttr      LDC属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_SetLDCAttr(FH_UINT32 grpidx,FH_VPU_LDC *pLdcAttr);

/**
 * @brief         获取镜头畸变校正(LDC)属性
 *
 * @param[in]     grpidx         GROUP号
 * @param[out]    pLdcAttr       LDC属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_GetLDCAttr(FH_UINT32 grpidx,FH_VPU_LDC *pLdcAttr);

/*内部接口,不建议用户调用此接口*/
FH_SINT32 FH_VPU_GetPkginfo(FH_UINT32 grpidx,FH_SINT32 intMemSlot,FH_PKG_INFO *pstVpupkginfo);

/**
 * @brief         设置全局图形叠加信息。
 *
 * @param[in]    grpidx               GROUP号
 * @param[in]    pstVpugraphinfo      图形叠加信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 需要在 #FH_VPSS_SetViAttr 后调用。
 *
 */
FH_SINT32 FH_VPSS_SetGlbGraphV2(FH_UINT32 grpidx,const FH_VPU_LOGOV2 *pstVpugraphinfo);

/**
 * @brief      设置全局图形叠加信息,设置生效后返回。
 *
 * @param[in]  grpidx           GROUP号
 * @param[in]  timeout_ms       接口阻塞时间(ms)
 * @param[in]  pstVpugraphinfo  图形叠加信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 需要在 #FH_VPSS_SetViAttr 后调用。
 */
FH_SINT32 FH_VPSS_SetGlbGraphV2_Timeout(FH_UINT32 grpidx,FH_UINT32 timeout_ms,const FH_VPU_LOGOV2 *pstVpugraphinfo);
/**
 * @brief         获取全局图形叠加信息。
 *
 * @param[in]     grpidx               GROUP号
 * @param[out]    pstVpugraphinfo      图形叠加信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_GetGlbGraphV2(FH_UINT32 grpidx,FH_VPU_LOGOV2 * pstVpugraphinfo);

/**
 * @brief         设置通道图形叠加信息。
 *
 * @param[in]     grpidx               GROUP号
 * @param[in]     chan                 通道号
 * @param[out]    pstVpugraphinfo      图形叠加信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 需要在 #FH_VPSS_SetChnAttr 后调用。
 */
FH_SINT32 FH_VPSS_SetChnGraphV2(FH_UINT32 grpidx, FH_UINT32 chan, const FH_VPU_LOGOV2 *pstVpugraphinfo);


/**
 * @brief      设置通道图形叠加信息,设置生效后返回。
 *
 * @param[in]  grpidx           GROUP号
 * @param[in]  chan             通道号
 * @param[in]  timeout_ms       接口阻塞时间(ms)
 * @param[in]  pstVpugraphinfo  图形叠加信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode}
 *
 * @note
 *    - 需要在 #FH_VPSS_SetChnAttr 后调用。
 */
FH_SINT32 FH_VPSS_SetChnGraphV2_Timeout(FH_UINT32 grpidx,FH_UINT32 chan,FH_UINT32 timeout_ms, const FH_VPU_LOGOV2 *pstVpugraphinfo);

/**
 * @brief         获取通道图形叠加信息。
 *
 * @param[in]     grpidx               GROUP号
 * @param[in]     chan                 通道号
 * @param[out]    pstVpugraphinfo      图形叠加信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_GetChnGraphV2(FH_UINT32 grpidx,FH_UINT32 chan, FH_VPU_LOGOV2 * pstVpugraphinfo);

/**
 * @brief         检查VPSS输入缓存地址是否被使用
 *
 * @param[in]    grpidx          GROUP号
 * @param[in]    addr            查询地址
 * @param[in]    size            查询大小
 * @param[in]    timeout_ms      接口阻塞时间(ms)
 * @param[in]    isidle          是否正在被使用
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_CheckAddr(FH_UINT32 grpidx,FH_PHYADDR addr,FH_UINT32 size,FH_UINT32 timeout_ms,FH_UINT32 *isidle);

/**
 * @brief         设置通道输入图像源位置
 *
 * @param[in]    grpidx      GROUP号
 * @param[in]    chan        通道号
 * @param[in]    sel         输入图像源位置
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 此接口仅适用于支持选择输入源的通道，参考开发文档VPU框图。
 *    - AI-RGB通道 sel: 0->LDC之后 1->PM之后 2->Gosd之后
 *
 */
FH_SINT32 FH_VPSS_SetChnViSel(FH_UINT32 grpidx,FH_UINT32 chan,FH_UINT32 sel);


/**
 * @brief     设置group模块参数
 *
 * @param[in]  grpidx  GROUP号
 * @param[in]  attr    group模块参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
*     - 修改分块和shadow需要在 #FH_VPSS_SetViAttr 前，vi_auto可动态修改
 */
FH_SINT32 FH_VPSS_SetGrpParam(FH_UINT32 grpidx,const FH_VPU_GRP_PARAM *attr);
/**
 * @brief      获取group模块参数
 *
 * @param[in]  grpidx  GROUP号
 * @param      attr    group模块参数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 */
FH_SINT32 FH_VPSS_GetGrpParam(FH_UINT32 grpidx,FH_VPU_GRP_PARAM *attr);
/**
 * @brief      设置通道相关信息
 *
 * @param[in]  grpidx  GROUP号
 * @param[in]  chan    通道号
 * @param[in]  attr    通道相关信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 修改fcmode需要在 #FH_VPSS_SetFramectrl 前，修改align_patch_mode要在 #FH_VPSS_SetChnAttr 前
 */
FH_SINT32 FH_VPSS_SetChnParam(FH_UINT32 grpidx,FH_UINT32 chan,const FH_VPU_CHN_PARAM *attr);
/**
 * @brief      获取通道相关信息
 *
 * @param[in]  grpidx  GROUP号
 * @param[in]  chan    通道号
 * @param      attr    通道相关信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 */
FH_SINT32 FH_VPSS_GetChnParam(FH_UINT32 grpidx,FH_UINT32 chan,FH_VPU_CHN_PARAM *attr);

/**
 * @brief         设置通道 APC属性
 *
 * @param[in]    grpidx      GROUP号
 * @param[in]    chan        通道号
 * @param[in]    attr        APC属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 此接口仅适用于AI-RGB通道。
 *
 */
FH_SINT32 FH_VPSS_SetChnApcAttr(FH_UINT32 grpidx,FH_UINT32 chan,const FH_VPU_APC *attr);

/**
 * @brief         获取通道 APC属性
 *
 * @param[in]     grpidx      GROUP号
 * @param[in]     chan        通道号
 * @param[out]    attr        APC属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 此接口仅适用于AI-RGB通道。
 *
 */
FH_SINT32 FH_VPSS_GetChnApcAttr(FH_UINT32 grpidx,FH_UINT32 chan,FH_VPU_APC *attr);

/**
 * @brief         设置RGB前处理属性
 *
 * @param[in]    grpidx      GROUP号
 * @param[in]    chan        通道号
 * @param[in]    attr        RGB前处理属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 此接口仅适用于AI-RGB通道。
 *
 */
FH_SINT32 FH_VPSS_SetRGBPreAttr(FH_UINT32 grpidx,FH_UINT32 chan,const FH_VPU_RGB_PRE *attr);

/**
 * @brief         获取RGB前处理属性
 *
 * @param[in]     grpidx      GROUP号
 * @param[in]     chan        通道号
 * @param[out]    attr        RGB前处理属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 此接口仅适用于AI-RGB通道。
 *
 */
FH_SINT32 FH_VPSS_GetRGBPreAttr(FH_UINT32 grpidx,FH_UINT32 chan,FH_VPU_RGB_PRE *attr);

/**
 * @brief         设置拼接属性
 *
 * @param[in]    grpidx      GROUP号
 * @param[in]    attr        拼接属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 部分平台不支持,详见模块能力级。
 *
 */
FH_SINT32 FH_VPSS_SetMergeAttr(FH_UINT32 grpidx,const FH_VPU_MERGE_ATTR *attr);

/**
 * @brief         获取拼接属性
 *
 * @param[in]     grpidx      GROUP号
 * @param[out]    attr        拼接属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 *
 */
FH_SINT32 FH_VPSS_GetMergeAttr(FH_UINT32 grpidx,FH_VPU_MERGE_ATTR *attr);
/**
 * @brief         设置2DLUT属性
 *
 * @param[in]     grpidx      GROUP号
 * @param[out]    attr        2DLUT属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 部分平台不支持,详见模块能力级。
 *
 */
FH_SINT32 FH_VPSS_SetLut2dAttr(FH_UINT32 grpidx,const FH_VPU_LUT2D_ATTR *attr);
/**
 * @brief         获取2DLUT属性
 *
 * @param[in]     grpidx      GROUP号
 * @param[out]    attr        2DLUT属性
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 部分平台不支持,详见模块能力级。
 *
 */

FH_SINT32 FH_VPSS_GetLut2dAttr(FH_UINT32 grpidx,FH_VPU_LUT2D_ATTR *attr);

/**
 * @brief      设置调用vgs进行旋转的旋转角度
 *
 * @param[in]  grpidx  GROUP号
 * @param[in]  chan    通道号
 * @param[in]  rotate  旋转角度
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 */
FH_SINT32 FH_VPSS_SetChnRotation(FH_UINT32 grpidx,FH_UINT32 chan, FH_ROTATE_OPS rotate);

/**
 * @brief      设置丢帧数目
 *
 * @param[in]  grpidx       GROUP号
 * @param[in]  drop_frames  丢帧的个数
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 */
FH_SINT32 FH_VPSS_SetDropFrames(FH_UINT32 grpidx,FH_UINT32 drop_frames);

/**
 * @brief      获取指定通道的运行信息
 *
 * @param[in]  grpidx      GROUP号
 * @param[in]  chan        通道号
 * @param[out] chn_status  通道的运行信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 */
FH_SINT32 FH_VPSS_GetChnStatus(FH_UINT32 grpidx,FH_UINT32 chan, FH_VPU_CHN_STATUS* chn_status);


/**
 * @brief      执行VPU扩展命令
 *
 * @param[in]  grpidx  GROUP号
 * @param[in]  chan    通道号
 * @param[in]  cmd     扩展命令
 * @param[in,out] param   扩展命令参数
 * @param[in]  size    扩展命令参数长度
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    此API主要用来扩展标准API的功能，可以在只更新驱动和应用的情况下，透过MPI LIB，直接发送命令和参数到驱动
 */
FH_SINT32 FH_VPSS_ExtCmd(FH_UINT32 grpidx,FH_UINT32 chan, FH_UINT32 cmd, void *param, FH_UINT32 size);


/**
 * @brief      VPU进入休眠状态
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 *
 * @note
 *    - 这个API仅仅在RT-Thread下支持，linux上调用该API无实际效果。
 */

FH_SINT32 FH_VPSS_Suspend(void);

/**
 * @brief       VPU从休眠状态恢复，进入工作状态，保留原来的参数设置
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,vpu_errcode} 。
 * @note
 *    - 这个API仅仅在RT-Thread下支持，linux上调用该API无实际效果。
 *
 */
FH_SINT32 FH_VPSS_Resume(void);


/*内部调试函数，不建议用户调用*/
FH_SINT32 FH_VPSS_ReadMallocedMem(FH_UINT32 grpidx,FH_SINT32 intMemSlot, FH_UINT32 offset, FH_UINT32 *pstData);
FH_SINT32 FH_VPSS_WriteMallocedMem(FH_UINT32 grpidx,FH_SINT32 intMemSlot, FH_UINT32 offset, FH_UINT32 *pstData);
FH_SINT32 FH_VPSS_ImportMallocedMem(FH_UINT32 grpidx,FH_SINT32 intMemSlot, FH_UINT32 offset, FH_UINT32 *pstSrc, FH_UINT32 size);
FH_SINT32 FH_VPSS_ExportMallocedMem(FH_UINT32 grpidx,FH_SINT32 intMemSlot, FH_UINT32 offset, FH_UINT32 *pstDst, FH_UINT32 size);

FH_SINT32 FH_VPSS_SetChnAttr_Ex(FH_UINT32 grpidx,FH_UINT32  chan,const FH_VPU_CHN_CONFIG_EX *pstChnconfig);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
#endif /*__MPI_VO_H__ */
