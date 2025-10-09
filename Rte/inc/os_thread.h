
/***************************************************************************
 *
 * Copyright (c) 2021 ZelosTech.com, Inc. All Rights Reserved
 *
 **************************************************************************/

#ifndef _OS_THREAD_H_
#define _OS_THREAD_H_

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*os_sleep_t)(size_t Ms);
extern ssize_t os_sleep_configure(os_sleep_t Sleep);

typedef struct {
    const char* pName;
    ssize_t Priority;
    ssize_t StackSize;
    ssize_t ScheduleType;

} ThreadAttr_t;

typedef void  OsThread_t;
typedef void* (*OsThreadEntry_t)(void*);

extern OsThread_t* os_thread_create(OsThreadEntry_t Entry,
                                    void* pParameter,
                                    const ThreadAttr_t* pAttr);
extern ssize_t os_thread_destroy(OsThread_t* pThread);

extern ssize_t os_thread_join(OsThread_t* pThread);
extern ssize_t os_thread_suspend(OsThread_t* pThread);
extern ssize_t os_thread_resume(OsThread_t* pThread);
extern void    os_thread_sleep_ms(size_t Ms);

extern ssize_t os_thread_list(char* pBuffer, size_t BufferLength);

#ifdef __cplusplus
}
#endif

#endif
