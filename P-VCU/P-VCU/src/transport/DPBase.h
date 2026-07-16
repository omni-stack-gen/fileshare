#ifndef _DPBASE_H	// dpbase.h
#define _DPBASE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include "DPDef.h"
// #include "DevConfig.h"


// 平台初始化
void	DPPlatformInit();
void	DPPlatformDeinit();

// 时间
DWORD	DPGetTickCount();
long long DPGetTickCount64();
void	DPSleep(DWORD dwMilliseconds);
HANDLE	DPCreateTimer(void (*TimerCallBack)(void* arg), void* arg, DWORD dwMilliseconds);
void	DPCloseTimer(HANDLE hTimer);

// 线程
#define DP_PRIO_LOW			50
#define DP_PRIO_NORMAL		200
#define DP_PRIO_HIGH		RT_THREAD_PRIORITY_MAX

HANDLE	DPCreateThread(DWORD dwStackSize, const char* szName, void * (*func)(void* param), void* param/*, DWORD level*/);
BOOL	DPCloseThread(HANDLE hThread);
void	DPDetachThread(HANDLE hThread);

// 事件
HANDLE	DPCreateEvent(DWORD dwInitCount);
void	DPCloseEvent(HANDLE hEvent);
void	DPSetEvent(HANDLE hEvent);
int	DPGetEvent(HANDLE hEvent, DWORD dwMilliseconds);

// 信号量
HANDLE	DPCreateSemaphore(DWORD dwInitialCount, DWORD dwMaximumCount);
void	DPDeleteSemaphore(HANDLE hSemaphore);
void	DPSetSemaphore(HANDLE hSemaphore);
int	DPGetSemaphore(HANDLE hSemaphore, DWORD dwMilliseconds);

// 互斥
HANDLE	DPCreateMutex();
void	DPDeleteMutex(HANDLE hMutex);
void	DPLockMutex(HANDLE hMutex);
void	DPUnlockMutex(HANDLE hMutex);

//邮箱
HANDLE	DPCreateMailbox(int size);
void	DPDeleteMailbox(HANDLE hMailbox);
int 	DPSendMailbox(HANDLE hMailbox,unsigned int value,unsigned int timeout);
int 	DPGetMailbox(HANDLE hMailbox,unsigned int * lpvalue,unsigned int timeout);

//消息队列
HANDLE	DPCreateMessagequeue();
void	DPDeleteMessagequeue(HANDLE hMailbox);
int 	DPSendMessagequeue(HANDLE hMailbox,void* buffer, int size,unsigned int timeout);
int 	DPRecvMessagequeue(HANDLE hMailbox,void* buffer, int size,unsigned int timeout);


#endif	// dpbase.h

