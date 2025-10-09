
/***************************************************************************
 *
 * Copyright (c) 2021 ZelosTech.com, Inc. All Rights Reserved
 *
 **************************************************************************/

#ifndef _OS_HEAP_H_
#define _OS_HEAP_H_

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern ssize_t os_heap_configure(void*  pBuffer,
                                 size_t Length);

extern void* os_malloc(size_t bytes);
extern void  os_free(void* ptr);

extern ssize_t os_heap_size(void);
extern ssize_t os_heap_free_size(void);

#ifdef __cplusplus
}
#endif

#endif
