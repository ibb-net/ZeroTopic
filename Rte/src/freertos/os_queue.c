/***************************************************************************
 *
 * Copyright (c) 2024 OpenIBBOs. All Rights Reserved
 *
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "os_queue.h"
#include "os_heap.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FreeRTOS队列内部结构体
 */
typedef struct {
    QueueHandle_t xQueue;          // FreeRTOS队列句柄
    char name[64];                 // 队列名称
    size_t queue_length;           // 队列最大长度
    size_t item_size;              // 元素大小
} OsQueueFreeRTOS_t;

/**
 * @brief 队列创建
 */
OsQueue_t* os_queue_create(size_t queue_length, size_t item_size, const char* pName)
{
    if (queue_length == 0 || item_size == 0) {
        return NULL;
    }
    
    // 分配队列结构体
    OsQueueFreeRTOS_t* pQueue = (OsQueueFreeRTOS_t*)os_malloc(sizeof(OsQueueFreeRTOS_t));
    if (pQueue == NULL) {
        return NULL;
    }
    
    // 创建FreeRTOS队列
    pQueue->xQueue = xQueueCreate(queue_length, item_size);
    if (pQueue->xQueue == NULL) {
        os_free(pQueue);
        return NULL;
    }
    
    // 设置队列属性
    pQueue->queue_length = queue_length;
    pQueue->item_size = item_size;
    
    if (pName != NULL) {
        strncpy(pQueue->name, pName, sizeof(pQueue->name) - 1);
        pQueue->name[sizeof(pQueue->name) - 1] = '\0';
    } else {
        snprintf(pQueue->name, sizeof(pQueue->name), "queue_%p", pQueue);
    }
    
    return (OsQueue_t*)pQueue;
}

/**
 * @brief 队列销毁
 */
ssize_t os_queue_destroy(OsQueue_t* pQueue)
{
    if (pQueue == NULL) {
        return -1;
    }
    
    OsQueueFreeRTOS_t* pFreeRTOSQueue = (OsQueueFreeRTOS_t*)pQueue;
    
    // 删除FreeRTOS队列
    vQueueDelete(pFreeRTOSQueue->xQueue);
    
    // 释放队列结构体
    os_free(pQueue);
    
    return 0;
}

/**
 * @brief 队列发送(任务模式)
 */
ssize_t os_queue_send(OsQueue_t* pQueue, const void* pItem, size_t timeout_ms)
{
    if (pQueue == NULL || pItem == NULL) {
        return -1;
    }
    
    OsQueueFreeRTOS_t* pFreeRTOSQueue = (OsQueueFreeRTOS_t*)pQueue;
    TickType_t xTicksToWait;
    
    // 转换超时时间
    if (timeout_ms == UINT32_MAX) {
        xTicksToWait = portMAX_DELAY;
    } else {
        xTicksToWait = pdMS_TO_TICKS(timeout_ms);
    }
    
    // 发送到FreeRTOS队列
    BaseType_t xResult = xQueueSend(pFreeRTOSQueue->xQueue, pItem, xTicksToWait);
    
    return (xResult == pdPASS) ? 0 : -1;
}

/**
 * @brief 队列发送(中断模式)
 */
ssize_t os_queue_send_isr(OsQueue_t* pQueue, const void* pItem)
{
    if (pQueue == NULL || pItem == NULL) {
        return -1;
    }
    
    OsQueueFreeRTOS_t* pFreeRTOSQueue = (OsQueueFreeRTOS_t*)pQueue;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // 从中断上下文发送到FreeRTOS队列
    BaseType_t xResult = xQueueSendFromISR(pFreeRTOSQueue->xQueue, pItem, &xHigherPriorityTaskWoken);
    
    // 如果需要，进行任务切换
    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    
    return (xResult == pdPASS) ? 0 : -1;
}

/**
 * @brief 队列接收
 */
ssize_t os_queue_receive(OsQueue_t* pQueue, void* pItem, size_t timeout_ms)
{
    if (pQueue == NULL || pItem == NULL) {
        return -1;
    }
    
    OsQueueFreeRTOS_t* pFreeRTOSQueue = (OsQueueFreeRTOS_t*)pQueue;
    TickType_t xTicksToWait;
    
    // 转换超时时间
    if (timeout_ms == UINT32_MAX) {
        xTicksToWait = portMAX_DELAY;
    } else {
        xTicksToWait = pdMS_TO_TICKS(timeout_ms);
    }
    
    // 从FreeRTOS队列接收
    BaseType_t xResult = xQueueReceive(pFreeRTOSQueue->xQueue, pItem, xTicksToWait);
    
    return (xResult == pdPASS) ? 0 : -1;
}

/**
 * @brief 获取队列当前元素数量
 */
ssize_t os_queue_get_count(OsQueue_t* pQueue)
{
    if (pQueue == NULL) {
        return -1;
    }
    
    OsQueueFreeRTOS_t* pFreeRTOSQueue = (OsQueueFreeRTOS_t*)pQueue;
    
    return (ssize_t)uxQueueMessagesWaiting(pFreeRTOSQueue->xQueue);
}

/**
 * @brief 获取队列剩余空间
 */
ssize_t os_queue_get_space(OsQueue_t* pQueue)
{
    if (pQueue == NULL) {
        return -1;
    }
    
    OsQueueFreeRTOS_t* pFreeRTOSQueue = (OsQueueFreeRTOS_t*)pQueue;
    
    return (ssize_t)uxQueueSpacesAvailable(pFreeRTOSQueue->xQueue);
}

/**
 * @brief 检查队列是否为空
 */
int os_queue_is_empty(OsQueue_t* pQueue)
{
    if (pQueue == NULL) {
        return -1;
    }
    
    OsQueueFreeRTOS_t* pFreeRTOSQueue = (OsQueueFreeRTOS_t*)pQueue;
    
    return (uxQueueMessagesWaiting(pFreeRTOSQueue->xQueue) == 0) ? 1 : 0;
}

/**
 * @brief 检查队列是否已满
 */
int os_queue_is_full(OsQueue_t* pQueue)
{
    if (pQueue == NULL) {
        return -1;
    }
    
    OsQueueFreeRTOS_t* pFreeRTOSQueue = (OsQueueFreeRTOS_t*)pQueue;
    
    return (uxQueueSpacesAvailable(pFreeRTOSQueue->xQueue) == 0) ? 1 : 0;
}

/**
 * @brief 重置队列
 */
ssize_t os_queue_reset(OsQueue_t* pQueue)
{
    if (pQueue == NULL) {
        return -1;
    }
    
    OsQueueFreeRTOS_t* pFreeRTOSQueue = (OsQueueFreeRTOS_t*)pQueue;
    
    // FreeRTOS没有直接的重置队列函数，需要清空所有消息
    // 这里通过循环接收所有消息来实现重置
    void* pTempItem = os_malloc(pFreeRTOSQueue->item_size);
    if (pTempItem == NULL) {
        return -1;
    }
    
    // 清空所有消息
    while (xQueueReceive(pFreeRTOSQueue->xQueue, pTempItem, 0) == pdPASS) {
        // 继续接收直到队列为空
    }
    
    os_free(pTempItem);
    
    return 0;
}

#ifdef __cplusplus
}
#endif
