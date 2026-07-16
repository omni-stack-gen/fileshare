#ifndef __FY_COMM_LCM_H__
#define __FY_COMM_LCM_H__

#include "fh_errno.h"
#include "fh_video.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */


typedef FH_SINT32 LCM_DEV;

/**********************************************************************
	PART I		- Macro
**********************************************************************/
#define LCM_CMD_DATA_LEN			(256)

#define LCM_DEVICE_NUM				(1)

/**********************************************************************
	PART II		- Structure and Enumeration
**********************************************************************/

typedef enum {
	LCM_SCREEN_NONE				= 0x0,
	LCM_SCREEN_MCU				= 0x1,
	LCM_SCREEN_RGB				= 0x2,
	LCM_SCREEN_MIPI 			= 0x3,
	LCM_SCREEN_DUMMY			,
} LCM_SCREEN_ID_E;


typedef struct {
	LCM_SCREEN_ID_E				enScreen;
	FH_UINT32				u32Width;
	FH_UINT32				u32Height;
	FH_UINT32				u32FPS;
} LCM_ATTR_S;

typedef struct {
	FH_UINT8				u8Cmd;
	FH_UINT8				u8WaitMs;
	FH_UINT16				u16DataLen;
	FH_UINT8				pu8Data[LCM_CMD_DATA_LEN];
} LCM_CMD_S;

/**********************************************************************
	PART III	- Error Code
**********************************************************************/
typedef enum {
	ERR_LCM_NOT_ENABLE			= 0x45,
	ERR_LCM_NOT_DISABLE			= 0x46,
	ERR_LCM_NOT_CONFIG			= 0x47,
	ERR_LCM_INVALID_DEVID			= 0x48,
	ERR_LCM_BUTT
}EN_LCM_ERR_CODE_E;

/* System define error code */
#define FH_ERR_LCM_BUSY				FH_DEF_ERR(FH_ID_LCM, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
#define FH_ERR_LCM_NO_MEM			FH_DEF_ERR(FH_ID_LCM, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
#define FH_ERR_LCM_NULL_PTR			FH_DEF_ERR(FH_ID_LCM, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
#define FH_ERR_LCM_SYS_NOTREADY			FH_DEF_ERR(FH_ID_LCM, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
#define FH_ERR_LCM_ILLEGAL_PARAM		FH_DEF_ERR(FH_ID_LCM, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
#define FH_ERR_LCM_NOT_SUPPORT			FH_DEF_ERR(FH_ID_LCM, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
#define FH_ERR_LCM_NOT_PERMIT			FH_DEF_ERR(FH_ID_LCM, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)

#define FH_ERR_LCM_NOT_ENABLE			FH_DEF_ERR(FH_ID_LCM, EN_ERR_LEVEL_ERROR, ERR_LCM_NOT_ENABLE)
#define FH_ERR_LCM_NOT_DISABLE			FH_DEF_ERR(FH_ID_LCM, EN_ERR_LEVEL_ERROR, ERR_LCM_NOT_DISABLE)
#define FH_ERR_LCM_NOT_CONFIG			FH_DEF_ERR(FH_ID_LCM, EN_ERR_LEVEL_ERROR, ERR_LCM_NOT_CONFIG)
#define FH_ERR_LCM_INVALID_DEVID		FH_DEF_ERR(FH_ID_LCM, EN_ERR_LEVEL_ERROR, ERR_LCM_INVALID_DEVID)


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __FY_COMM_VO_H__ */

