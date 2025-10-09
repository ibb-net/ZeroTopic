
#include <pthread.h>
#include "os_heap.h"
#include "os_mmap.h"
#include "os_mutex.h"

typedef struct {
    const char* pHandleType;
    OsMMap_t*   pMMap;
    pthread_mutex_t* pMutex;
} InMutex_t;

static const char HandleType[] = {"LINUX_MUTEX"};

OsMutex_t* os_mutex_create(const char* pName)
{
    InMutex_t* pInMutex = NULL;
    OsMMap_t* pMMap = NULL;
    pthread_mutex_t* pMutex = NULL;
    pthread_mutexattr_t MutexAttr;

    if (pName == NULL) {
        pMutex = os_malloc(sizeof(pthread_mutex_t));
        if (pMutex == NULL) {
            goto __error;
        }

        if (pthread_mutex_init(pMutex, NULL) < 0) {
            goto __error;
        }
    } else {
        if (pthread_mutexattr_init(&MutexAttr) < 0) {
            goto __error;
        }
        if (pthread_mutexattr_setpshared(&MutexAttr, PTHREAD_PROCESS_SHARED) < 0) {
            goto __error;
        }

        pMMap = os_mmap_create(pName, sizeof(pthread_mutex_t));
        if (pMMap == NULL) {
            goto __error;
        }

        pMutex = (pthread_mutex_t*)(pMMap->pBuffer);
        if (pthread_mutex_init(pMutex, &MutexAttr) < 0) {
            goto __error;
        }
    }

    pInMutex = os_malloc(sizeof(InMutex_t));
    if (pInMutex == NULL) {
        goto __error;
    }

    pInMutex->pHandleType = HandleType;
    pInMutex->pMMap = pMMap;
    pInMutex->pMutex = pMutex;
    return pInMutex;

__error:
    if (pInMutex != NULL) {
        os_free(pInMutex);
    }

    if (pMutex != NULL) {
        pthread_mutex_destroy(pMutex);
        if (pMMap == NULL) {
            os_free(pMutex);
        }
    }

    if (pMMap != NULL) {
        os_mmap_destroy(pMMap);
    }
    return NULL;
}

ssize_t os_mutex_destroy(OsMutex_t* pMutex)
{
    InMutex_t* pInMutex = (InMutex_t*)pMutex;

    if (pInMutex == NULL) {
        return 0;
    }

    if (pInMutex->pHandleType != HandleType) {
        return -1;
    }

    if (pInMutex->pMutex != NULL) {
        pthread_mutex_destroy(pInMutex->pMutex);
        if (pInMutex->pMMap == NULL) {
            os_free(pInMutex->pMutex);
        }
    }

    if (pInMutex->pMMap != NULL) {
        os_mmap_destroy(pInMutex->pMMap);
    }

    os_free(pInMutex);
    return 0;
}


OsMutex_t* os_mutex_open(const char* pName)
{
    InMutex_t* pInMutex = NULL;
    OsMMap_t* pMMap = NULL;
    pthread_mutex_t* pMutex = NULL;

    pMMap = os_mmap_open(pName, sizeof(pthread_mutex_t));
    if (pMMap == NULL) {
        goto __error;
    }

    pMutex = (pthread_mutex_t*)(pMMap->pBuffer);
    if (pMutex == NULL) {
        goto __error;
    }

    pInMutex = os_malloc(sizeof(InMutex_t));
    if (pInMutex == NULL) {
        goto __error;
    }

    pInMutex->pHandleType = HandleType;
    pInMutex->pMMap = pMMap;
    pInMutex->pMutex = pMutex;
    return pInMutex;

__error:
    if (pInMutex != NULL) {
        os_free(pInMutex);
    }

    if (pMMap != NULL) {
        os_mmap_close(pMMap);
    }
    return NULL;
}


ssize_t os_mutex_close(OsMutex_t* pMutex)
{
    InMutex_t* pInMutex = (InMutex_t*)pMutex;

    if (pInMutex == NULL) {
        return 0;
    }

    if (pInMutex->pHandleType != HandleType) {
        return -1;
    }

    if (pInMutex->pMMap != NULL) {
        os_mmap_close(pInMutex->pMMap);
    }

    os_free(pInMutex);
    return 0;
}

ssize_t os_mutex_take(OsMutex_t* pMutex)
{
    InMutex_t* pInMutex = (InMutex_t*)pMutex;
    if (pInMutex == NULL) {
        return -1;
    }

    if (pInMutex->pHandleType != HandleType) {
        return -2;
    }

    return pthread_mutex_lock(pInMutex->pMutex);
}

ssize_t os_mutex_give(OsMutex_t* pMutex)
{
    InMutex_t* pInMutex = (InMutex_t*)pMutex;
    if (pInMutex == NULL) {
        return -1;
    }

    if (pInMutex->pHandleType != HandleType) {
        return -2;
    }

    return pthread_mutex_unlock(pInMutex->pMutex);
}