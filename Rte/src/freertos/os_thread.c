
/***************************************************************************
 *
 * Copyright (c) 2021 ZelosTech.com, Inc. All Rights Reserved
 *
 **************************************************************************/

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "os_heap.h"
#include "os_thread.h"

OsThread_t* os_thread_create(OsThreadEntry_t Entry,
                             void* pParameter,
                             const ThreadAttr_t* pAttr)
{
    int RetVal = 0;
    TaskHandle_t* pTaskHandle = NULL;

    if (pAttr == NULL) {
        goto __error;
    }

    pTaskHandle = os_malloc(sizeof(TaskHandle_t));
    if (pTaskHandle == NULL) {
        goto __error;
    }

    RetVal = xTaskCreate((TaskFunction_t)Entry,
                         pAttr->pName,
                         pAttr->StackSize / sizeof(StackType_t),
                         pParameter,
                         pAttr->Priority,
                         pTaskHandle);
    if (RetVal != pdPASS) {
        goto __error;
    }

    return pTaskHandle;

__error:
    if (pTaskHandle != NULL) {
        os_free(pTaskHandle);
    }

    return NULL;
}

ssize_t os_thread_destroy(OsThread_t* pThread)
{
    TaskHandle_t* pTaskHandle = pThread;

    if (pTaskHandle != NULL) {
        vTaskDelete(*pTaskHandle);
        os_free(pThread);
    }

    return 0;
}

ssize_t os_thread_join(OsThread_t* pThread)
{
    /** Todo:  How to check task is exist; */
    return 0;
}

ssize_t os_thread_suspend(OsThread_t* pThread)
{
    TaskHandle_t* pTaskHandle = pThread;

    if (pTaskHandle == NULL) {
        return -1;
    }

    vTaskSuspend(*pTaskHandle);
    return 0;
}

ssize_t os_thread_resume(OsThread_t* pThread)
{
    TaskHandle_t* pTaskHandle = pThread;

    if (pTaskHandle == NULL) {
        return -1;
    }

    vTaskResume(*pTaskHandle);
    return 0;
}

void os_thread_sleep_ms(size_t Ms)
{
    size_t Ticks = 0;

    Ticks = (Ticks + portTICK_RATE_MS - 1) / portTICK_RATE_MS;
    vTaskDelay(Ms);
}

ssize_t os_thread_list(char* pBuffer, size_t BufferLength)
{
    ssize_t RetVal = 0;
    ssize_t ExitCode = 0;
    size_t Offset = 0;

    size_t i = 0;
    size_t TaskCounter = 0;
    TaskStatus_t* pTaskList = NULL;

    TaskCounter = uxTaskGetNumberOfTasks();
    pTaskList = os_malloc(TaskCounter * sizeof(TaskStatus_t));
    if (pTaskList == NULL) {
        ExitCode = -1;
        goto __error;
    }

    TaskCounter = uxTaskGetSystemState(pTaskList, TaskCounter, NULL);

    RetVal = snprintf(&pBuffer[Offset], BufferLength - Offset,
                      "No\tName\tPrio\tStack\n");
    if ((RetVal < 0) || (RetVal >= BufferLength - Offset)) {
        ExitCode = -2;
        goto __error;
    }
    Offset = Offset + RetVal;

    for (i = 0; i < TaskCounter; i++) {
        RetVal = snprintf(&pBuffer[Offset], BufferLength - Offset,
                          "%d\t%s\t%d\t%d\n",
                          pTaskList[i].xTaskNumber,
                          pTaskList[i].pcTaskName,
                          pTaskList[i].uxCurrentPriority,
                          pTaskList[i].usStackHighWaterMark * sizeof(StackType_t));
        if ((RetVal < 0) || (RetVal >= BufferLength - Offset)) {
            ExitCode = -3;
            goto __error;
        }
        Offset = Offset + RetVal;
    }

    vPortFree(pTaskList);
    return Offset;

__error:
    if (pTaskList != NULL) {
        vPortFree(pTaskList);
    }
    return ExitCode;
}

/**
 * @brief 获取当前线程句柄
 */
OsThread_t* os_thread_get_current(void)
{
    TaskHandle_t xCurrentTask = xTaskGetCurrentTaskHandle();
    if (xCurrentTask == NULL) {
        return NULL;
    }
    
    // 返回当前任务的句柄
    return (OsThread_t*)xCurrentTask;
}

/**
 * @brief 获取线程名称
 */
ssize_t os_thread_get_name(OsThread_t* pThread, char* pName, size_t name_length)
{
    if (pThread == NULL || pName == NULL || name_length == 0) {
        return -1;
    }
    
    TaskHandle_t xTask = (TaskHandle_t)pThread;
    const char* pcTaskName = pcTaskGetName(xTask);
    
    if (pcTaskName == NULL) {
        return -1;
    }
    
    strncpy(pName, pcTaskName, name_length - 1);
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
    
    TaskHandle_t xTask = (TaskHandle_t)pThread;
    return (ssize_t)uxTaskPriorityGet(xTask);
}

/* TODO: per thread cpu usage; all thread cpu usage;
 */
