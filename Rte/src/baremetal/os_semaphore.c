
#include "os_interrupt.h"
#include "os_heap.h"
#include "os_thread.h"
#include "os_semaphore.h"

typedef struct {
    const char* pHandleType;
    size_t      Counter;
} InOsSemaphore_t;

static char HandleType[] = {"BAREMETAL_SEMAPHORE"};

OsSemaphore_t* os_semaphore_create(size_t Init, const char* pName)
{
    InOsSemaphore_t* pInOsSemaphore = NULL;

    if (pName != NULL) {
        goto __error;
    }

    pInOsSemaphore = os_malloc(sizeof(InOsSemaphore_t));
    if (pInOsSemaphore == NULL) {
        goto __error;
    }

    pInOsSemaphore->pHandleType = HandleType;
    pInOsSemaphore->Counter     = Init;
    return pInOsSemaphore;

__error:
    if (pInOsSemaphore != NULL) {
        os_free(pInOsSemaphore);
    }
    return NULL;
}

ssize_t os_semaphore_destroy(OsSemaphore_t* pSem)
{
    InOsSemaphore_t* pInOsSemaphore = pSem;

    if (pInOsSemaphore == NULL) {
        return 0;
    }

    if (pInOsSemaphore->pHandleType != HandleType) {
        return -1;
    }

    os_free(pSem);
    return 0;
}

ssize_t os_semaphore_take(OsSemaphore_t* pSem, size_t WaitMs)
{
    InOsSemaphore_t* pInOsSemaphore = pSem;

    size_t i       = 0;
    size_t Counter = 0;

    if (pInOsSemaphore == NULL) {
        return -1;
    }

    if (pInOsSemaphore->pHandleType != HandleType) {
        return -2;
    }

    for (i = 0; i < WaitMs; i++) {
        os_critical_enter();
        Counter = pInOsSemaphore->Counter;
        if (Counter > 0) {
            pInOsSemaphore->Counter = Counter - 1;
        }
        os_critical_exit();

        if (Counter > 0) {
            break;
        }

        os_thread_sleep_ms(1);
    }

    return Counter;
}

ssize_t os_semaphore_give(OsSemaphore_t* pSem)
{
    InOsSemaphore_t* pInOsSemaphore = pSem;

    if (pInOsSemaphore == NULL) {
        return -1;
    }

    if (pInOsSemaphore->pHandleType != HandleType) {
        return -2;
    }

    os_critical_enter();
    pInOsSemaphore->Counter = pInOsSemaphore->Counter + 1;
    os_critical_exit();

    return 0;
}

ssize_t os_semaphore_give_isr(OsSemaphore_t* pSem)
{
    InOsSemaphore_t* pInOsSemaphore = pSem;

    if (pInOsSemaphore == NULL) {
        return -1;
    }

    if (pInOsSemaphore->pHandleType != HandleType) {
        return -2;
    }

    pInOsSemaphore->Counter = pInOsSemaphore->Counter + 1;
    return 0;
}

OsSemaphore_t* os_semaphore_open(const char* pName)
{
    (void)(pName);
    return NULL;
}

ssize_t os_semaphore_close(OsSemaphore_t* pSem)
{
    (void)(pSem);
    return -1;
}
