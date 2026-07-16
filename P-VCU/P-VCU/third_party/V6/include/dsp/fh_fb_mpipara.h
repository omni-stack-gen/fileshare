#ifndef __FH_FB_MPIPARA_H__
#define __FH_FB_MPIPARA_H__

#include "types/type_def.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */



typedef struct
{
    FH_UINT32              u32Width;
    FH_UINT32              u32Height;
}FYFB_SIZE_S;

typedef struct
{
    FH_BOOL             bKeyEnable;            /* colorkey enable flag */
    FH_UINT32              u32Key;                /* colorkey value, maybe contains alpha */
}FYFB_COLORKEY_S;

typedef struct
{
    FH_SINT32              s32XPos;               /* horizontal position */
    FH_SINT32              s32YPos;               /* vertical position */
}FYFB_POINT_S;

typedef struct
{
    FH_UINT32           base;        /**<  phy addr */
    FH_VOID *           vbase;       /**<  vir addr */
    FH_UINT32           size;        /**<  memsize */
}FYFB_MEM_INFO_S;

/* Alpha info */
typedef struct
{
    FH_BOOL                bAlphaEnable;          /* alpha enable flag */
    FH_BOOL                bAlphaChannel;         /* alpha channel enable flag */
    FH_UINT8               u8Alpha0;              /* alpha0 value, used in ARGB1555 */
    FH_UINT8               u8Alpha1;              /* alpha1 value, used in ARGB1555 */
    FH_UINT8               u8GlobalAlpha;         /* global alpha value */
    FH_UINT8               u8Reserved;
}FYFB_ALPHA_S;



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __FH_FB_MPIPARA_H__ */

