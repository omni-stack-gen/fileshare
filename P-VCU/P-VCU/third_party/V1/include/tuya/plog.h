#ifndef __PLOG_H__
#define __PLOG_H__

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>

typedef int SOCKET;
typedef unsigned int DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef unsigned int BOOL;
typedef unsigned long long UINT64;
typedef void* HANDLE;
typedef unsigned char UINT8;
typedef int LONG;
typedef DWORD COLORREF;
// typedef	socklen_t SOCKET_LEN;

#define	SOCKET_ERROR	-1
#define	TRUE	1
#define	FALSE	0
#define	INVALID_HANDLE_VALUE	(void*)0xffffffff
#define	INVALID_SOCKET			-1
#define MAX_PATH				256
#define	INFINITE				0xffffffff
#define	WINAPI

#define DEBUG_LOG 4
#define INFO_LOG 3
#define WARN_LOG 2
#define ERR_LOG 1

#define LOG(level, fmt, arg...)                                                       \
	{                                                                                 \
		printf("<tuya> %s (%s:%d) " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##arg); \
	}

#if DEBUG_LOG
#define debug(fmt, arg...) LOG(LOG_DEBUG, fmt, ##arg)
#else
#define debug(fmt, arg...)
#endif

#if ERR_LOG
#define error(fmt, arg...) LOG(LOG_ERR, fmt, ##arg)
#else
#define error(fmt, arg...)
#endif

#if WARN_LOG
#define warn(fmt, arg...) LOG(LOG_WARNING, fmt, ##arg)
#else
#define warn(fmt, arg...)
#endif

#if INFO_LOG
#define info(fmt, arg...) LOG(LOG_INFO, fmt, ##arg)
#else
#define info(fmt, arg...)
#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#ifndef SAFE_FREE
#define SAFE_FREE(p)    \
	do                  \
	{                   \
		if (p)          \
		{               \
			free(p);    \
			(p) = NULL; \
		}               \
	} while (0)
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

inline unsigned int GetTickCount()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	unsigned long long lt = ts.tv_sec;
	lt *= 1000;
	lt += (ts.tv_nsec / 1000000);
	return (unsigned int)lt;
}

inline unsigned long long GetTickCountus()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	unsigned long long lt = ts.tv_sec;
	lt *= 1000000;
	lt += (ts.tv_nsec / 1000);
	return lt;
}

inline void MySleep(long long Milliseconds)
{
	struct timespec ts;

	ts.tv_sec = Milliseconds / 1000;
	ts.tv_nsec = (Milliseconds % 1000) * 1000000LL;

	int iret = nanosleep(&ts, NULL);
	if (iret == -1)
	{
		error("MySleep %lld errno is %d\n", Milliseconds, errno);
	}
}

inline void MySleepus(long long Milliseconds)
{
	struct timespec ts;

	ts.tv_sec = Milliseconds / 1000;
	ts.tv_nsec = (Milliseconds % 1000) * 1000LL;

	int iret = nanosleep(&ts, NULL);
	if (iret == -1)
	{
		error("MySleep %lld errno is %d\n", Milliseconds, errno);
	}
}

#endif //__PLOG_H__