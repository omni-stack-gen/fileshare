#ifndef __FH_SYSTEM_MPIPARM_H__
#define __FH_SYSTEM_MPIPARM_H__

#include "fh_errno.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */


/** @brief 0xA0058002: 参数空指针错误 */
#define FH_ERR_SYS_NULL_PTR         FH_DEF_ERR(FH_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/** @brief 0xA0058002: 系统控制属性未配置 */
#define FH_ERR_SYS_NOTREADY         FH_DEF_ERR(FH_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
/** @brief 0xA0058002: 操作不允许 */
#define FH_ERR_SYS_NOT_PERM         FH_DEF_ERR(FH_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/** @brief 0xA0058002: 分配内存失败 */
#define FH_ERR_SYS_NOMEM            FH_DEF_ERR(FH_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/** @brief 0xA0058002: 参数设置无效 */
#define FH_ERR_SYS_ILLEGAL_PARAM    FH_DEF_ERR(FH_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/** @brief 0xA0058002: 系统忙 */
#define FH_ERR_SYS_BUSY             FH_DEF_ERR(FH_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
/** @brief 0xA0058002: 操作不支持 */
#define FH_ERR_SYS_NOT_SUPPORT      FH_DEF_ERR(FH_ID_SYS, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif  /* __FH_SYSTEM_MPIPARM_H__ */

