
#include <windows.h>
#include "os_semaphore.h"

OsSemaphore_t* os_semaphore_create(size_t Init, const char* pName)
{
    return CreateSemaphore(NULL, Init, 0x7FFFFFFF, pName);
}

ssize_t os_semaphore_destroy(OsSemaphore_t* pSem)
{
    return os_semaphore_close(pSem);
}

OsSemaphore_t* os_semaphore_open(const char* pName)
{
    return OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, pName);
}

ssize_t os_semaphore_close(OsSemaphore_t* pSem)
{
    if (CloseHandle(pSem) != TRUE) {
        return -1;
    }

    return 0;
}

ssize_t os_semaphore_take(OsSemaphore_t* pSem, size_t WaitMs)
{
    ssize_t ExitCode = 0;

    if (WaitForSingleObject(pSem, WaitMs) == WAIT_OBJECT_0) {
        ExitCode++;
    }

    return ExitCode;
}

ssize_t os_semaphore_give(OsSemaphore_t* pSem)
{
    if (ReleaseSemaphore(pSem, 1, NULL) != TRUE) {
        return -1;
    }

    return 0;
}