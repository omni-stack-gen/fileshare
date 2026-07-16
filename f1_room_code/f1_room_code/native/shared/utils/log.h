#ifndef __LOG_H__
#define __LOG_H__

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#ifndef LOG_TAG
#define LOG_TAG          "app0"
#endif

typedef enum log_level
{
	LOG_LEVEL_NONE = 0,
	LOG_LEVEL_FATAL,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_WARN,
	LOG_LEVEL_INFO,
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_VERBOSE
} log_level_t;

#define LOGV(format, ...) log_output(LOG_LEVEL_VERBOSE, LOG_TAG, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define LOGD(format, ...) log_output(LOG_LEVEL_DEBUG,   LOG_TAG, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define LOGI(format, ...) log_output(LOG_LEVEL_INFO,    LOG_TAG, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define LOGW(format, ...) log_output(LOG_LEVEL_WARN,    LOG_TAG, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define LOGE(format, ...) log_output(LOG_LEVEL_ERROR,   LOG_TAG, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define LOGF(format, ...) log_output(LOG_LEVEL_FATAL,   LOG_TAG, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define LOGDEBUG() printf("\033[1;31m<%s:%s> line:%d\033[0m\r\n", __FILE__, __FUNCTION__, __LINE__)

#define LOG_BUFFER_HEX(tag, data, size) log_hex_buffer(tag, __FILE__, __LINE__, __func__, data, size);

#ifdef __cplusplus
extern "C"
{
#endif

void log_init(const char *ident);

void log_output(log_level_t level, const char *tag, const char *file, const long line,
                const char *func, const char *format, ...) __attribute__((format(printf, 6, 7)));

void log_hex_buffer(const char *tag, const char *file, const long line, const char *func, const void *data,
                    size_t size);

void log_console_onoff(bool onoff);

#ifdef __cplusplus
}
#endif

#endif /* __LOG_H__ */