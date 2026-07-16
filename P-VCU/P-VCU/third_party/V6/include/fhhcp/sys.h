#ifndef _NPU_SYS_H_
#define _NPU_SYS_H_

#include "base.h"
#ifdef __cplusplus
extern "C" {
#endif

/*!
* @brief 查询NPU接口版本信息
* @param [out] majorVersion major版本
* @param [out] minorVersion minor版本
* @param [out] patchVersion patch版本
* @note  版本信息格式 x.y.z
* 返回值：
    0：成功，其它值：错误码
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_SDK_GetVersion(int32_t *majorVersion,
                       int32_t *minorVersion,
                       int32_t *patchVersion);

/*!
* @brief 设置NPU接口log等级
* @param [in] level log等级，取值范围[0,4]，值越大输出log越少
* 返回值：
    0：成功，其它值：错误码
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_SDK_SetLogLevel(uint32_t level);

/*!
* @brief 设置是否进入低功耗模式
* @param [in] enable   是否使能的标志 0:不使能, 非0:使能
* 返回值：
    0：成功，其它值：错误码
*/
TY_NPU_FUNC_VISIBILITY int32_t TY_SDK_EnableLowPower(int32_t enable);

#ifdef __cplusplus
}
#endif

#endif
