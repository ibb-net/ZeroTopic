
#include <pthread.h>
#include <unistd.h>
#include <string.h>
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

/**
 * @brief 获取当前线程句柄
 */
OsThread_t* os_thread_get_current(void)
{
    // 在POSIX中，当前线程就是pthread_self()的返回值
    // 但为了保持接口一致性，我们返回一个指向当前线程ID的指针
    static pthread_t current_thread;
    current_thread = pthread_self();
    return (OsThread_t*)&current_thread;
}

/**
 * @brief 获取线程名称
 */
ssize_t os_thread_get_name(OsThread_t* pThread, char* pName, size_t name_length)
{
    if (pThread == NULL || pName == NULL || name_length == 0) {
        return -1;
    }
    
    // 在POSIX中，获取线程名称比较复杂
    // 这里简化实现，返回默认名称
    strncpy(pName, "POSIX_Thread", name_length - 1);
    pName[name_length - 1] = '\0';
    
    return 0;
}

/**
 * @brief 获取线程优先级
 */
ssize_t os_thread_get_priority(OsThread_t* pThread)
{
    if (pThread == NULL) {
        return -1;
    }
    
    // 在POSIX中，获取线程优先级需要调用pthread_getschedparam
    // 这里简化实现，返回默认优先级
    return 0;
}