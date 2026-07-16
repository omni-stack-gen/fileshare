#ifndef __LOG_H__
#define __LOG_H__

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#ifndef LOG_TAG
#define LOG_TAG "NULL"
#endif

#define DBG_VERBOSE 0
#define DBG_DEBUG 1
#define DBG_INFO 2
#define DBG_WARN 3
#define DBG_ERROR 4
#define DBG_FATAL 5

#define LOG_FULL_BIT (1 << 31)
#define LOG_TAG_BIT (1 << 3)
#define LOG_TIMESTAMP_BIT (1 << 2)
#define LOG_PIDTID_BIT (1 << 1)
#define LOG_FUNCLINE_BIT (1 << 0)
#define LOG_VERBOSE_BIT (LOG_TAG_BIT | LOG_TIMESTAMP_BIT | LOG_PIDTID_BIT | LOG_FUNCLINE_BIT)

#define B_RED(str) "\033[1;31m" str "\033[0m"
#define B_GREEN(str) "\033[1;32m" str "\033[0m"
#define B_YELLOW(str) "\033[1;33m" str "\033[0m"
#define B_BLUE(str) "\033[1;34m" str "\033[0m"
#define B_MAGENTA(str) "\033[1;35m" str "\033[0m"
#define B_CYAN(str) "\033[1;36m" str "\033[0m"
#define B_WHITE(str) "\033[1;37m" str "\033[0m"

#define RED(str) "\033[31m" str "\033[0m"
#define GREEN(str) "\033[32m" str "\033[0m"
#define YELLOW(str) "\033[33m" str "\033[0m"
#define BLUE(str) "\033[34m" str "\033[0m"
#define MAGENTA(str) "\033[35m" str "\033[0m"
#define CYAN(str) "\033[36m" str "\033[0m"
#define WHITE(str) "\033[37m" str "\033[0m"

#ifdef __cplusplus
extern "C" {
#endif

void goblin_log_init(const char *path);

void goblin_log_deinit();

void goblin_log_set_level(int level);

void goblin_log_set_file_size(int size);

void goblin_log_set_cyclic(int enable);

void goblin_log_set_path(const char *path);

void goblin_log_set_file_bit(int bit);

void goblin_log_set_write_disable();

void goblin_log_output(int level, const char *tag, const char *file, const long line, const char *func, const char *format, ...);

void goblin_log_hex(const char *file, const long line, uint8_t *data, size_t len);

void goblin_log_hex_string(const char *file, const long line, uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#define LOGV(...) goblin_log_output(DBG_VERBOSE, LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOGD(...) goblin_log_output(DBG_DEBUG,   LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOGI(...) goblin_log_output(DBG_INFO,    LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOGW(...) goblin_log_output(DBG_WARN,    LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOGE(...) goblin_log_output(DBG_ERROR,   LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOGF(...) goblin_log_output(DBG_FATAL,   LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define logv(...) goblin_log_output(DBG_VERBOSE, LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define logd(...) goblin_log_output(DBG_DEBUG,   LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define logi(...) goblin_log_output(DBG_INFO,    LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define logw(...) goblin_log_output(DBG_WARN,    LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define loge(...) goblin_log_output(DBG_ERROR,   LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define logf(...) goblin_log_output(DBG_FATAL,   LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define ALOGV(...) goblin_log_output(DBG_VERBOSE, LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define ALOGD(...) goblin_log_output(DBG_DEBUG,   LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define ALOGI(...) goblin_log_output(DBG_INFO,    LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define ALOGW(...) goblin_log_output(DBG_WARN,    LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define ALOGE(...) goblin_log_output(DBG_ERROR,   LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define ALOGF(...) goblin_log_output(DBG_FATAL,   LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define PRINTF(...) goblin_log_output(DBG_INFO, LOG_TAG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define LOGDEBUG() printf("\033[1;31m<%s:%s> line:%d\033[0m\n", __FILE__, __FUNCTION__, __LINE__)

#define LOG_HEX(data, len) goblin_log_hex(__FILE__, __LINE__, data, len)

#define LOG_HEX_STRING(data, len) goblin_log_hex_string(__FILE__, __LINE__, data, len)

typedef void (*GoblinLogOutput)(int level, const char *tag, const char *file, const long line, const char *func, const char *format, va_list ap);
void goblin_log_def_output(int level, const char *tag, const char *file, const long line, const char *func, const char *format, va_list ap);
#endif //!__LOG_H__