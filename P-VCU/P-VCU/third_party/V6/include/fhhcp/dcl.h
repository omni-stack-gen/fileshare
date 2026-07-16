/**
* @file dcl.h
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef INC_EXTERNAL_DCL_DCL_H_
#define INC_EXTERNAL_DCL_DCL_H_

#include "dcl_base.h"
#include "dcl_mdl.h"
#include "dcl_rt.h"
#include "dcl_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

// Model toolchain internal version supported by current version 
#define DCL_MODEL_INTERNAL_MAJOR_VERSION    0


/**
 * @ingroup DCL
 * @brief dcl initialize
 *
 * @par Restriction
 * The dclInit interface can be called only once in a process
 * @param configPath [IN]    the config path,it can be NULL
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclInit(const char *configPath);

/**
 * @ingroup DCL
 * @brief dcl finalize
 *
 * @par Restriction
 * Need to call dclFinalize before the process exits.
 * After calling dclFinalize,the services cannot continue to be used normally.
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclFinalize();

/**
 * @ingroup DCL
 * @brief query DCL interface version
 *
 * @param majorVersion[OUT] DCL interface major version
 * @param minorVersion[OUT] DCL interface minor version
 * @param patchVersion[OUT] DCL interface patch version
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclrtGetVersion(int32_t *majorVersion, int32_t *minorVersion, int32_t *patchVersion);

/**
 * @ingroup DCL
 * @brief Set DCL log level
 *
 * @param level[IN] log level
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclSetLogLevel(int32_t level);

#ifdef __cplusplus
}
#endif

#endif // INC_EXTERNAL_DCL_DCL_H_
