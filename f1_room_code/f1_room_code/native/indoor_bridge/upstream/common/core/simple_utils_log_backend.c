/**
 * @file
 * @brief simple 示例 utils log 后端适配实现。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#include "simple_utils_log_backend.h"

#include "simple_log.h"

#include <utils/log.h>

static log_level_t simple_utils_log_level(simple_log_level_t level)
{
    switch (level)
    {
    case SIMPLE_LOG_LEVEL_DEBUG:
        return LOG_LEVEL_DEBUG;
    case SIMPLE_LOG_LEVEL_WARNING:
        return LOG_LEVEL_WARN;
    case SIMPLE_LOG_LEVEL_ERROR:
        return LOG_LEVEL_ERROR;
    case SIMPLE_LOG_LEVEL_INFO:
    default:
        return LOG_LEVEL_INFO;
    }
}

static void simple_utils_log_write(void *ctx,
                                   simple_log_level_t level,
                                   const char *module,
                                   const char *file,
                                   const long line,
                                   const char *func,
                                   const char *message)
{
    (void)ctx;
    log_output(simple_utils_log_level(level),
               module ? module : "simple",
               file ? file : "-",
               line,
               func ? func : "-",
               "%s",
               message ? message : "");
}

int simple_utils_log_backend_register(const char *ident)
{
    static const simple_log_backend_t backend =
    {
        NULL,
        simple_utils_log_write,
    };

    log_init(ident ? ident : "mtrans_simple");
    return simple_log_register_backend(&backend);
}
