#ifndef  __FH_COMM_AENC_H__
#define  __FH_COMM_AENC_H__

#include "fh_common.h"
#include "fh_aio_mpipara.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

typedef enum fyEN_AENC_ERR_CODE_E
{
	AENC_ERR_ENCODER_ERR     = 64 ,
	AENC_ERR_VQE_ERR        = 65
} EN_AENC_ERR_CODE_E;

typedef enum fyAENC_PROC_E
{
	AENC_CHN_STATUS,
	AENC_EN_TYPE,
	AENC_FRAME_CNT,
	AENC_FRAME_SIZE,
	AENC_ENCODE_SIZE,
	AENC_FS_RATE,
	AENC_CHN_WIDTH,
	AENC_TRACK_MODE,
	AENC_DATA_STATUS,
	AENC_PROC_BUTT
}AENC_PROC_E;

#define MAX_AENC_FRM_CNT	300
#define AENC_MAX_CHN_NUM	32

typedef struct fyAENC_CH_ATTR_S
{
	PAYLOAD_TYPE_E		enType;			//!< 编码类型
	FH_UINT32		u32FrmCnt;		//!< 编码缓存帧个数
	FH_UINT32		u32FrmSize;		//!< 编码缓存帧大小
	AUDIO_SAMPLE_RATE_E	fs_rate;		//!< 采样率
	AUDIO_BIT_WIDTH_E	chn_width;		//!< 采样精度
	AUDIO_SOUND_MODE_E	track_mode;		//!< 声道模式
	FH_VOID			*pValue;		//!< 内部参数，无需配置
}AENC_CHN_ATTR_S;

/***************************follow is unsurpport function******************************/
typedef struct fyAENC_ENCODER_S
{
	PAYLOAD_TYPE_E		enType;
	FH_UINT32		u32MaxFrmLen;
	FH_CHAR			aszName[16];
	FH_SINT32		(*pfnOpenEncoder)(FH_VOID *pEncoderAttr, FH_VOID **ppEncoder);
	FH_SINT32		(*pfnEncodeFrm)(FH_VOID *pEncoder, const AUDIO_FRAME_S *pstData,
				FH_UINT8 *pu8Outbuf,FH_UINT32 *pu32OutLen);
	FH_SINT32		(*pfnCloseEncoder)(FH_VOID *pEncoder);
} AENC_ENCODER_S;

typedef struct _Proc_t
{
	FH_UINT32		u32ProcInfo[AENC_PROC_BUTT];
}PROC_T;

typedef struct _AENCProcInfo_s
{
	PROC_T			chn[AENC_MAX_CHN_NUM];
}AENC_PROC_INFO_S;

/** 无效的设备 	ID */
#define FH_ERR_AENC_INVALID_DEVID	FH_DEF_ERR(FH_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
/** 无效的通道 	ID */
#define FH_ERR_AENC_INVALID_CHNID	FH_DEF_ERR(FH_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/** 非法传入参数 */
#define FH_ERR_AENC_ILLEGAL_PARAM	FH_DEF_ERR(FH_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/** 通道已创建 */
#define FH_ERR_AENC_EXIST		FH_DEF_ERR(FH_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
/** 通道未创建 */
#define FH_ERR_AENC_UNEXIST		FH_DEF_ERR(FH_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/** 参数指针为空 */
#define FH_ERR_AENC_NULL_PTR		FH_DEF_ERR(FH_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/** 参数未配置 */
#define FH_ERR_AENC_NOT_CONFIG		FH_DEF_ERR(FH_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
/** 不支持的参数/接口 */
#define FH_ERR_AENC_NOT_SUPPORT		FH_DEF_ERR(FH_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
/** 无效的操作 */
#define FH_ERR_AENC_NOT_PERM		FH_DEF_ERR(FH_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/** 内存申请失败 */
#define FH_ERR_AENC_NOMEM		FH_DEF_ERR(FH_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/** 无内存 */
#define FH_ERR_AENC_NOBUF		FH_DEF_ERR(FH_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
/** 缓存为空 */
#define FH_ERR_AENC_BUF_EMPTY		FH_DEF_ERR(FH_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
/** 缓存已满 */
#define FH_ERR_AENC_BUF_FULL		FH_DEF_ERR(FH_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/** 系统未初始化 */
#define FH_ERR_AENC_SYS_NOTREADY	FH_DEF_ERR(FH_ID_AENC, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
/** 编码出错 */
#define FH_ERR_AENC_ENCODER_ERR		FH_DEF_ERR(FH_ID_AENC, EN_ERR_LEVEL_ERROR, AENC_ERR_ENCODER_ERR)
/** VQE 内部错误 */
#define FH_ERR_AENC_VQE_ERR		FH_DEF_ERR(FH_ID_AENC, EN_ERR_LEVEL_ERROR, AENC_ERR_VQE_ERR)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif/* End of #ifndef __FH_COMM_AENC_H__*/
