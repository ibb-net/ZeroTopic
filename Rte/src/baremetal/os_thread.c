
#include "os_thread.h"

static os_sleep_t SleepFunc  = NULL;

ssize_t os_sleep_configure(os_sleep_t Sleep)
{
    if (Sleep == NULL) {
        return -1;
    }

    SleepFunc = Sleep;
    return 0;
}

void os_thread_sleep_ms(size_t Ms)
{
    if (SleepFunc != NULL) {
        (*SleepFunc)(Ms);
    }
}
