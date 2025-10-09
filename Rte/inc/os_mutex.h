
/***************************************************************************
 *
 * Copyright (c) 2021 ZelosTech.com, Inc. All Rights Reserved
 *
 **************************************************************************/

#ifndef _OS_MUTEX_H_
#define _OS_MUTEX_H_

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void OsMutex_t;

extern OsMutex_t* os_mutex_create(const char* pName);
extern ssize_t    os_mutex_destroy(OsMutex_t* pMutex);

extern OsMutex_t* os_mutex_open(const char* pName);
extern ssize_t    os_mutex_close(OsMutex_t* pMutex);

extern ssize_t os_mutex_take(OsMutex_t* pMutex);
extern ssize_t os_mutex_give(OsMutex_t* pMutex);

#ifdef __cplusplus
}
#endif

#endif
