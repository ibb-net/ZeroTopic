
#include <pthread.h>
#include <unistd.h>
#include "os_heap.h"
#include "os_thread.h"

OsThread_t* os_thread_create(OsThreadEntry_t Entry,
                             void* pParameter,
                             const ThreadAttr_t* pAttr)
{
    (void)(pAttr);

    int RetVal   = 0;
    pthread_t* pThread = NULL;

    pThread = os_malloc(sizeof(pthread_t));
    if (pThread == NULL) {
        return NULL;
    }

    RetVal = pthread_create(pThread, NULL, Entry, pParameter);
    if (RetVal < 0) {
        return NULL;
    }

    return pThread;
}

ssize_t os_thread_join(OsThread_t* pThread)
{
    if (pThread == NULL) {
        return -1;
    }

    return pthread_join((*((pthread_t*)pThread)), NULL);
}

ssize_t os_thread_destroy(OsThread_t* pThread)
{
    (void)(pThread);
    return 0;
}

void os_thread_sleep_ms(size_t Ms)
{
    usleep(Ms * 1000);
}