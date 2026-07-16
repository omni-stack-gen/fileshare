#ifndef __TRANSPORT_INTERNAL_LOG_H__
#define __TRANSPORT_INTERNAL_LOG_H__

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#include "transport.h"

void transport_log(transport_log_level_t level, const char *file, const long line,
                   const char *func, const char *format, ...) __attribute__((format(printf, 5, 6)));

#define TRANSPORT_LOGD(format, ...) transport_log(TRANSPORT_DEBUG, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define TRANSPORT_LOGI(format, ...) transport_log(TRANSPORT_INFO, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define TRANSPORT_LOGW(format, ...) transport_log(TRANSPORT_WARNING, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define TRANSPORT_LOGE(format, ...) transport_log(TRANSPORT_ERROR, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define TRANSPORT_LOGDEBUG() printf("\033[1;31m<%s:%s> line:%d\033[0m\r\n", __FILE__, __FUNCTION__, __LINE__)


#endif /* __TRANSPORT_INTERNAL_LOG_H__ */
