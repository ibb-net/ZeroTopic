
#include <windows.h>
#include "os_thread.h"

OsThread_t* os_thread_create(OsThreadEntry_t Entry,
                             void* pParameter,
                             const ThreadAttr_t* pAttr)
{
    (void)(pAttr);

    return CreateThread(NULL, 0,
                        (LPTHREAD_START_ROUTINE)Entry,
                        pParameter, 0, NULL);
}

ssize_t os_thread_join(OsThread_t* pThread)
{
    if (WaitForSingleObject(pThread, INFINITE) != WAIT_OBJECT_0) {
        return -1;
    }

    return 0;
}

ssize_t os_thread_destroy(OsThread_t* pThread)
{
    return 0;
}

void os_thread_sleep_ms(size_t Ms)
{
    Sleep(Ms);
}