#ifndef __FH_VB_MPIPARA_H__
#define __FH_VB_MPIPARA_H__
/**|VB|**/
#include "fh_errno.h"
#include "fh_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


#define VB_MAX_POOLS 256                //!< 最大的视频缓存池个数
#define VB_MAX_COMM_POOLS 16            //!< 最大的公共视频缓存池个数
#define VB_MAX_MOD_COMM_POOLS 16        //!< 最大的模块视频缓存池个数

#define VB_MAX_USER   VB_UID_BUTT

#define VB_INVALID_POOLID (-1UL)        //!< 无效的缓存池ID
#define VB_INVALID_HANDLE (-1UL)        //!< 无效的缓存块句柄

#define POOL_OWNER_COMMON   -1		/* Common pool use this owner id, module common pool use VB_UID as owner id */
#define POOL_OWNER_PRIVATE  -2		/* Private pool use this owner id */

#define VB_SUPPLEMENT_JPEG_MASK 0x1

typedef FH_UINT32 VB_POOL;
typedef FH_UINT32 VB_BLK;


typedef enum
{
    VB_UID_VIU          = 0,		//!< VIU标识
    VB_UID_VOU          = 1,		//!< VOU标识
    VB_UID_VGS          = 2,		//!< VGS标识
    VB_UID_VENC         = 3,		//!< VENC标识
    VB_UID_VDEC         = 4,		//!< VDEC标识
    VB_UID_VDA          = 5,		//!< VDA标识
    VB_UID_H264E        = 6,		//!< H264标识
    VB_UID_JPEGED       = 7,		//!< JPEGD标识
    VB_UID_MPEG4E       = 8,		//!< MPEG4E标识
    VB_UID_H264D        = 9,		//!< H264D标识
    VB_UID_JPEGD        = 10,		//!< JPEGE标识
    VB_UID_MPEG4D       = 11,		//!< MPEG4D标识
    VB_UID_VPSS         = 12,		//!< VPSS标识
    VB_UID_GRP          = 13,		//!< GRP标识
    VB_UID_MPI          = 14,		//!< MPI标识
    VB_UID_PCIV         = 15,		//!< PCIV标识
    VB_UID_AI           = 16,		//!< AI标识
    VB_UID_AENC         = 17,		//!< AENC标识
    VB_UID_RC           = 18,		//!< RC标识
    VB_UID_VFMW         = 19,		//!< VFMW标识
    VB_UID_USER         = 20,		//!< USER标识
    VB_UID_H265E        = 21,		//!< h265E标识
    VB_UID_BGM          = 22,		//!< BGM标识
    VB_UID_BGMX         = 23,		//!< BGMX标识
    VB_UID_BGMSW        = 24,		//!< BGMSW标识
    VB_UID_TDE          = 25,		//!< TDE标识
    VB_UID_IVE          = 26,		//!< IVE标识
    VB_UID_NNA          = 27,		//!< NNA标识
    VB_UID_ISP          = 28,		//!< ISP标识
    VB_UID_VGS2         = 29,		//!< VGS2标识
    VB_UID_VICAP        = 30,		//!< VGS2标识
    VB_UID_BUTT
} VB_UID_E;

typedef enum
{
    POOL_TYPE_COMMON            = 0,		//!< 公共缓存池类型
    POOL_TYPE_PRIVATE           = 1,		//!< 私有缓存池类型
    POOL_TYPE_MODULE_COMMON     = 2,		//!< 模块公共缓存池类型（目前只有VDEC可能用到）
    POOL_TYPE_BUTT
} POOL_TYPE_E;

typedef struct
{
    FH_UINT32 u32MaxPoolCnt;					//!< 系统中可容纳的缓存池数(目前固定为: #VB_MAX_POOLS )
    struct
    {
        FH_UINT32 u32BlkSize;					//!< 缓存块的大小
        FH_UINT32 u32BlkCnt;					//!< 缓存块个数
        FH_CHAR acMmzName[MAX_VMM_NAME_LEN];	//!< 缓存池所在的VMM名
    }astCommPool[VB_MAX_COMM_POOLS];
} VB_CONF_S;

/** @brief 0xA0018006: 参数空指针错误 */
#define FH_ERR_VB_NULL_PTR          FH_DEF_ERR(FH_ID_VB, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/** @brief 0xA001800c: 分配内存失败 */
#define FH_ERR_VB_NOMEM             FH_DEF_ERR(FH_ID_VB, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/** @brief 0xA001800d: 分配缓存失败 */
#define FH_ERR_VB_NOBUF             FH_DEF_ERR(FH_ID_VB, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
/** @brief 0xA0018005: 视频缓存池不存在 */
#define FH_ERR_VB_UNEXIST           FH_DEF_ERR(FH_ID_VB, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/** @brief 0xA0018003: 参数设置无效 */
#define FH_ERR_VB_ILLEGAL_PARAM     FH_DEF_ERR(FH_ID_VB, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/** @brief 0xA0018010: 系统控制属性未配置 */
#define FH_ERR_VB_NOTREADY          FH_DEF_ERR(FH_ID_VB, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
/** @brief 0xA0018012: 系统忙 */
#define FH_ERR_VB_BUSY              FH_DEF_ERR(FH_ID_VB, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
/** @brief 0xA0018009: 操作不允许 */
#define FH_ERR_VB_NOT_PERM          FH_DEF_ERR(FH_ID_VB, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/** @brief 0xA0018040: 创建缓存池太多 */
#define FH_ERR_VB_2MPOOLS           FH_DEF_ERR(FH_ID_VB, EN_ERR_LEVEL_ERROR, EN_ERR_BUTT + 1)

#define FY_TRACE_VB(level,fmt...)   FY_TRACE(level, FH_ID_VB,##fmt)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif  /* __FH_VB_MPIPARA_H__ */

