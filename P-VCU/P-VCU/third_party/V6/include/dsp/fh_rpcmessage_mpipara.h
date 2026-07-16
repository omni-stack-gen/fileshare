#ifndef __FH_TTYXBUS_MPIPARA_H__
#define __FH_TTYXBUS_MPIPARA_H__
/**|TTYXBUS|**/
#include "fh_errno.h"
#include "fh_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


typedef struct
{
	unsigned int core;
    unsigned int cmd;
	int len;
    unsigned char* argv;

} RPC_MESSAGE_CONF_S;

//typedef struct
//{
//	struct rpc_hdr		hdr;
//	RPC_MESSAGE_CONF_S  io;
//
//} RPC_MESSAGE_COMM_S;

///** @brief 0xA0018006: 参数空指针错误 */
//#define FH_ERR_TTYXBUS_NULL_PTR          FH_DEF_ERR(FH_ID_TTYXBUS, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
///** @brief 0xA001800c: 分配内存失败 */
//#define FH_ERR_TTYXBUS_NOMEM             FH_DEF_ERR(FH_ID_TTYXBUS, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)

typedef struct
{
	int rpc_switch;
    int (*callback)(RPC_MESSAGE_CONF_S* param);

} RPC_MESSAGE_CALL_S;


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif  /* __FH_TTYXBUS_MPIPARA_H__ */

