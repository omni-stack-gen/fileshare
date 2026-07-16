#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <time.h>
#include <linux/input.h>
#include <signal.h>
#include <dpbase.h>

unsigned int DPGetTickCount(void)
{
	struct timespec t_start;
	clock_gettime(CLOCK_MONOTONIC, &t_start);
	return (t_start.tv_sec * 1000 + t_start.tv_nsec / 1000000);
}

long long DPGetTickCount64()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
	long long t = ts.tv_sec*1000;
    return (t * 1000 + ts.tv_nsec / 1000);
}

void DPSleep(unsigned int dwMilliseconds)
{
	struct timespec ts;
    ts.tv_sec  =  dwMilliseconds / 1000;
    ts.tv_nsec = (dwMilliseconds % 1000) * 1000000LL;

    nanosleep(&ts, NULL);

	// unsigned int curtick;
	// unsigned int starttick = DPGetTickCount();
	// unsigned int leftms = dwMilliseconds;
	// while(1)
	// {
	// 	if(0 == usleep(leftms * 1000))
	// 		break;

	// 	curtick = DPGetTickCount();
	// 	if((curtick - starttick) > leftms)
	// 		break;

	// 	leftms -= curtick - starttick;
	// 	starttick = curtick;
	// }
}

void DPGetLocalTime(SYSTEMTIME* lpSystemTime)
{
	if(lpSystemTime)
	{
		struct tm *pst = NULL;
		time_t t = time(NULL);
		pst = localtime(&t);
		memcpy(lpSystemTime,pst,sizeof(SYSTEMTIME));
		lpSystemTime->wYear += 1900;
		lpSystemTime->wMonth += 1;
	}
}

bool DPSetLocalTime(SYSTEMTIME *st)
{
	struct timeval time_tv;
	time_t timep;
	int ret;
	struct tm pst = {0};

	memcpy(&pst, st, sizeof(struct tm));
	printf("SetLocalTime %d %d %d %d %d %d\r\n", st->wYear, st->wMonth, st->wDay, st->wHour, st->wMinute, st->wSecond);

	pst.tm_year -= 1900;
	pst.tm_mon -= 1;
	timep = mktime((struct tm*)&pst);
	time_tv.tv_sec = timep;
	time_tv.tv_usec = 0;

	ret = settimeofday(&time_tv, NULL);
	if(ret != 0)
	{
		printf("settimeofday failed %d\r\n", errno);
		return false;
	}
	else
	{
		//WriteRTC(&pst);
		return true;
	}
}

// bool DPSystemTimeToFileTime(SYSTEMTIME* lpSystemTime, FILETIME* lpFileTime)
// {
// 	time_t timep;
// 	unsigned long long ui;

// 	lpSystemTime->wYear -= 1900;
// 	lpSystemTime->wMonth -= 1;
// 	timep = mktime((struct tm*)lpSystemTime);
// 	ui = timep + SECOND_FROM_1601_TO_1970;
// 	ui *= 10000000;
// 	lpFileTime->dwHighDateTime = ui >> 32;
// 	lpFileTime->dwLowDateTime = ui;
// 	return true;
// }

// bool DPFileTimeToSystemTime(FILETIME* lpFileTime, SYSTEMTIME* lpSystemTime)
// {
// 	unsigned long long ui;
// 	time_t timep;
// 	struct tm *pst;

// 	ui = lpFileTime->dwHighDateTime;
// 	ui <<= 32;
// 	ui += lpFileTime->dwLowDateTime;
// 	ui /= 10000000;
// 	timep = ui - SECOND_FROM_1601_TO_1970;
// 	pst = localtime(&timep);
// 	memcpy(lpSystemTime,pst,sizeof(SYSTEMTIME));
// 	lpSystemTime->wYear += 1900;
// 	lpSystemTime->wMonth += 1;
// 	return true;
// }


// FILETIME timeToFileTime(const time_t *ptime)
// {
//     unsigned long long iTime = (unsigned long long)*ptime * 10000000 + SECOND_FROM_1601_TO_1970;
//     FILETIME ftime;
//     ftime.dwHighDateTime = (unsigned int)((iTime >> 32) & 0x00000000FFFFFFFF);
//     ftime.dwLowDateTime = (unsigned int)(iTime & 0x00000000FFFFFFFF);
//     return ftime;
// }