#ifndef _DPBASELIB_H_
#define _DPBASELIB_H_

#include <sys/sem.h>
#include <pthread.h>
#define   SAFE_FREE(p) {if((p) != NULL){free(p);(p) = NULL;}}

/******************************************************************************
Function: Mutex_Init
Description: 初始化互斥锁
Param:
    pMutex in 互斥锁指针
Return: None
Others: None
******************************************************************************/
void Mutex_Init(pthread_mutex_t* pMutex);

/******************************************************************************
Function: Mutex_Destroy
Description: 销毁互斥锁
Param:
    pMutex in 互斥锁指针
Return: None
Others: None
******************************************************************************/
void Mutex_Destroy(pthread_mutex_t* pMutex);

/******************************************************************************
Function: Mutex_Lock
Description: 互斥锁加锁函数
Param:
    pMutex in 互斥锁指针
Return: None
Others: None
******************************************************************************/
void Mutex_Lock(pthread_mutex_t* pMutex);

/******************************************************************************
Function: Mutex_UnLock
Description: 互斥锁解锁函数
Param:
    pMutex in 互斥锁指针
Return: None
Others: None
******************************************************************************/
void Mutex_UnLock(pthread_mutex_t* pMutex);

int utf82unicode(unsigned short *dst, unsigned char *utf8);
void unicode2utf8(unsigned char *dst, unsigned short *unicode);

void DPCreateTimeEvent(void);

#endif //!_DPBASELIB_H_