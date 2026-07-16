
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

#include <linux/input.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>

#include "dpbase.h"

void DPInitCriticalSection(pthread_mutex_t* pcsCriticalSection)
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(pcsCriticalSection, &attr);
	pthread_mutexattr_destroy(&attr);
}

void DPDeleteCriticalSection(pthread_mutex_t* pcsCriticalSection)
{
	pthread_mutex_destroy(pcsCriticalSection);
}

void DPEnterCriticalSection(pthread_mutex_t* pcsCriticalSection)
{
	pthread_mutex_lock(pcsCriticalSection);
}

void DPLeaveCriticalSection(pthread_mutex_t* pcsCriticalSection)
{
	pthread_mutex_unlock(pcsCriticalSection);
}

void* DPCreateSemaphore(unsigned int dwInitCount, unsigned int dwMaxCount)
{
	sem_t *semid = (sem_t*)malloc(sizeof(sem_t));
	sem_init(semid, 0, dwInitCount);
	return semid;
}

void DPSetSemaphore(void* hSem)
{
	sem_post((sem_t *)hSem);
}

bool DPGetSemaphore(void* hSem, unsigned int timeout)
{
	struct timespec abstime;
	int wret;

	if(timeout == 0)
		wret = sem_trywait((sem_t *)hSem);
	else if(timeout == 0xFFFFFFFF)
		wret = sem_wait((sem_t *)hSem);
	else
	{
		clock_gettime(CLOCK_REALTIME, &abstime);
		abstime.tv_sec += timeout/1000;
		abstime.tv_nsec += (timeout % 1000) * 1000000;
		abstime.tv_sec += abstime.tv_nsec/1000000000;
		abstime.tv_nsec = abstime.tv_nsec%1000000000;
		wret = sem_timedwait((sem_t *)hSem, &abstime);
	}
	return (wret == 0);
}

unsigned int DPGetLastError()
{
	return errno;
}


typedef struct _MSG_Q
{
	unsigned int wptr;
	unsigned int rptr;
	unsigned int itemsize;
	unsigned int maxsize;
	unsigned int count;
	sem_t semid;
	pthread_mutex_t mutex;
	unsigned int tick;					// 创建次数，用来确定什么时候free
	char msgdata[0];
} MSG_Q;

bool DPCreateMsgQueue(const char* name, unsigned int vol, unsigned int itemsize, void** pRead, void** pWrite)
{
	MSG_Q* pMsg = (MSG_Q*)malloc(sizeof(MSG_Q) + vol * itemsize);
	memset(pMsg, 0, sizeof(MSG_Q) + vol * itemsize);
	pMsg->itemsize = itemsize;
	pMsg->maxsize = vol;
	sem_init(&pMsg->semid, 0, 0);
	pthread_mutex_init(&pMsg->mutex, NULL);
	if(pRead != NULL)
	{
		*pRead = pMsg;
		pMsg->tick++;
	}
	if(pWrite != NULL)
	{
		*pWrite = pMsg;
		pMsg->tick++;
	}
	return true;
}

bool DPWriteMsgQueue(void* hMsg, void* data, unsigned int len, unsigned int timeout)
{
	bool ret = false;
	MSG_Q* pMsg = (MSG_Q*)hMsg;
	if(len != pMsg->itemsize)
	{
		LOGE("DPWriteMsgQueue fail %d, %d \n", len, pMsg->itemsize);
		return false;
	}
	pthread_mutex_lock(&pMsg->mutex);
	if(pMsg->count < pMsg->maxsize)
	{
		memcpy(pMsg->msgdata + pMsg->wptr * pMsg->itemsize, data, len);
		pMsg->wptr++;
		pMsg->wptr %= pMsg->maxsize;
		pMsg->count++;
		sem_post(&pMsg->semid);
		ret = true;
	}
	else
	{
		LOGE("DPWriteMsgQueue fail 2 %d, %d \n", pMsg->count, pMsg->maxsize);
	}
	pthread_mutex_unlock(&pMsg->mutex);
	return ret;
}

bool DPReadMsgQueue(void* hMsg, void* data, unsigned int len, unsigned int timeout)
{
	MSG_Q* pMsg = (MSG_Q*)hMsg;
	struct timespec abstime;
	bool ret = false;
	int wret;

	if(timeout == 0)
		wret = sem_trywait(&pMsg->semid);
	else if(timeout == 0xFFFFFFFF)
		wret = sem_wait(&pMsg->semid);
	else
	{
		clock_gettime(CLOCK_REALTIME, &abstime);
		abstime.tv_sec += timeout/1000;
		abstime.tv_nsec += (timeout % 1000) * 1000000;
		abstime.tv_sec += abstime.tv_nsec/1000000000;
		abstime.tv_nsec = abstime.tv_nsec%1000000000;
		wret = sem_timedwait(&pMsg->semid, &abstime);
	}

	if(wret == 0)
	{
		pthread_mutex_lock(&pMsg->mutex);
		if(pMsg->count > 0)
		{
			memcpy(data, pMsg->msgdata + pMsg->rptr * pMsg->itemsize, len);
			pMsg->rptr++;
			pMsg->rptr %= pMsg->maxsize;
			pMsg->count--;
			ret = true;
		}
		pthread_mutex_unlock(&pMsg->mutex);
	}
	return ret;
}

void DPCloseMsgQueue(void* hMsg)
{
	MSG_Q* pMsg = (MSG_Q*)hMsg;
	if(pMsg->tick > 0)
	{
		pMsg->tick--;
		if(pMsg->tick == 0)
		{
			sem_destroy(&pMsg->semid);
			pthread_mutex_destroy(&pMsg->mutex);
			free(pMsg);
		}
	}

}


typedef unsigned int (threadfun) (void* pdata);

// static int PPIORITYMap[8] =
// {
// 	99,
// 	80,
// 	60,
// 	50,
// 	40,
// 	30,
// 	20,
// 	10
// };

typedef struct
{
	threadfun* pfunc;
	void* pdata;
	unsigned int priproty;
} ThreadInfo;

static void* threadthub(void* pdata)
{
	ThreadInfo* pinfo = (ThreadInfo*)pdata;
	unsigned int dRet;

	dRet = pinfo->pfunc(pinfo->pdata);
	free(pinfo);
	return (void*)dRet;
}

void* DPThreadCreate(int stacksize, unsigned int (*func) (void *), void *arg, bool isjoin, unsigned int level)
{
	pthread_t *thread = (pthread_t *) malloc(sizeof(pthread_t));
	pthread_attr_t attr;
	pthread_attr_init(&attr); /*初始化线程属性*/
	pthread_attr_setstacksize(&attr, stacksize);
	if(!isjoin)
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	ThreadInfo* pinfo = (ThreadInfo*)malloc(sizeof(ThreadInfo));
	pinfo->pfunc = func;
	pinfo->pdata = arg;
	pinfo->priproty = level;
	pthread_create(thread, &attr, threadthub, (void*)pinfo);
	pthread_attr_destroy(&attr);
	if(!isjoin)
	{
		free(thread);
		return NULL;
	}
	return thread;
}

bool DPThreadJoin(void* hThread, unsigned int* pRet)
{
	pthread_t *thread = (pthread_t *) hThread;
	bool bret;
	void* pret;

	if (thread == NULL)
		return true;
	bret =  (pthread_join(*thread, &pret) == 0);
	free(thread);
	if(pRet != NULL)
		*pRet = (unsigned int)pret;
	return bret;
}