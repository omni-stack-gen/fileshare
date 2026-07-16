/**
 * @file call_utils.h
 * @brief 呼叫模块通用工具函数。
 */
#ifndef __CALL_UTILS_H__
#define __CALL_UTILS_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 获取单调时钟毫秒值。
 *
 * @return 当前单调时钟时间（毫秒）。
 */
uint64_t call_monotonic_ms(void);

/**
 * @brief 获取本地壁钟毫秒值。
 *
 * @return 当前本地壁钟时间（毫秒）。
 */
uint64_t call_wallclock_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* __CALL_UTILS_H__ */
