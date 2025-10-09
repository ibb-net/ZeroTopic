
#include "os_mutex.h"

static char HandleType[] = {"BAREMETAL_MUTEX"};

OsMutex_t* os_mutex_create(const char* pName)
{
    return HandleType;
}

ssize_t os_mutex_destroy(OsMutex_t* pMutex)
{
    if (HandleType != pMutex) {
        return -1;
    }
    return 0;
}

ssize_t os_mutex_take(OsMutex_t* pMutex)
{
    if (HandleType != pMutex) {
        return -1;
    }
    return 0;
}

ssize_t os_mutex_give(OsMutex_t* pMutex)
{
    if (HandleType != pMutex) {
        return -1;
    }
    return 0;
}

OsMutex_t* os_mutex_open(const char* pName)
{
    (void)(pName);
    return NULL;
}

ssize_t os_mutex_close(OsMutex_t* pMutex)
{
    (void)(pMutex);
    return -1;
}
