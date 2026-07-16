/******************************************************************************
Copyright(c) 2011 - 2018 Digital Power Inc.
File name: sem.c
Author: liaosenlin
Version: 1.0.0
Date: 2018/10/04
Description: 简单的信号量封装函数实现
History:
Bug report: liaosenlin@d-power.com.cn
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <semaphore.h>
#include <sys/time.h>
#include <time.h>

#include <sem.h>
#include <bmem.h>
// #include <dpcommon.h>

//#include <memwatch.h>

void *CreateSemaphore(unsigned int Value)
{
    sem_t *semid = (sem_t *)bmalloc(sizeof(sem_t));

    if(semid == NULL)
        return NULL;

    sem_init(semid, 0, Value); //无名信号量

    return (void*)semid;
}

void SetSemaphore(void *Sem)
{
    if(Sem != NULL)
        sem_post((sem_t *)Sem);
}

bool GetSemaphore(void *Sem, unsigned int timeout)
{
    if(Sem == NULL)
        return false;

    struct timespec abstime;

    int wret;

    if(timeout == 0)
        wret = sem_trywait((sem_t *)Sem);
    else if(timeout == 0xffffffff)
        wret = sem_wait((sem_t *)Sem);
    else
    {
        clock_gettime(CLOCK_REALTIME, &abstime);
        abstime.tv_sec += timeout / 1000;
        abstime.tv_nsec += (timeout % 1000) * 1000000;
        abstime.tv_sec += abstime.tv_nsec / 1000000000;
        abstime.tv_nsec = abstime.tv_nsec % 1000000000;
        wret = sem_timedwait((sem_t *)Sem, &abstime);
    }

    return (wret == 0);
}

void DestorySemaphore(void *Sem)
{
    if(Sem == NULL)
        return ;

    sem_destroy((sem_t *)Sem);

    bfree(Sem);
    Sem = NULL;
}