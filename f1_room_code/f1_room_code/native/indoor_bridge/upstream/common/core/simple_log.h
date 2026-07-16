/**
 * @file
 * @brief simple 示例应用公共模块 simple_log 的公共头文件。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#ifndef __SIMPLE_LOG_H__
#define __SIMPLE_LOG_H__

#include <stdarg.h>

#include "call_flow_runtime.h"
#include "mtrans.h"
#include "transport.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief simple 示例统一日志级别。
 */
typedef enum simple_log_level
{
    /** 调试日志。 */
    SIMPLE_LOG_LEVEL_DEBUG = 0,
    /** 普通信息日志。 */
    SIMPLE_LOG_LEVEL_INFO,
    /** 警告日志。 */
    SIMPLE_LOG_LEVEL_WARNING,
    /** 错误日志。 */
    SIMPLE_LOG_LEVEL_ERROR,
} simple_log_level_t;

/**
 * @brief simple 示例日志后端。
 *
 * 后端函数由进程入口注册。未注册时 simple_log 使用 stderr fallback，便于 host 测试和早期
 * 初始化；板端主线进程应注册 utils log 后端以统一输出路径。
 */
typedef struct simple_log_backend
{
    /** 后端私有上下文，可为 NULL。 */
    void *ctx;
    /** 输出一条已经格式化后的日志正文。 */
    void (*write_fn)(void *ctx,
                     simple_log_level_t level,
                     const char *module,
                     const char *file,
                     const long line,
                     const char *func,
                     const char *message);
} simple_log_backend_t;

/**
 * @brief 注册 simple 示例日志后端。
 *
 * @param[in] backend 后端描述，函数指针在调用期间会被复制，可为 NULL 表示恢复 stderr fallback。
 *
 * @return 成功返回 0；参数非法返回负数。
 */
int simple_log_register_backend(const simple_log_backend_t *backend);

/**
 * @brief 输出 simple 示例统一日志。
 *
 * @param[in] level 日志级别。
 * @param[in] file 源文件名，可为 NULL。
 * @param[in] line 源代码行号。
 * @param[in] func 函数名，可为 NULL。
 * @param[in] format printf 风格格式串，不能为 NULL。
 */
void simple_log_printf(simple_log_level_t level,
                       const char *file,
                       const long line,
                       const char *func,
                       const char *format,
                       ...);

/**
 * @brief 将 call 模块日志转接到 simple 日志。
 *
 * @param[in] level call 模块日志级别。
 * @param[in] file 源文件名，可为 NULL。
 * @param[in] line 源代码行号。
 * @param[in] func 函数名，可为 NULL。
 * @param[in] format printf 风格格式串，不能为 NULL。
 * @param[in] ap 可变参数列表，调用方保持所有权。
 */
void simple_example_call_log_handler(call_log_level_t level,
                                     const char *file,
                                     const long line,
                                     const char *func,
                                     const char *format,
                                     va_list ap);

/**
 * @brief 将 mtrans 模块日志转接到 simple 日志。
 *
 * @param[in] level mtrans 模块日志级别。
 * @param[in] file 源文件名，可为 NULL。
 * @param[in] line 源代码行号。
 * @param[in] func 函数名，可为 NULL。
 * @param[in] format printf 风格格式串，不能为 NULL。
 * @param[in] ap 可变参数列表，调用方保持所有权。
 */
void simple_example_mtrans_log_handler(mtrans_log_level_t level,
                                       const char *file,
                                       const long line,
                                       const char *func,
                                       const char *format,
                                       va_list ap);

/**
 * @brief 将 transport 模块日志转接到 simple 日志。
 *
 * @param[in] level transport 模块日志级别。
 * @param[in] file 源文件名，可为 NULL。
 * @param[in] line 源代码行号。
 * @param[in] func 函数名，可为 NULL。
 * @param[in] format printf 风格格式串，不能为 NULL。
 * @param[in] ap 可变参数列表，调用方保持所有权。
 */
void simple_example_transport_log_handler(transport_log_level_t level,
                                          const char *file,
                                          const long line,
                                          const char *func,
                                          const char *format,
                                          va_list ap);

/** @brief 输出 DEBUG 级别 simple 日志。 */
#define SIMPLE_LOGD(format, ...) \
    simple_log_printf(SIMPLE_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __FUNCTION__, \
                      format, ##__VA_ARGS__)
/** @brief 输出 INFO 级别 simple 日志。 */
#define SIMPLE_LOGI(format, ...) \
    simple_log_printf(SIMPLE_LOG_LEVEL_INFO, __FILE__, __LINE__, __FUNCTION__, \
                      format, ##__VA_ARGS__)
/** @brief 输出 WARNING 级别 simple 日志。 */
#define SIMPLE_LOGW(format, ...) \
    simple_log_printf(SIMPLE_LOG_LEVEL_WARNING, __FILE__, __LINE__, __FUNCTION__, \
                      format, ##__VA_ARGS__)
/** @brief 输出 ERROR 级别 simple 日志。 */
#define SIMPLE_LOGE(format, ...) \
    simple_log_printf(SIMPLE_LOG_LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, \
                      format, ##__VA_ARGS__)
/** @brief 输出当前位置调试标记。 */
#define SIMPLE_LOGDEBUG() SIMPLE_LOGD("debug marker")

#ifdef __cplusplus
}
#endif

#endif
