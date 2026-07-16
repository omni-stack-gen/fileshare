#include <dpbaselib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

#include <systemmsg.h>
#include <dpbaselib.h>

static void TimeThread(union sigval v)
{
    DPPostMessage(TIME_MESSAGE, 0, 0, 0, MSG_TIME_TYPE);
}

void DPCreateTimeEvent(void)
{
    timer_t timerid;
    struct sigevent evp;
    memset(&evp, 0, sizeof(struct sigevent));

    evp.sigev_notify = SIGEV_THREAD;
    evp.sigev_notify_function = TimeThread;

    int ret = timer_create(CLOCK_REALTIME, &evp, &timerid);

    if(ret)
        perror("DPCreateTimeEvent fail\r\n");
    else
    {
        struct itimerspec ts;
        ts.it_interval.tv_sec = 1;
        ts.it_interval.tv_nsec = 0;
        ts.it_value.tv_sec = 1;
        ts.it_value.tv_nsec = 0;
        ret = timer_settime(timerid, 0, &ts, NULL);

        if(ret)
            perror("timer_settime");
    }

    printf("==== DPCreateTimeEvent \n");
}

void Mutex_Init(pthread_mutex_t* pMutex)
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);

	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(pMutex, &attr);
	pthread_mutexattr_destroy(&attr);
}

void Mutex_Destroy(pthread_mutex_t* pMutex)
{
	pthread_mutex_destroy(pMutex);
}

void Mutex_Lock(pthread_mutex_t* pMutex)
{
	pthread_mutex_lock(pMutex);
}

void Mutex_UnLock(pthread_mutex_t* pMutex)
{
	pthread_mutex_unlock(pMutex);
}



unsigned int hexconvert(char *data)
{
    char buf[16];
    unsigned int len;
    len = strlen(data);

    if(len < 7)
        return strtol(data, NULL, 16);
    else
    {
        unsigned int height, low;
        strcpy(buf, data);
        buf[len - 6] = 0;
        height = strtol(buf, NULL, 16);
        low = strtol(data + len - 6, NULL, 16);
        return ((height << 24) | low);
    }
}

char *CFindContent(char *start, char *match, char *ret, bool remove)
{
    char begin[32];
    char end[32];
    int leftlen;

    sprintf(begin, "<%s>", match);
    sprintf(end, "</%s>", match);
    start = strstr(start, begin);

    if(start == NULL)
        return NULL;

    match = strstr(start, end);

    if(match == NULL)
        return NULL;

    leftlen = match - start;
    leftlen -= strlen(begin);
    memcpy(ret, start + strlen(begin), leftlen);
    ret[leftlen] = 0;
    match += strlen(end);
    leftlen = strlen(match);
    memmove(start, match, leftlen);
    start[leftlen] = 0;
    return start;
}


int utf82unicode(unsigned short *dst, unsigned char *utf8)
{
    unsigned short *pStart = (unsigned short *)dst;

    while(utf8[0] != 0)
    {
        if((utf8[0] & 0xc0) == 0xc0)
        {
            if((utf8[0] & 0x20) == 0x20)
            {
                if((utf8[0] & 0x10) == 0x10)
                {
                    if((utf8[0] & 0x08) == 0x08)
                    {
                        if((utf8[0] & 0x04) == 0x04)
                        {
                            utf8 += 5;
                        }
                        else
                            utf8 += 4;
                    }
                    else
                        utf8 += 3;
                }
                else
                {
                    *dst = ((utf8[0] & 0xf) << 12) | ((utf8[1] & 0x3f) << 6) | (utf8[2] & 0x3f);
                    dst++;
                    utf8 += 3;
                }
            }
            else
            {
                *dst = ((utf8[0] & 0x1f) << 6) | (utf8[1] & 0x3f);
                dst++;
                utf8 += 2;
            }
        }
        else
        {
            *dst++ = *utf8++;
        }
    }

    *dst = 0;
    return dst - pStart;
}

void unicode2utf8(unsigned char *dst, unsigned short *unicode)
{
    unsigned short *ptr = NULL;
    ptr = (unsigned short *)unicode;

    while(*ptr != 0)
    {
        if(*ptr >= 0x0800)
        {
            *dst = 0xe0 | ((*ptr >> 12) & 0x0f);
            dst++;
            *dst = 0x80 | ((*ptr >> 6) & 0x3f);
            dst++;
            *dst = 0x80 | ((*ptr) & 0x3f);
            dst++;
            ptr++;
        }
        else if(*ptr >= 0x80 && *ptr <= 0x7ff)
        {
            *dst = 0xc0 | ((*ptr >> 6) & 0x1f);
            dst++;
            *dst = 0x80 | (*ptr & 0x3f);
            dst++;
            ptr++;
        }
        else
        {
            *dst = (unsigned char)(*ptr);
            dst++;
            ptr++;
        }
    }

    *dst = 0;
}

void unicode2wchar(wchar_t *dst, unsigned short *unicode)
{
    unsigned short *ptr = unicode;

    while(*ptr)
    {
        *dst++ = *ptr++;
    }

    *dst = 0;
}