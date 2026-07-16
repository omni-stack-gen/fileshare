#ifndef __FH_SYSTEM_MPI_H__
#define __FH_SYSTEM_MPI_H__

#include "types/type_def.h"
#include "fh_common.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/**
 * @brief         初始化MPP系统。打开各个设备驱动。
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,sys_errcode} 。
 *
 * @note
 *    - 对于各模块相关内存和功能裁剪需要在此函数之前调用。
 *
 */
FH_SINT32  FH_SYS_Init(FH_VOID);

/**
 * @brief         初始化MPP系统。打开部分设备驱动。
 *                适用于快速启动相关应用场景
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,sys_errcode} 。
 *
 * @note
 *    - 对于在此接口中打开的模块相关内存和功能裁剪需要在此函数之前调用。
 *
 */
FH_SINT32 FH_SYS_Init_Pre(FH_VOID);

/**
 * @brief         初始化MPP系统。打开FH_SYS_Init_Pre未被初始化的设备驱动。
 *                适用于快速启动相关应用场景
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,sys_errcode} 。
 *
 * @note
 *    - 对于在此接口中打开的模块相关内存和功能裁剪需要在此函数之前调用。
 *
 */
FH_SINT32 FH_SYS_Init_Post(FH_VOID);

/**
 * @brief         退出MPP系统。关闭各个设备驱动。释放内存资源。
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,sys_errcode} 。
 *
 *
 */
FH_SINT32  FH_SYS_Exit(FH_VOID);

/**
 * @brief         数据源到数据接收者的绑定接口
 *
 * @param[in]     src                数据源信息
 * @param[in]     dst                数据接收者信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,sys_errcode} 。
 *
 * @note
 *    - 支持的绑定关系和相关注意实现请参见系统控制章节相关说明
 *
 */
FH_SINT32 FH_SYS_Bind(FH_BIND_INFO src,FH_BIND_INFO dst);



/**
 * @brief         数据源到数据接收者的绑定接口
 *
 * @param[in]     src                数据源信息
 * @param[in]     dst                数据接收者信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,sys_errcode} 。
 *
 * @note
 *    - 解除数指定数据源和数据接收者的绑定关系
 *
 */
FH_SINT32 FH_SYS_UnBind(FH_BIND_INFO src,FH_BIND_INFO dst);

/**
 * @brief         解除数据源的所有绑定关系(存在多个接收者时也会全部解除)
 *
 * @param[in]     src                数据源信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,sys_errcode} 。
 *
 *
 */
FH_SINT32  FH_SYS_UnBindbySrc(FH_BIND_INFO src);

/**
 * @brief         解除数据接收者的所有绑定关系
 *
 * @param[in]     dst                数据接收者信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,sys_errcode} 。
 *
 *
 */
FH_SINT32 FH_SYS_UnBindbyDst(FH_BIND_INFO dst);

/**
 * @brief         根据接收者获取数据源的信息
 *
 * @param[out]    src                数据源信息
 * @param[in]     dst                数据接收者信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,sys_errcode} 。
 *
 *
 */
FH_SINT32 FH_SYS_GetBindbyDest(FH_BIND_INFO *src,FH_BIND_INFO dst);

/**
 * @brief         获取系统版本信息
 *
 * @param[out]    pstSystemversion   版本号等信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,sys_errcode} 。
 *
 *
 */
FH_SINT32  FH_SYS_GetVersion(FH_VERSION_INFO *pstSystemversion);

/**
 * @brief         获取DSP版本信息字符串
 *
 * @param[in]     print_enable       是否自动打印相关版本信息
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,sys_errcode} 。
 *
 *
 */
FH_CHAR *FH_DSP_Version(FH_UINT32 print_enable);

/**
 * @brief         获取芯片标识,芯片标识定义见fh_common.h
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,sys_errcode} 。
 *
 *
 */
FH_UINT32 FH_SYS_GetChipID(FH_VOID);

/**
 * @brief         设置寄存器/内存地址
 *
 * @param[in]     addr               物理地址
 * @param[out]    value              设置值
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,sys_errcode} 。
 *
 *
 */
FH_SINT32 FH_SYS_SetReg(FH_PHYADDR addr, FH_UINT32 value);

/**
 * @brief         获取寄存器/内存地址
 *
 * @param[in]     u32Addr            物理地址
 * @param[out]    pu32Value          实际值
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,sys_errcode} 。
 *
 *
 */
FH_SINT32 FH_SYS_GetReg(FH_PHYADDR u32Addr, FH_UINT32 *pu32Value);

/**
 * @brief       获取 MPP 的当前时间戳。
 *
 * @param[out]    pu64CurPts      当前时间戳指针。
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,sys_errcode} 。
 *
 * @note
 *    -  初始化时间戳基准会将当前系统的时间戳强制置成u64PtsBase，与系统原有时间戳没有任何约束。
 *       因此，建议在媒体业务没有启动时（例如操作系统刚启动），调用这个接口。
 *       如果媒体业务已经启动，建议调用 #FH_SYS_SyncPts 进行时间戳微调。
 *
 * @see
 *    -  FH_SYS_InitPtsBase
 *
 */
FH_SINT32 FH_SYS_GetCurPts(FH_UINT64 *pu64CurPts);

/**
 * @brief       初始化 MPP 的时间戳基准。
 *
 * @param[in]    u64PtsBase      时间戳基准。
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,sys_errcode} 。
 *
 * @note
 *    -  初始化时间戳基准会将当前系统的时间戳强制置成u64PtsBase，与系统原有时间戳没有任何约束。
 *       因此，建议在媒体业务没有启动时（例如操作系统刚启动），调用这个接口。
 *       如果媒体业务已经启动，建议调用 #FH_SYS_SyncPts 进行时间戳微调。
 *
 * @see
 *    -  FH_SYS_SyncPts
 *
 */
FH_SINT32 FH_SYS_InitPtsBase(FH_UINT64 u64PtsBase);

/**
 * @brief      同步 MPP 的时间戳。
 *
 * @param[in]  u64PtsBase  时间戳基准。
 *
 * @retval     0           成功
 * @retval     "非0"        失败，其值参见 @namelink{错误码,sys_errcode} 。
 *
 * @note
 *    - 对当前系统时间戳进行微调，微调后不会出现时间戳回退现象。
 *      在多片之间做同步时，由于单板的时钟源误差可能比较大，建议一秒钟进行一次时间戳微调。
 *
 * @see
 *    - FH_SYS_InitPtsBase
 *
 */
FH_SINT32 FH_SYS_SyncPts(FH_UINT64 u64PtsBase);

/**
 * @brief      获取Arc的心跳计数。
 *
 * @param[in]  u32Index  Arc索引（0，1）。
 * @param[in]  pu32HeartBeat  心跳计数。
 *
 * @retval     0           成功
 * @retval     "非0"        失败，其值参见 @namelink{错误码,sys_errcode} 。
 *
 * @note
 *    - 当获取的心跳计数值在增加时，说明对应的Arc正常，否则说明Arc已异常。
 *
 */
FH_SINT32 FH_SYS_GetArcHeartBeat(FH_UINT32 u32Index, FH_UINT32 *pu32HeartBeat);

/****
* @brief      获取arc的log。
*
* @param[in]  u32Index      Arc索引（0，1）。
* @param[in]  pBuf          log的buffer。
* @param[in]  u32Buflen     log的buffer长度。
*
* @retval     大于等于0         读取的实际size大小。
* @retval     小于0           失败。
*
* @note
*    - 传入arcid，logbuf，buflen之后，直接把arc的log写入buf，并返回实际size。
*      每一次调用后会从上次读的末尾向后延续。
****/
FH_SINT32 FH_SYS_GetArcLog(FH_UINT32 u32Index, FH_CHAR *pBuf, FH_UINT32 u32Buflen);

/****
* @brief      获取arc的runlog。
*
* @param[in]  u32Index      Arc索引（目前只支持0）。
* @param[in]  pBuf          获取runlog的buffer。
* @param[in]  u32Buflen     获取runlog的buffer长度。
*
* @retval     不小于0          读取的实际size。
* @retval     小于0           失败。
*
* @note
*    - 将arc的runlog写入pBuf，并返回实际读取的size;
*      每一次调用后会从上次读的末尾向后延续;
*      需要传入arc的运行地址，内部据此自动获取runlog的存放地址及长度。
****/
FH_SINT32 FH_SYS_GetArcRunlog(FH_UINT32 u32Index, FH_CHAR *pBuf, FH_UINT32 u32Buflen);

/**
 * @brief 获取媒体PTS时间
 *
 *
 *
 * @param[out] pts  获取到的PTS。
 *
 * @retval     0           成功
 * @retval     "非0"        失败，其值参见 @namelink{错误码,sys_errcode} 。
 *
 * @note
 *     - 此API用于获取媒体PTS时间，单位为微秒。
 *
 */
FH_SINT32 FH_SYS_GetTimeOfDay(unsigned long long *pts);

int FH_RPC_Customer_Command(unsigned int cmd, void* arg, unsigned int len);

int FH_RPC_APP_Command(unsigned int cmd, void* arg, unsigned int len);

int FH_RPC_ISP_Command(unsigned int cmd, void* arg, unsigned int len);

int FH_RPC_DSP_Command(unsigned int cmd, void* arg, unsigned int len);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
#endif /*__MPI_VO_H__ */

