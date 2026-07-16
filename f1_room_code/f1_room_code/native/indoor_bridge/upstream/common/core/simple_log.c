/**
 * @file
 * @brief simple 示例应用公共模块 simple_log 的实现文件。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#include "simple_log.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define SIMPLE_LOG_MESSAGE_MAX 1024
#define SIMPLE_LOG_LINE_MAX    1600

static simple_log_backend_t simple_log_backend;
static int simple_log_backend_registered = 0;

static const char *simple_log_basename(const char *file)
{
    const char *p;

    if (!file)
    {
        return "-";
    }

    p = strrchr(file, '/');
    return p ? (p + 1) : file;
}

static const char *simple_log_level_name(simple_log_level_t level)
{
    switch (level)
    {
    case SIMPLE_LOG_LEVEL_DEBUG:
        return "D";
    case SIMPLE_LOG_LEVEL_INFO:
        return "I";
    case SIMPLE_LOG_LEVEL_WARNING:
        return "W";
    case SIMPLE_LOG_LEVEL_ERROR:
        return "E";
    default:
        return "?";
    }
}

static void simple_log_stderr_write(simple_log_level_t level,
                                    const char *module,
                                    const char *file,
                                    const long line,
                                    const char *func,
                                    const char *message)
{
    char line_buf[SIMPLE_LOG_LINE_MAX];
    int pos;

    pos = snprintf(line_buf, sizeof(line_buf), "[%s][%s] [%s|%s(%ld)] %s",
                   module ? module : "simple",
                   simple_log_level_name(level),
                   simple_log_basename(file),
                   func ? func : "-",
                   line,
                   message ? message : "");
    if (pos < 0)
    {
        return;
    }
    if ((size_t)pos >= sizeof(line_buf))
    {
        pos = (int)sizeof(line_buf) - 1;
    }
    if (pos == 0 || line_buf[pos - 1] != '\n')
    {
        if ((size_t)pos < sizeof(line_buf) - 1U)
        {
            line_buf[pos++] = '\n';
        }
        else
        {
            line_buf[sizeof(line_buf) - 2U] = '\n';
            pos = (int)sizeof(line_buf) - 1;
        }
    }
    (void)write(STDERR_FILENO, line_buf, (size_t)pos);
}

static void simple_log_write(const char *module,
                             simple_log_level_t level,
                             const char *file,
                             const long line,
                             const char *func,
                             const char *format,
                             va_list ap)
{
    char message[SIMPLE_LOG_MESSAGE_MAX];

    if (format)
    {
        vsnprintf(message, sizeof(message), format, ap);
    }
    else
    {
        message[0] = '\0';
    }

    if (simple_log_backend_registered && simple_log_backend.write_fn)
    {
        simple_log_backend.write_fn(simple_log_backend.ctx,
                                    level,
                                    module,
                                    file,
                                    line,
                                    func,
                                    message);
        return;
    }

    simple_log_stderr_write(level, module, file, line, func, message);
}

static simple_log_level_t simple_call_log_level(call_log_level_t level)
{
    switch (level)
    {
    case CALL_DEBUG:
        return SIMPLE_LOG_LEVEL_DEBUG;
    case CALL_INFO:
        return SIMPLE_LOG_LEVEL_INFO;
    case CALL_WARNING:
        return SIMPLE_LOG_LEVEL_WARNING;
    case CALL_ERROR:
        return SIMPLE_LOG_LEVEL_ERROR;
    default:
        return SIMPLE_LOG_LEVEL_INFO;
    }
}

static simple_log_level_t simple_mtrans_log_level(mtrans_log_level_t level)
{
    switch (level)
    {
    case MTRANS_DEBUG:
        return SIMPLE_LOG_LEVEL_DEBUG;
    case MTRANS_INFO:
        return SIMPLE_LOG_LEVEL_INFO;
    case MTRANS_WARN:
        return SIMPLE_LOG_LEVEL_WARNING;
    case MTRANS_ERROR:
        return SIMPLE_LOG_LEVEL_ERROR;
    default:
        return SIMPLE_LOG_LEVEL_INFO;
    }
}

static simple_log_level_t simple_transport_log_level(transport_log_level_t level)
{
    switch (level)
    {
    case TRANSPORT_DEBUG:
        return SIMPLE_LOG_LEVEL_DEBUG;
    case TRANSPORT_INFO:
        return SIMPLE_LOG_LEVEL_INFO;
    case TRANSPORT_WARNING:
        return SIMPLE_LOG_LEVEL_WARNING;
    case TRANSPORT_ERROR:
        return SIMPLE_LOG_LEVEL_ERROR;
    default:
        return SIMPLE_LOG_LEVEL_INFO;
    }
}

int simple_log_register_backend(const simple_log_backend_t *backend)
{
    if (!backend)
    {
        memset(&simple_log_backend, 0, sizeof(simple_log_backend));
        simple_log_backend_registered = 0;
        return 0;
    }
    if (!backend->write_fn)
    {
        return -1;
    }

    simple_log_backend = *backend;
    simple_log_backend_registered = 1;
    return 0;
}

void simple_log_printf(simple_log_level_t level,
                       const char *file,
                       const long line,
                       const char *func,
                       const char *format,
                       ...)
{
    va_list ap;

    va_start(ap, format);
    simple_log_write("simple", level, file, line, func, format, ap);
    va_end(ap);
}

void simple_example_call_log_handler(call_log_level_t level,
                                     const char *file,
                                     const long line,
                                     const char *func,
                                     const char *format,
                                     va_list ap)
{
    simple_log_write("call", simple_call_log_level(level),
                     file, line, func, format, ap);
}

void simple_example_mtrans_log_handler(mtrans_log_level_t level,
                                       const char *file,
                                       const long line,
                                       const char *func,
                                       const char *format,
                                       va_list ap)
{
    simple_log_write("mtrans", simple_mtrans_log_level(level),
                     file, line, func, format, ap);
}

void simple_example_transport_log_handler(transport_log_level_t level,
                                          const char *file,
                                          const long line,
                                          const char *func,
                                          const char *format,
                                          va_list ap)
{
    simple_log_write("transport", simple_transport_log_level(level),
                     file, line, func, format, ap);
}
