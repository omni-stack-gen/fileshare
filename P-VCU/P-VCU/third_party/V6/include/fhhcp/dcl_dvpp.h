/**
* @file dcl_dvpp.h
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef INC_EXTERNAL_DCL_DCL_DVPP_H_
#define INC_EXTERNAL_DCL_DCL_DVPP_H_

#include <stdint.h>
#include "dcl_base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dcldvppPicDesc dcldvppPicDesc;

typedef dclPixelFormat dcldvppPixelFormat;

/**
 * @ingroup DCL
 * @brief Create dvpp picture description.
 *
 * @retval null for failed.
 * @retval OtherValues success.
 */
DCL_FUNC_VISIBILITY dcldvppPicDesc *dcldvppCreatePicDesc();

/**
 * @ingroup DCL
 * @brief Destroy dvpp picture description.
 *
 * @par Function
 * Can only destroy picture description information created
 * through dcldvppCreatePicDesc interface.
 * @param picDesc [IN]     dvpp picture description.
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dcldvppCreatePicDesc
 */
DCL_FUNC_VISIBILITY dclError dcldvppDestroyPicDesc(dcldvppPicDesc *picDesc);

/**
 * @ingroup DCL
 * @brief Set dvpp picture description's format.
 *
 * @param picDesc [IN]     dvpp picture description.
 * @param format [IN]      dvpp picture format.
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dcldvppSetPicDescFormat(dcldvppPicDesc *picDesc, dcldvppPixelFormat format);

/**
 * @ingroup DCL
 * @brief Set dvpp picture description's width and height.
 *
 * @param picDesc [IN]     dvpp picture description.
 * @param width [IN]      dvpp picture width.
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dcldvppSetPicDescWidthHeight(dcldvppPicDesc *picDesc, uint32_t width, uint32_t height);

/**
 * @ingroup DCL
 * @brief Set dvpp picture description's widthStride.
 *
 * @par Restriction
 * Width alignment requirements:
 * @li The minimum stride is 32 and the maximum is 4096 * 4
 * (that is, an image in argb format with a width of 4096);
 * @li For 8K scaling, widthStride is required to be aligned to 2;
 * @li For non 8K scaling, the calculation formula for widthStride
 * is different for different image formats:
 *   @li yuv400sp, yuv420sp, yuv422sp, yuv444sp: input image width aligned to 16
 *   @li yuv422packed: input image width * 2 and then align to 16
 *   @li yuv444packed, rgb888: input image width alignment * 3, alignment to 16
 *   @li xrgb8888: input image width * 4, align to 16
 *   @li HFBC:input image width
 * @param picDesc [IN]     dvpp picture description.
 * @param widthStride [IN]   dvpp picture widthStride.
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dcldvppSetPicDescStride(dcldvppPicDesc *picDesc, uint32_t widthStride, uint32_t heightStride);


/**
 * @ingroup DCL
 * @brief Get dvpp picture desc's format.
 *
 * @param picDesc [IN]    dvpp picture description.
 * @retval format
 * @retval default PIXEL_FORMAT_YUV_400.
 */
DCL_FUNC_VISIBILITY dcldvppPixelFormat dcldvppGetPicDescFormat(const dcldvppPicDesc *picDesc);

/**
 * @ingroup DCL
 * @brief Get dvpp picture desc's width.
 *
 * @param picDesc [IN]    dvpp picture description.
 * @retval width.
 * @retval default 0.
 */
DCL_FUNC_VISIBILITY uint32_t dcldvppGetPicDescWidth(const dcldvppPicDesc *picDesc);

/**
 * @ingroup DCL
 * @brief Get dvpp picture desc's height.
 *
 * @param picDesc [IN]    dvpp picture description.
 * @retval height.
 * @retval default 0.
 */
DCL_FUNC_VISIBILITY uint32_t dcldvppGetPicDescHeight(const dcldvppPicDesc *picDesc);

/**
 * @ingroup DCL
 * @brief Get dvpp picture desc's widthStride.
 *
 * @par Restriction
 * Width alignment requirements:
 * @li The minimum stride is 32 and the maximum is 4096 * 4
 * (that is, an image in argb format with a width of 4096);
 * @li For 8K scaling, widthStride is required to be aligned to 2;
 * @li For non 8K scaling, the calculation formula for widthStride
 * is different for different image formats:
 *   @li yuv400sp, yuv420sp, yuv422sp, yuv444sp: input image width aligned to 16
 *   @li yuv422packed: input image width * 2 and then align to 16
 *   @li yuv444packed, rgb888: input image width alignment * 3, alignment to 16
 *   @li xrgb8888: input image width * 4, align to 16
 *   @li HFBC:input image width
 * @param picDesc [IN]    dvpp picture description.
 * @retval stride width.
 * @retval default 0.
 */
DCL_FUNC_VISIBILITY uint32_t dcldvppGetPicDescWidthStride(const dcldvppPicDesc *picDesc);

/**
 * @ingroup DCL
 * @brief Get dvpp picture desc's heightStride.
 *
 * @par Restriction
 * Height alignment requirements:
 * @li The height of the input image is aligned to 2.
 * High stride minimum 6 and maximum 4096.
 * @param picDesc [IN]    dvpp picture description.
 * @retval stride height.
 * @retval default 0.
 */
DCL_FUNC_VISIBILITY uint32_t dcldvppGetPicDescHeightStride(const dcldvppPicDesc *picDesc);

#ifdef __cplusplus
}
#endif


#endif // INC_EXTERNAL_DCL_DCL_DVPP_H_
