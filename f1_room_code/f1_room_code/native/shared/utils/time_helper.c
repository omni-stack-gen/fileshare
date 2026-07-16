#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>
#include <time.h>

#include <utils/time_helper.h>

#define LOG_TAG "time_helper"
#include <utils/log.h>

#define TIME_FORMAT "%Y%m%d%H%M%S"

uint64_t time_now(void)
{
	struct timespec now;

	clock_gettime(CLOCK_BOOTTIME, &now);

	return (uint64_t)now.tv_sec * MSEC_PER_SEC + (uint64_t)now.tv_nsec / NSEC_PER_MSEC;
}

uint64_t time_now_us(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_BOOTTIME, &ts);

	return (uint64_t)ts.tv_sec * USEC_PER_SEC + (uint64_t)ts.tv_nsec / NSEC_PER_USEC;
}

bool time_expired(uint64_t time)
{
	return time_after_equal(time_now(), time);
}

void time_count_down_ms(uint64_t *time, uint64_t timeout_ms)
{
	if (time == NULL)
	{
		return ;
	}

	*time = time_now() + timeout_ms;
}

uint64_t time_relative_us(void)
{
	uint64_t us;

	struct timespec ts;
	clock_gettime(CLOCK_BOOTTIME, &ts);

	us = ts.tv_sec;
	us = us * 1000000LL;
	us += ts.tv_nsec / 1000;

	return us;
}

uint32_t time_abs_sec(void)
{
	struct timeval tv;
	uint32_t       sec;
	gettimeofday(&tv, NULL);
	sec = tv.tv_sec;
	return sec;
}

uint64_t time_abs_ms(void)
{
	struct timeval tv;
	uint64_t       ms;
	gettimeofday(&tv, NULL);
	ms = tv.tv_sec * 1000LL + tv.tv_usec / 1000;
	return ms;
}

uint64_t time_abs_us(void)
{
	struct timeval tv;
	uint64_t       us;
	gettimeofday(&tv, NULL);
	us = tv.tv_sec * 1000000LL + tv.tv_usec;
	return us;
}

void time_timespec_diff(struct timespec *start, struct timespec *stop, struct timespec *diff)
{
	if ((stop->tv_nsec - start->tv_nsec) < 0)
	{
		diff->tv_sec = stop->tv_sec - start->tv_sec - 1;
		diff->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000UL;
	}
	else
	{
		diff->tv_sec = stop->tv_sec - start->tv_sec;
		diff->tv_nsec = stop->tv_nsec - start->tv_nsec;
	}

	return ;
}

double timespec_diff_ms(struct timespec *start, struct timespec *end)
{
	return (end->tv_sec - start->tv_sec) * 1000.0 + (end->tv_nsec - start->tv_nsec) / 1000000.0;
}

int time_get_local_time(struct tm *now_tm)
{
	int ret = 0;

	struct timeval tv;
	struct tm broken;

	if (!now_tm)
	{
		return -1;
	}

	ret = gettimeofday(&tv, NULL);

	if (-1 == ret)
	{
		LOGE("gettimeofday failed :%d \n", ret);
		return -1;
	}

	if (!localtime_r(&tv.tv_sec, &broken))
	{
		LOGE("localtime_r failed \n");
		return -1;
	}

	broken.tm_year += 1900;
	broken.tm_mon += 1;

	memcpy(now_tm, &broken, sizeof(struct tm));

	return 0;
}

int time_set_local_time(struct tm *now_tm)
{
	int ret = 0;

	struct timeval tv;
	struct tm broken;
	struct timezone tz;

	if (!now_tm)
	{
		LOGE("time_set_local_time invild param \n");
		return -1;
	}

	ret = gettimeofday(&tv, NULL);

	if (-1 == ret)
	{
		LOGE("gettimeofday failed :%d \n", ret);
		return -1;
	}

	if (!localtime_r(&tv.tv_sec, &broken))
	{
		LOGE("localtime_r failed \n");
		return -1;
	}

	tz.tz_minuteswest = timezone / 60;

	if (broken.tm_isdst > 0)
	{
		tz.tz_minuteswest -= 60;
	}

	tz.tz_dsttime = 0;

	broken.tm_sec = now_tm->tm_sec;
	broken.tm_min = now_tm->tm_min;
	broken.tm_hour = now_tm->tm_hour;
	broken.tm_mday = now_tm->tm_mday;
	broken.tm_mon = now_tm->tm_mon - 1;
	broken.tm_year = now_tm->tm_year - 1900;

	tv.tv_sec = mktime(&broken);
	tv.tv_usec = 0;

	ret = settimeofday(&tv, &tz);

	if (-1 == ret)
	{
		LOGE("settimeofday failed :%d \n", ret);
		return -1;
	}

	// LOGI("set time %d-%02d-%02d %02d:%02d:%02d daylight saving time:%d tm_zone:%s \n",
	//      broken.tm_year + 1900, broken.tm_mon + 1, broken.tm_mday,
	//      broken.tm_hour, broken.tm_min, broken.tm_sec, broken.tm_isdst, broken.tm_zone);

	return ret;
}

int time_set_system_time(time_t new_time)
{
	int ret = 0;

	struct timespec ts = {0};

	ts.tv_sec = new_time;
	ts.tv_nsec = 0;

	ret = clock_settime(CLOCK_REALTIME, &ts);

	if (ret == -1)
	{
		LOGE("clock_settime failed :%d \n", ret);
		return -1;
	}

	return 0;
}

int time_to_time_str(time_t t, char *buffer, int buffer_len)
{
	struct tm now_tm = {0};

	if (!localtime_r(&t, &now_tm))
	{
		LOGE("localtime_r error \n");
		return -1;
	}

	return strftime(buffer, buffer_len, TIME_FORMAT, &now_tm) == 0 ? -1 : 0;
}

time_t time_str_to_time(const char *time_str)
{
	struct tm now_tm = {0};

	if (!strptime(time_str, TIME_FORMAT, &now_tm))
	{
		LOGE("strptime error \n");
		return (time_t) -1;
	}

	now_tm.tm_isdst = -1;

	return mktime(&now_tm);
}

int time_get_date_fmt(char *time_format, int format_len)
{
	struct timeval tv;
	struct tm now_tm;
	struct timezone tz;

	time_t now_sec;

	if (gettimeofday(&tv, &tz) == -1)
	{
		return -1;
	}

	now_sec = tv.tv_sec;

	if (!localtime_r(&now_sec, &now_tm))
	{
		return -1;
	}

	strftime(time_format, format_len, TIME_FORMAT, &now_tm);

	return 0;
}