
/***************************************************************************
 *
 * Copyright (c) 2021 ZelosTech.com, Inc. All Rights Reserved
 *
 **************************************************************************/

#include "FreeRTOS.h"
#include "semphr.h"
#include "os_mutex.h"

OsMutex_t* os_mutex_create(const char* pName)
{
    if (pName != NULL) {
        return NULL;
    }

    return xSemaphoreCreateMutex();
}

ssize_t os_mutex_destroy(OsMutex_t* pMutex)
{
    vSemaphoreDelete(pMutex);
    return 0;
}

ssize_t os_mutex_take(OsMutex_t* pMutex)
{
    ssize_t RetVal = 0;

    RetVal = xSemaphoreTake(pMutex, portMAX_DELAY);
    if (RetVal != pdPASS) {
        return -1;
    }
    return 0;
}

ssize_t os_mutex_give(OsMutex_t* pMutex)
{
    ssize_t RetVal = 0;

    RetVal = xSemaphoreGive(pMutex);
    if (RetVal != pdPASS) {
        return -1;
    }
    return 0;
}
