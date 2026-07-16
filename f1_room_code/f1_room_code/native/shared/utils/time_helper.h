#ifndef __TIME_HELPER_H__
#define __TIME_HELPER_H__

#include <common_defs.h>

#define USEC_PER_SEC    (1000000ULL)
#define MSEC_PER_SEC    (1000ULL)
#define USEC_PER_MSEC   (1000ULL)
#define NSEC_PER_SEC    (1000000000ULL)
#define NSEC_PER_MSEC   (1000000ULL)
#define NSEC_PER_USEC   (1000ULL)
#define TIME_INVALID    ((uint64_t) -1)

#ifdef __cplusplus
extern "C"
{
#endif

/* 以毫秒为单位获取运行时钟时间 */
uint64_t time_now(void);

uint64_t time_now_us(void);

// return true if time a > time b
static inline bool time_after(uint64_t a, uint64_t b)
{
	return a > b;
}

// return true if time a < time b
static inline bool time_before(uint64_t a, uint64_t b)
{
	return time_after(b, a);
}

// return true if time a >= time b
static inline bool time_after_equal(uint64_t a, uint64_t b)
{
	return a >= b;
}

// return true if time a <= time b
static inline bool time_before_equal(uint64_t a, uint64_t b)
{
	return time_after_equal(b, a);
}

static inline uint64_t time_offset(uint64_t time, uint64_t offset)
{
	/* check overflow */
	if (offset > UINT64_MAX - time)
	{
		return UINT64_MAX;
	}

	return time + offset;
}

static inline uint64_t time_diff(uint64_t a, uint64_t b)
{
	return (a < b) ? b - a : a - b;
}

static inline uint64_t time_to_secs(uint64_t time)
{
	return time / MSEC_PER_SEC;
}

// static inline uint64_t time_to_msecs(uint64_t time)
// {
//  return time / USEC_PER_MSEC;
// }

bool time_expired(uint64_t time);

void time_count_down_ms(uint64_t *time, uint64_t timeout_ms);

uint64_t time_relative_us(void);

uint32_t time_abs_sec(void);

uint64_t time_abs_ms(void);

uint64_t time_abs_us(void);

int time_get_local_time(struct tm *now_tm);

int time_set_local_time(struct tm *now_tm);

void time_timespec_diff(struct timespec *start, struct timespec *stop, struct timespec *diff);

double timespec_diff_ms(struct timespec *start, struct timespec *end);

int time_set_system_time(time_t new_time);

int time_to_time_str(time_t t, char *buffer, int buffer_len);

time_t time_str_to_time(const char *time_str);

int time_get_date_fmt(char *time_format, int format_len);

#ifdef __cplusplus
}
#endif

#endif //!__TIME_HELPER_H__