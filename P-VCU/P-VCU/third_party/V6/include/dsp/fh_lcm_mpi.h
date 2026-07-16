#ifndef __MPI_LCM_H__
#define __MPI_LCM_H__

#include "fh_lcm_mpipara.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

/**********************************************************************
	Standard API
**********************************************************************/

FH_SINT32 FH_LCM_SetPubAttr(LCM_DEV lcmDev, const LCM_ATTR_S *pstAttr);

FH_SINT32 FH_LCM_Enable(LCM_DEV lcmDev);

FH_SINT32 FH_LCM_Disable(LCM_DEV lcmDev);

FH_SINT32 FH_LCM_DSI_Read(LCM_DEV lcmDev, LCM_CMD_S* pstCmd);

FH_SINT32 FH_LCM_DSI_Write(LCM_DEV lcmDev, LCM_CMD_S* pstCmd);

FH_SINT32 FH_LCM_SPI_Read(LCM_DEV lcmDev, LCM_CMD_S* pstCmd);

FH_SINT32 FH_LCM_SPI_Write(LCM_DEV lcmDev, LCM_CMD_S* pstCmd);

FH_SINT32 FH_LCM_I2C_Read(LCM_DEV lcmDev, LCM_CMD_S* pstCmd);

FH_SINT32 FH_LCM_I2C_Write(LCM_DEV lcmDev, LCM_CMD_S* pstCmd);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /*__MPI_LCM_H__ */

