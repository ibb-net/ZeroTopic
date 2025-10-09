
/***************************************************************************
 *
 * Copyright (c) 2021 ZelosTech.com, Inc. All Rights Reserved
 *
 **************************************************************************/

#ifndef _OS_SEMAPHORE_H_
#define _OS_SEMAPHORE_H_

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void OsSemaphore_t;

extern OsSemaphore_t* os_semaphore_create(size_t Init, const char* pName);
extern ssize_t os_semaphore_destroy(OsSemaphore_t* pSem);

extern OsSemaphore_t* os_semaphore_open(const char* pName);
extern ssize_t os_semaphore_close(OsSemaphore_t* pSem);

extern ssize_t os_semaphore_take(OsSemaphore_t* pSem, size_t WaitMs);
extern ssize_t os_semaphore_give(OsSemaphore_t* pSem);

extern ssize_t os_semaphore_give_isr(OsSemaphore_t* pSem);

#ifdef __cplusplus
}
#endif

#endif
