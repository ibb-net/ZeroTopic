
#include <semaphore.h>
#include <time.h>
#include <stdint.h>
#include "os_heap.h"
#include "os_mmap.h"
#include "os_semaphore.h"

typedef struct {
    const char* pHandleType;
    OsMMap_t* pMMap;
    sem_t* pSem;
} InSem_t;

static const char HandleType[] = {"LINUX_SEMAPHORE"};

OsSemaphore_t* os_semaphore_create(size_t Init, const char* pName)
{
    InSem_t* pInSem = NULL;
    OsMMap_t* pMMap = NULL;
    sem_t* pSem = NULL;

    if (pName == NULL) {
        pSem = os_malloc(sizeof(sem_t));
        if (pSem == NULL) {
            goto __error;
        }

        if (sem_init(pSem, 0, Init) < 0) {
            goto __error;
        }
    } else {
        pMMap = os_mmap_create(pName, sizeof(sem_t));
        if (pMMap == NULL) {
            goto __error;
        }

        pSem = (sem_t*)(pMMap->pBuffer);
        if (sem_init(pSem, 1, Init) < 0) {
            goto __error;
        }
    }

    pInSem = os_malloc(sizeof(InSem_t));
    if (pInSem == NULL) {
        goto __error;
    }

    pInSem->pHandleType = HandleType;
    pInSem->pMMap = pMMap;
    pInSem->pSem = pSem;
    return pInSem;

__error:
    if (pInSem != NULL) {
        os_free(pInSem);
    }

    if (pSem != NULL) {
        sem_destroy(pSem);
        if (pMMap == NULL) {
            os_free(pSem);
        }
    }

    if (pMMap != NULL) {
        os_mmap_destroy(pMMap);
    }
    return NULL;
}

ssize_t os_semaphore_destroy(OsSemaphore_t* pSem)
{
    InSem_t* pInSem = (InSem_t*)pSem;

    if (pInSem == NULL) {
        return 0;
    }

    if (pInSem->pHandleType != HandleType) {
        return -1;
    }

    if (pInSem->pSem != NULL) {
        sem_destroy(pInSem->pSem);
        if (pInSem->pMMap == NULL) {
            os_free(pInSem->pSem);
        }
    }

    if (pInSem->pMMap != NULL) {
        os_mmap_destroy(pInSem->pMMap);
    }

    os_free(pInSem);
    return 0;
}


OsSemaphore_t* os_semaphore_open(const char* pName)
{
    InSem_t* pInSem = NULL;
    OsMMap_t* pMMap = NULL;
    sem_t* pSem = NULL;

    pMMap = os_mmap_open(pName, sizeof(sem_t));
    if (pMMap == NULL) {
        goto __error;
    }

    pSem = (sem_t*)(pMMap->pBuffer);
    if (pSem == NULL) {
        goto __error;
    }

    pInSem = os_malloc(sizeof(InSem_t));
    if (pInSem == NULL) {
        goto __error;
    }

    pInSem->pHandleType = HandleType;
    pInSem->pMMap = pMMap;
    pInSem->pSem = pSem;
    return pInSem;

__error:
    if (pInSem != NULL) {
        os_free(pInSem);
    }

    if (pMMap != NULL) {
        os_mmap_close(pMMap);
    }
    return NULL;
}


ssize_t os_semaphore_close(OsSemaphore_t* pSem)
{
    InSem_t* pInSem = (InSem_t*)pSem;

    if (pInSem == NULL) {
        return 0;
    }

    if (pInSem->pHandleType != HandleType) {
        return -1;
    }

    if (pInSem->pMMap != NULL) {
        os_mmap_close(pInSem->pMMap);
    }

    os_free(pInSem);
    return 0;
}

ssize_t os_semaphore_take(OsSemaphore_t* pSem, size_t WaitMs)
{
    InSem_t* pInSem = (InSem_t*)pSem;

    int      RetVal   = 0;
    int      ExitCode = 0;
    int      SemVal   = 0;
    uint64_t WaitNs = WaitMs * 1000000;
    struct timespec TimeSpec;

    if (pInSem == NULL) {
        return -1;
    }

    if (pInSem->pHandleType != HandleType) {
        return -2;
    }

    if (WaitMs == 0) {
        RetVal = sem_getvalue(pInSem->pSem, &SemVal);
        if (RetVal < 0) {
            return -3;
        }
        if (SemVal > 0) {
            ExitCode++;
        }

    } else {
        RetVal = clock_gettime(CLOCK_REALTIME, &TimeSpec);
        if (RetVal < 0) {
            return -4;
        }

        TimeSpec.tv_sec  = TimeSpec.tv_sec + (WaitNs / 1000000000);
        TimeSpec.tv_nsec = TimeSpec.tv_nsec + (WaitNs % 1000000000);
        TimeSpec.tv_sec  = TimeSpec.tv_sec + TimeSpec.tv_nsec / 1000000000;
        TimeSpec.tv_nsec = TimeSpec.tv_nsec % 1000000000;

        /* Warning:
         * when sempahore = 0 and WaitMs = 0,
         * call sem_timedwait takes many times
        */
        RetVal = sem_timedwait(pInSem->pSem, &TimeSpec);
        if (RetVal >= 0) {
            ExitCode++;
        }
    }

    return ExitCode;
}

ssize_t os_semaphore_give(OsSemaphore_t* pSem)
{
    InSem_t* pInSem = (InSem_t*)pSem;

    if (pInSem == NULL) {
        return -1;
    }

    if (pInSem->pHandleType != HandleType) {
        return -2;
    }

    return sem_post(pInSem->pSem);
}