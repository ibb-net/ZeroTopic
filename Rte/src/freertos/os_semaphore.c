
/***************************************************************************
 *
 * Copyright (c) 2021 ZelosTech.com, Inc. All Rights Reserved
 *
 **************************************************************************/

#include "FreeRTOS.h"
#include "semphr.h"
#include "os_semaphore.h"

OsSemaphore_t* os_semaphore_create(size_t Init, const char* pName)
{
    if (pName != NULL) {
        return NULL;
    }

    return xSemaphoreCreateCounting((UBaseType_t)(-1), Init);
}

ssize_t os_semaphore_destroy(OsSemaphore_t* pSem)
{
    vSemaphoreDelete(pSem);
    return 0;
}

ssize_t os_semaphore_take(OsSemaphore_t* pSem, size_t WaitMs)
{
    ssize_t RetVal    = 0;
    ssize_t ExitCode  = 0;
    size_t  WaitTicks = 0;

    if (pSem == NULL) {
        ExitCode = -1;
        goto __exit;
    }

    WaitTicks = (WaitMs + portTICK_RATE_MS - 1) / portTICK_RATE_MS;

    RetVal = xSemaphoreTake(pSem, WaitTicks);
    if (RetVal == pdPASS) {
        ExitCode++;
    }

__exit:
    return ExitCode;
}

ssize_t os_semaphore_give(OsSemaphore_t* pSem)
{
    ssize_t RetVal    = 0;

    RetVal = xSemaphoreGive(pSem);
    if (RetVal != pdPASS) {
        return -1;
    }

    return 0;
}

ssize_t os_semaphore_give_isr(OsSemaphore_t* pSem)
{
    ssize_t    RetVal = 0;
    BaseType_t Yield  = 0;

    RetVal = xSemaphoreGiveFromISR(pSem, &Yield);
    if (RetVal != pdPASS) {
        return -1;
    }

    portYIELD_FROM_ISR(Yield);
    return 0;
}
