
#include <stdlib.h>
#include "os_heap.h"

static size_t malloc_counter = 0;
static size_t malloc_free = 0;

void* os_malloc(size_t bytes)
{
    malloc_counter++;
    return malloc(bytes);
}

void  os_free(void* ptr)
{
    malloc_free++;
    free(ptr);
}