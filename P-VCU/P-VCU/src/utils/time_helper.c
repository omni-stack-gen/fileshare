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

#include <time_helper.h>

#define LOG_TAG "time_helper"
#include <log.h>

uint64_t time_now(void)
{
	// struct timespec now;

	// clock_gettime(CLOCK_BOOTTIME, &now);
	// return now.tv_sec * MSEC_PER_SEC + now.tv_nsec / NSEC_PER_MSEC;
    struct timespec t_start;
	clock_gettime(CLOCK_MONOTONIC, &t_start);
	return (t_start.tv_sec * 1000 + t_start.tv_nsec / 1000000);
}

// bool time_expired(uint64_t time)
// {
// 	return time_before(time, time_now());
// }

uint32_t time_left(uint32_t end, uint32_t now)
{
    uint32_t t_left;

    if (end > now)
        t_left = end - now;
    else
        t_left = 0;

    return t_left;
}

uint32_t time_relative_ms()
{
    uint32_t ms;

	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

    ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000LL;

    return ms;
}

uint64_t time_relative_us()
{
    uint64_t us;

	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

    us = ts.tv_sec;
	us = us * 1000000LL;
	us += ts.tv_nsec / 1000;

    return us;
}

uint32_t time_abs_sec()
{
    struct timeval tv;
    uint32_t       sec;
    gettimeofday(&tv, NULL);
    sec = tv.tv_sec;
    return sec;
}

uint32_t time_abs_ms()
{
    struct timeval tv;
    uint32_t       ms;
    gettimeofday(&tv, NULL);
    ms = tv.tv_sec * 1000LL + tv.tv_usec / 1000;
    return ms;
}

uint64_t time_ads_us()
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

int time_get_local_time(struct tm *now_tm)
{
    if(!now_tm)
        return -1;

    struct tm tm_time;

    time_t timep;

    time(&timep);

    if(localtime_r(&timep, &tm_time))
    {
        tm_time.tm_year += 1900;
        tm_time.tm_mon += 1;

        memcpy(now_tm, &tm_time, sizeof(struct tm));

        return 0;
    }
    else
        LOGE("localtime failed \n");

    return -1;
}

int time_set_local_time(struct tm *now_tm)
{
    int ret = -1;

    if(!now_tm)
        return -1;

    struct timeval  tv;
    struct timezone tz;
    time_t          timep;
    struct tm       t;

    memset(&t, 0, sizeof(t));
    memcpy(&t, now_tm, sizeof(struct tm));

    t.tm_year -= 1900;
    t.tm_mon  -= 1;

    timep = mktime(&t);

    ret = gettimeofday(&tv, &tz);

    if(ret != 0)
    {
        LOGE("gettimeofday failed \n");
        return ret;
    }

    tv.tv_sec  = timep;
    tv.tv_usec = 0;

    ret = settimeofday(&tv, &tz);

	LOGI("%d-%02d-%02d %02d:%02d:%02d\n",
		    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
		    t.tm_hour, t.tm_min, t.tm_sec);

    if(ret != 0)
        LOGE("settimeofday failed\n");

    return ret;
}

int time_get_date_by_fmt(char *time_format, char *buffer, int buffer_len)
{
    struct timeval tv;
    struct tm now_tm;
    struct timezone tz;

    time_t now_sec;

    if(gettimeofday(&tv, &tz) == -1)
        return -1;

    now_sec = tv.tv_sec;

    if(!localtime_r(&now_sec, &now_tm))
        return -1;

    strftime(buffer, buffer_len, time_format, &now_tm);

    return 0;
}