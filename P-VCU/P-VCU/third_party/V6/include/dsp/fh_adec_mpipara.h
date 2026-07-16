#ifndef  __FH_COMM_ADEC_H__
#define  __FH_COMM_ADEC_H__

#include "fh_common.h"
#include "fh_aio_mpipara.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

typedef enum fyADEC_MODE_E
{
	ADEC_MODE_PACK = 0,		//!< 帧模式
	ADEC_MODE_STREAM ,		//!< 流模式，暂不支持
	ADEC_MODE_BUTT
}ADEC_MODE_E;

typedef enum fyEN_ADEC_ERR_CODE_E
{
	ADEC_ERR_DECODER_ERR	 = 64,
	ADEC_ERR_BUF_LACK
} EN_ADEC_ERR_CODE_E;

typedef enum fyADEC_PROC_E
{
	ADEC_CHN_STATUS,
	ADEC_EN_TYPE,
	ADEC_FRAME_CNT,
	ADEC_FRAME_SIZE,
	ADEC_FS_RATE,
	ADEC_CHN_WIDTH,
	ADEC_DECODE_MODE,
	ADEC_AAC_INVLID_SIZE,
	ADEC_TOTAL_POOL,
	ADEC_CACHE_SIZE,
	ADEC_READ_NUM,
	ADEC_WRITE_NUM,
	ADEC_LAST_WRITE_NUM,
	ADEC_GET_CNT,
	ADEC_LAST_GET_CNT,
	ADEC_PROC_BUTT
}ADEC_PROC_E;

#define MAX_ADEC_FRM_CNT	300
#define ADEC_MAX_CHN_NUM	32

typedef struct fyADEC_CH_ATTR_S
{
	PAYLOAD_TYPE_E		enType;		//!< 解码类型
	FH_UINT32		u32FrmCnt;	//!< 解码缓存帧个数
	FH_UINT32		u32FrmSize;	//!< 解码缓存帧大小
	AUDIO_SAMPLE_RATE_E	fs_rate;	//!< 采样率
	AUDIO_BIT_WIDTH_E	chn_width;	//!< 采样精度
	ADEC_MODE_E		enMode;		//!< 声道模式
	FH_VOID			*pValue;	//!< 内部参数，无需配置
}ADEC_CHN_ATTR_S;

typedef struct _ProcParam_s
{
	FH_UINT32		u32ProcInfo[ADEC_PROC_BUTT];
}PROC_PARAM_S;

typedef struct _ADECProcInfo_s
{
	PROC_PARAM_S		chn[ADEC_MAX_CHN_NUM];
}ADEC_PROC_INFO_S;

/******************************follow is unsurpport function***************************/
typedef struct fyADEC_DECODER_S
{
	PAYLOAD_TYPE_E		enType;
	FH_CHAR			aszName[16];
	FH_SINT32		(*pfnOpenDecoder)(FH_VOID *pDecoderAttr, FH_VOID **ppDecoder); /*struct ppDecoder is packed by user,user malloc and free memory for this struct */
	FH_SINT32		(*pfnDecodeFrm)(FH_VOID *pDecoder, FH_UINT8 **pu8Inbuf,FH_SINT32 *ps32LeftByte,
					FH_UINT16 *pu16Outbuf,FH_UINT32 *pu32OutLen,FH_UINT32 *pu32Chns);
	FH_SINT32		(*pfnGetFrmInfo)(FH_VOID *pDecoder, FH_VOID *pInfo);
	FH_SINT32		(*pfnCloseDecoder)(FH_VOID *pDecoder);
	FH_SINT32		(*pfnResetDecoder)(FH_VOID *pDecoder);
} ADEC_DECODER_S;

/** 无效的设备 	ID */
#define FH_ERR_ADEC_INVALID_DEVID     FH_DEF_ERR(FH_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
/** 无效的通道 	ID */
#define FH_ERR_ADEC_INVALID_CHNID     FH_DEF_ERR(FH_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/** 非法传入参数 */
#define FH_ERR_ADEC_ILLEGAL_PARAM     FH_DEF_ERR(FH_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/** 通道已创建 */
#define FH_ERR_ADEC_EXIST             FH_DEF_ERR(FH_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
/** 通道未创建 */
#define FH_ERR_ADEC_UNEXIST           FH_DEF_ERR(FH_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/** 参数指针为空 */
#define FH_ERR_ADEC_NULL_PTR          FH_DEF_ERR(FH_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/** 参数未配置 */
#define FH_ERR_ADEC_NOT_CONFIG        FH_DEF_ERR(FH_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
/** 不支持的参数/接口 */
#define FH_ERR_ADEC_NOT_SUPPORT       FH_DEF_ERR(FH_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
/** 无效的操作 */
#define FH_ERR_ADEC_NOT_PERM          FH_DEF_ERR(FH_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/** 内存申请失败 */
#define FH_ERR_ADEC_NOMEM             FH_DEF_ERR(FH_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/** 无内存 */
#define FH_ERR_ADEC_NOBUF             FH_DEF_ERR(FH_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
/** 缓存为空 */
#define FH_ERR_ADEC_BUF_EMPTY         FH_DEF_ERR(FH_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
/** 缓存已满 */
#define FH_ERR_ADEC_BUF_FULL          FH_DEF_ERR(FH_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/** 系统未初始化 */
#define FH_ERR_ADEC_SYS_NOTREADY      FH_DEF_ERR(FH_ID_ADEC, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
/** 解码出错 */
#define FH_ERR_ADEC_DECODER_ERR       FH_DEF_ERR(FH_ID_ADEC, EN_ERR_LEVEL_ERROR, ADEC_ERR_DECODER_ERR)
/** 输入数据比缓存大，内存不够 */
#define FH_ERR_ADEC_BUF_LACK          FH_DEF_ERR(FH_ID_ADEC, EN_ERR_LEVEL_ERROR, ADEC_ERR_BUF_LACK)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif/* End of #ifndef __FH_COMM_ADEC_H__*/
