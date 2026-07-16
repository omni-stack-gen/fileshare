/**
 * @file
 * @brief simple 示例 utils log 后端注册接口。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#ifndef __SIMPLE_UTILS_LOG_BACKEND_H__
#define __SIMPLE_UTILS_LOG_BACKEND_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 将 examples/support/utils/log 注册为 simple 日志后端。
 *
 * @param[in] ident 进程日志标识，可为 NULL。
 *
 * @return 成功返回 0；失败返回负数。
 */
int simple_utils_log_backend_register(const char *ident);

#ifdef __cplusplus
}
#endif

#endif
