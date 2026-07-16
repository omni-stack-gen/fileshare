#ifndef __TIME_HELPER_H__
#define __TIME_HELPER_H__

#include <stdio.h>
#include <stdint.h>

#include <time.h>

#define TIME_FORMAT     "%Y%m%d%H%M%S"
#define TIME_FORMAT2    "%Y/%m/%d %H:%M:%S"

#ifdef __cplusplus
extern "C"
{
#endif

/* 以毫秒为单位获取运行时钟时间 */
uint64_t time_now(void);

uint32_t time_relative_ms();

uint64_t time_relative_us();

uint32_t time_abs_sec();

uint32_t time_abs_ms();

uint64_t time_ads_us();

int time_get_local_time(struct tm *now_tm);

int time_set_local_time(struct tm *now_tm);

void time_timespec_diff(struct timespec *start, struct timespec *stop, struct timespec *diff);

int time_get_date_by_fmt(char *time_format, char *buffer, int buffer_len);

#ifdef __cplusplus
}
#endif

#endif //!__TIME_HELPER_H__