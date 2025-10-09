
/***************************************************************************
 *
 * Copyright (c) 2021 ZelosTech.com, Inc. All Rights Reserved
 *
 **************************************************************************/

#ifndef _OS_NAMED_H_
#define _OS_NAMED_H_

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern ssize_t os_named_point_configure(size_t MaxCounter);

extern ssize_t os_named_point_insert(const char* pName,
                                     void*       pPoint,
                                     size_t      Length);
extern void*   os_named_point_search(const char* pName,
                                     size_t      Length);
extern ssize_t os_named_point_delete(const char* pName,
                                     size_t      Length);

#ifdef __cplusplus
}
#endif

#endif
