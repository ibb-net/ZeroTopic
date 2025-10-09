

/***************************************************************************
 *
 * Copyright (c) 2021 ZelosTech.com, Inc. All Rights Reserved
 *
 **************************************************************************/

#ifndef _OS_MEMORY_MAP_H_
#define _OS_MEMORY_MAP_H_

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void*  pBuffer;
    size_t Length;

} OsMMap_t;

extern OsMMap_t* os_mmap_create(const char* pName, size_t Length);
extern ssize_t   os_mmap_destroy(OsMMap_t* pMMap);

extern OsMMap_t* os_mmap_open(const char* pName, size_t Length);
extern ssize_t   os_mmap_close(OsMMap_t* pMMap);

#ifdef __cplusplus
}
#endif

#endif
