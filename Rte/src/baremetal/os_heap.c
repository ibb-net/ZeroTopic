
#include <stdint.h>
#include "tlsf.h"
#include "os_interrupt.h"
#include "os_heap.h"

static tlsf_t gpTLSF = 0;

static size_t gMallocCounter = 0;
static size_t gMallocFree = 0;

ssize_t os_heap_configure(void*  pBuffer,
                          size_t Length)
{
    gpTLSF = tlsf_create_with_pool(pBuffer, Length);
    if (gpTLSF == NULL) {
        return -1;
    }

    return 0;
}

void* os_malloc(size_t bytes)
{
    void* pPoint = NULL;

    if (gpTLSF == NULL) {
        return NULL;
    }
    gMallocCounter++;

    os_critical_enter();
    pPoint = tlsf_malloc(gpTLSF, bytes);
    os_critical_exit();

    return pPoint;
}

void  os_free(void* ptr)
{
    if (gpTLSF == NULL) {
        return;
    }

    gMallocFree++;

    os_critical_enter();
    tlsf_free(gpTLSF, ptr);
    os_critical_exit();
}
