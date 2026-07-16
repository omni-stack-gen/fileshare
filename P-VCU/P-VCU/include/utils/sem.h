/******************************************************************************
Copyright(c) 2011 - 2018 Digital Power Inc.
File name: sem.h
Author: liaosenlin
Version: 1.0.0
Date: 2018/10/04
Description: 简单的信号量封装函数声明
History:
Bug report: liaosenlin@d-power.com.cn
******************************************************************************/
#ifndef __SEM_H__
#define __SEM_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

/******************************************************************************
Function: CreateSemaphore
Description: 创建无名信号量
Param:
    Value in 初始化信号量值的大小
Return:
    失败返回NULL
    成功返回操作信号的句柄
Others: None
******************************************************************************/
void *CreateSemaphore(unsigned int Value);

/******************************************************************************
Function: SetSemaphore
Description: 设置信号量
Param:
    Sem in CreateSemaphore返回的句柄
Return: None
Others: None
******************************************************************************/
void SetSemaphore(void *Sem);

/******************************************************************************
Function: GetSemaphore
Description: 获取信号量
Param:
    Sem     in CreateSemaphore返回的句柄
    timeout in 获取信号量超时时间
Return:
    成功返回true
    失败返回false
Others: None
******************************************************************************/
bool GetSemaphore(void *Sem, unsigned int timeout);

/******************************************************************************
Function: DestorySemaphore
Description: 释放信号量
Param:
    Sem    in    CreateSemaphore返回的句柄
Return: None
Others: None
******************************************************************************/
void DestorySemaphore(void *Sem);

#ifdef __cplusplus
}
#endif

#endif //!__SEM_H__