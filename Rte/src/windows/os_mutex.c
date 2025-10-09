
#include <windows.h>
#include "os_mutex.h"

OsMutex_t* os_mutex_create(const char* pName)
{
    return CreateMutex(NULL, FALSE, pName);
}

ssize_t os_mutex_destroy(OsMutex_t* pMutex)
{
    return os_mutex_close(pMutex);
}

OsMutex_t* os_mutex_open(const char* pName)
{
    return OpenMutex(MUTEX_ALL_ACCESS, FALSE, pName);
}

ssize_t os_mutex_close(OsMutex_t* pMutex)
{
    if (CloseHandle(pMutex) != TRUE) {
        return -1;
    }

    return 0;
}

ssize_t os_mutex_take(OsMutex_t* pMutex)
{
    if (WaitForSingleObject(pMutex, INFINITE) != WAIT_OBJECT_0) {
        return -1;
    }

    return 0;
}

ssize_t os_mutex_give(OsMutex_t* pMutex)
{
    if (ReleaseMutex(pMutex) != TRUE) {
        return -1;
    }

    return 0;
}