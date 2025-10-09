
/***************************************************************************
 *
 * Copyright (c) 2021 ZelosTech.com, Inc. All Rights Reserved
 *
 **************************************************************************/

#include "FreeRTOS.h"
#include "os_heap.h"

void* os_malloc(size_t bytes)
{
    return pvPortMalloc(bytes);
}   

void  os_free(void* ptr)
{
    vPortFree(ptr);
}

ssize_t os_heap_size(void)
{
    return configTOTAL_HEAP_SIZE;
}

ssize_t os_heap_free_size(void)
{
    return xPortGetFreeHeapSize();
}
