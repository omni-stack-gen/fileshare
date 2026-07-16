/**
* @file dcl_ive.h
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef DCL_IVE_H
#define DCL_IVE_H

#include "dcl_kcf.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

typedef uint64_t dcliveDmaHandle;

typedef struct IVE_DATA_S
{
   uint64_t phyAddr; /* RW;The physical address of the data */
   uint64_t virAddr; /* RW;The virtaul address of the data */
   uint32_t stride; /* RW;The stride of 2D data by byte */
   uint32_t width;  /* RW;The width of 2D data by byte */
   uint32_t height; /* RW;The height of 2D data by byte */
   uint32_t reserved;
}dclIveData;

/**
 * @ingroup DCL
 * @brief dma copy dp2000: only support 1D, only support block
 *        
 * @param src [IN]     source data which must be continuous by physical address 
 * @param dst [OUT]    source data which must be continuous by physical address 
 * @param handle [OUT] dma task indicator which can be used for dcliveDmaCopyQuery when non-block
 * @param isBlock [IN] block indicator, 0: non-block, 1:block  not support for dp2000
 * @retval DCL_SUCCESS The function is successfully executed.
 *
 * @see
 */
DCL_FUNC_VISIBILITY dclError dcliveDma(const dclIveData *src, const dclIveData *dst, dcliveDmaHandle *handle, dclBool isBlock);

/**
 * @ingroup DCL
 * @brief dma 2d copy query result dp2000: nonsupport
 *
 * @param handle [IN]    dma copy handle
 * @param finished [OUT]    dma copy done flag 1 : done  0: not done
 * @param block [IN]  true: block mode
 * @retval DCL_SUCCESS The function is successfully executed.
 *
 * @see
 */
DCL_FUNC_VISIBILITY dclError dcliveDmaCopyQuery(dcliveDmaHandle handle, dclBool *finished, dclBool block);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
