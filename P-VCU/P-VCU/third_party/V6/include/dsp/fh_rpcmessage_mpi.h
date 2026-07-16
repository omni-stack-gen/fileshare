#ifndef __FH_VB_MPI_H__
#define __FH_VB_MPI_H__

#include "fh_rpcmessage_mpipara.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


/**
 * @brief         初始化视xbus通信
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ttyxbus_errcode} 。
 *
 * @note
 *    - 必须先调用 #FH_VB_SetConf 配置缓存池属性，否则会失败。
 *    - 可重复初始化。
 *
 */
FH_SINT32 FH_RPC_Message_Init(FH_VOID);

/**
 * @brief         去初始化xbus通信
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ttyxbus_errcode} 。
 *
 * @note
 *    - 建议在整个系统去初始化后，再去初始化缓存池。
 *    - 必须先去初始化已经创建的模块公共视频缓存池，再去初始化缓存池，否则返回失败。
 *    - 去初始化不会清除先前对缓存池的配置。
 *    - 可以反复去初始化。
 *
 */
FH_SINT32 FH_RPC_Message_Exit(FH_VOID);

/**
 * @brief         发送xbus信息
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ttyxbus_errcode} 。
 *
 * @note
 *    - 无。
 *
 */
FH_SINT32 FH_RPC_SendMessage(RPC_MESSAGE_CONF_S *pstTBConf);

/**
 * @brief         获取xbus信息
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ttyxbus_errcode} 。
 *
 * @note
 *    - 先 #FH_VB_SetConf 后才能获取得到。
 *
 */
FH_SINT32 FH_RPC_GetMessage(RPC_MESSAGE_CONF_S *pstTBConf);

/**
 * @brief         将xbus信息传给对方注册的函数
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ttyxbus_errcode} 。
 *
 * @note
 *    - 先 #FH_VB_SetConf 后才能获取得到。
 *
 */
FH_SINT32 FH_RPC_CallMessage(RPC_MESSAGE_CONF_S *pstRpcMsgConf);

/**
 * @brief         注册被调用的callback函数
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ttyxbus_errcode} 。
 *
 * @note
 *    - 先FH_VB_SetConf后才能获取得到。
 *
 */
FH_SINT32 FH_RPC_RegisterMessage(int core, int (*callback)(RPC_MESSAGE_CONF_S *pstTtyXbusConf));

/**
 * @brief         删除注册被调用的callback函数
 *
 *
 * @retval  0              成功
 * @retval  "非0"          失败，其值参见 @namelink{错误码,ttyxbus_errcode} 。
 *
 * @note
 *    - 先 #FH_VB_SetConf 后才能获取得到。
 *
 */
FH_SINT32 FH_RPC_UnRegisterMessage(int core);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /*__FH_VB_MPI_H__ */

