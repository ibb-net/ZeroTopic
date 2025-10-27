/***************************************************************************
 *
 * Copyright (c) 2024 OpenIBBOs. All Rights Reserved
 *
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>

#include "os_queue.h"
#include "../../../Middlewares/common/ringbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief POSIX队列内部结构体
 */
typedef struct {
    ringbuf_t ringbuf;             // 环形缓冲区
    pthread_mutex_t mutex;         // 互斥锁
    pthread_cond_t send_cond;      // 发送条件变量
    pthread_cond_t recv_cond;      // 接收条件变量
    char name[64];                 // 队列名称
    size_t queue_length;           // 队列最大长度
    size_t item_size;              // 元素大小
    int destroyed;                 // 销毁标志
} OsQueuePosix_t;

/**
 * @brief 计算超时时间
 */
static int calculate_timeout(struct timespec* pTimeout, size_t timeout_ms)
{
    if (timeout_ms == 0) {
        return 0; // 立即返回
    }
    
    if (timeout_ms == UINT32_MAX) {
        return 1; // 无限等待
    }
    
    // 获取当前时间
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        return -1;
    }
    
    // 计算绝对超时时间
    pTimeout->tv_sec = tv.tv_sec + timeout_ms / 1000;
    pTimeout->tv_nsec = (tv.tv_usec * 1000) + ((timeout_ms % 1000) * 1000000);
    
    // 处理纳秒溢出
    if (pTimeout->tv_nsec >= 1000000000) {
        pTimeout->tv_sec++;
        pTimeout->tv_nsec -= 1000000000;
    }
    
    return 1;
}

/**
 * @brief 队列创建
 */
OsQueue_t* os_queue_create(size_t queue_length, size_t item_size, const char* pName)
{
    if (queue_length == 0 || item_size == 0) {
        return NULL;
    }
    
    // 分配队列结构体
    OsQueuePosix_t* pQueue = (OsQueuePosix_t*)malloc(sizeof(OsQueuePosix_t));
    if (pQueue == NULL) {
        return NULL;
    }
    
    // 初始化环形缓冲区
    if (ringbuf_init_malloc(&pQueue->ringbuf, queue_length, item_size) != 0) {
        free(pQueue);
        return NULL;
    }
    
    // 初始化互斥锁
    if (pthread_mutex_init(&pQueue->mutex, NULL) != 0) {
        ringbuf_destroy(&pQueue->ringbuf, true);
        free(pQueue);
        return NULL;
    }
    
    // 初始化条件变量
    if (pthread_cond_init(&pQueue->send_cond, NULL) != 0) {
        pthread_mutex_destroy(&pQueue->mutex);
        ringbuf_destroy(&pQueue->ringbuf, true);
        free(pQueue);
        return NULL;
    }
    
    if (pthread_cond_init(&pQueue->recv_cond, NULL) != 0) {
        pthread_cond_destroy(&pQueue->send_cond);
        pthread_mutex_destroy(&pQueue->mutex);
        ringbuf_destroy(&pQueue->ringbuf, true);
        free(pQueue);
        return NULL;
    }
    
    // 设置队列属性
    pQueue->queue_length = queue_length;
    pQueue->item_size = item_size;
    pQueue->destroyed = 0;
    
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
    
    OsQueuePosix_t* pPosixQueue = (OsQueuePosix_t*)pQueue;
    
    // 加锁
    if (pthread_mutex_lock(&pPosixQueue->mutex) != 0) {
        return -1;
    }
    
    // 设置销毁标志
    pPosixQueue->destroyed = 1;
    
    // 唤醒所有等待的线程
    pthread_cond_broadcast(&pPosixQueue->send_cond);
    pthread_cond_broadcast(&pPosixQueue->recv_cond);
    
    // 解锁
    pthread_mutex_unlock(&pPosixQueue->mutex);
    
    // 等待所有线程退出
    pthread_mutex_destroy(&pPosixQueue->mutex);
    pthread_cond_destroy(&pPosixQueue->send_cond);
    pthread_cond_destroy(&pPosixQueue->recv_cond);
    
    // 销毁环形缓冲区
    ringbuf_destroy(&pPosixQueue->ringbuf, true);
    
    // 释放队列结构体
    free(pQueue);
    
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
    
    OsQueuePosix_t* pPosixQueue = (OsQueuePosix_t*)pQueue;
    struct timespec timeout;
    int timeout_result;
    
    // 计算超时时间
    timeout_result = calculate_timeout(&timeout, timeout_ms);
    if (timeout_result < 0) {
        return -1;
    }
    
    // 加锁
    if (pthread_mutex_lock(&pPosixQueue->mutex) != 0) {
        return -1;
    }
    
    // 等待队列有空间
    while (ringbuf_is_full(&pPosixQueue->ringbuf) && !pPosixQueue->destroyed) {
        if (timeout_result == 0) {
            // 立即返回
            pthread_mutex_unlock(&pPosixQueue->mutex);
            return -1;
        } else if (timeout_result == 1 && timeout_ms != UINT32_MAX) {
            // 超时等待
            int result = pthread_cond_timedwait(&pPosixQueue->send_cond, &pPosixQueue->mutex, &timeout);
            if (result == ETIMEDOUT) {
                pthread_mutex_unlock(&pPosixQueue->mutex);
                return -1;
            } else if (result != 0) {
                pthread_mutex_unlock(&pPosixQueue->mutex);
                return -1;
            }
        } else {
            // 无限等待
            if (pthread_cond_wait(&pPosixQueue->send_cond, &pPosixQueue->mutex) != 0) {
                pthread_mutex_unlock(&pPosixQueue->mutex);
                return -1;
            }
        }
    }
    
    // 检查是否已销毁
    if (pPosixQueue->destroyed) {
        pthread_mutex_unlock(&pPosixQueue->mutex);
        return -1;
    }
    
    // 推入元素
    if (ringbuf_push(&pPosixQueue->ringbuf, pItem) != 0) {
        pthread_mutex_unlock(&pPosixQueue->mutex);
        return -1;
    }
    
    // 唤醒等待接收的线程
    pthread_cond_signal(&pPosixQueue->recv_cond);
    
    // 解锁
    pthread_mutex_unlock(&pPosixQueue->mutex);
    
    return 0;
}

/**
 * @brief 队列发送(中断模式)
 */
ssize_t os_queue_send_isr(OsQueue_t* pQueue, const void* pItem)
{
    if (pQueue == NULL || pItem == NULL) {
        return -1;
    }
    
    OsQueuePosix_t* pPosixQueue = (OsQueuePosix_t*)pQueue;
    
    // 加锁
    if (pthread_mutex_lock(&pPosixQueue->mutex) != 0) {
        return -1;
    }
    
    // 检查队列是否已满
    if (ringbuf_is_full(&pPosixQueue->ringbuf)) {
        pthread_mutex_unlock(&pPosixQueue->mutex);
        return -1;
    }
    
    // 推入元素
    if (ringbuf_push(&pPosixQueue->ringbuf, pItem) != 0) {
        pthread_mutex_unlock(&pPosixQueue->mutex);
        return -1;
    }
    
    // 唤醒等待接收的线程
    pthread_cond_signal(&pPosixQueue->recv_cond);
    
    // 解锁
    pthread_mutex_unlock(&pPosixQueue->mutex);
    
    return 0;
}

/**
 * @brief 队列接收
 */
ssize_t os_queue_receive(OsQueue_t* pQueue, void* pItem, size_t timeout_ms)
{
    if (pQueue == NULL || pItem == NULL) {
        return -1;
    }
    
    OsQueuePosix_t* pPosixQueue = (OsQueuePosix_t*)pQueue;
    struct timespec timeout;
    int timeout_result;
    
    // 计算超时时间
    timeout_result = calculate_timeout(&timeout, timeout_ms);
    if (timeout_result < 0) {
        return -1;
    }
    
    // 加锁
    if (pthread_mutex_lock(&pPosixQueue->mutex) != 0) {
        return -1;
    }
    
    // 等待队列有数据
    while (ringbuf_is_empty(&pPosixQueue->ringbuf) && !pPosixQueue->destroyed) {
        if (timeout_result == 0) {
            // 立即返回
            pthread_mutex_unlock(&pPosixQueue->mutex);
            return -1;
        } else if (timeout_result == 1 && timeout_ms != UINT32_MAX) {
            // 超时等待
            int result = pthread_cond_timedwait(&pPosixQueue->recv_cond, &pPosixQueue->mutex, &timeout);
            if (result == ETIMEDOUT) {
                pthread_mutex_unlock(&pPosixQueue->mutex);
                return -1;
            } else if (result != 0) {
                pthread_mutex_unlock(&pPosixQueue->mutex);
                return -1;
            }
        } else {
            // 无限等待
            if (pthread_cond_wait(&pPosixQueue->recv_cond, &pPosixQueue->mutex) != 0) {
                pthread_mutex_unlock(&pPosixQueue->mutex);
                return -1;
            }
        }
    }
    
    // 检查是否已销毁
    if (pPosixQueue->destroyed) {
        pthread_mutex_unlock(&pPosixQueue->mutex);
        return -1;
    }
    
    // 弹出元素
    if (ringbuf_pop(&pPosixQueue->ringbuf, pItem) != 0) {
        pthread_mutex_unlock(&pPosixQueue->mutex);
        return -1;
    }
    
    // 唤醒等待发送的线程
    pthread_cond_signal(&pPosixQueue->send_cond);
    
    // 解锁
    pthread_mutex_unlock(&pPosixQueue->mutex);
    
    return 0;
}

/**
 * @brief 获取队列当前元素数量
 */
ssize_t os_queue_get_count(OsQueue_t* pQueue)
{
    if (pQueue == NULL) {
        return -1;
    }
    
    OsQueuePosix_t* pPosixQueue = (OsQueuePosix_t*)pQueue;
    
    // 加锁
    if (pthread_mutex_lock(&pPosixQueue->mutex) != 0) {
        return -1;
    }
    
    ssize_t count = ringbuf_count(&pPosixQueue->ringbuf);
    
    // 解锁
    pthread_mutex_unlock(&pPosixQueue->mutex);
    
    return count;
}

/**
 * @brief 获取队列剩余空间
 */
ssize_t os_queue_get_space(OsQueue_t* pQueue)
{
    if (pQueue == NULL) {
        return -1;
    }
    
    OsQueuePosix_t* pPosixQueue = (OsQueuePosix_t*)pQueue;
    
    // 加锁
    if (pthread_mutex_lock(&pPosixQueue->mutex) != 0) {
        return -1;
    }
    
    ssize_t space = ringbuf_space(&pPosixQueue->ringbuf);
    
    // 解锁
    pthread_mutex_unlock(&pPosixQueue->mutex);
    
    return space;
}

/**
 * @brief 检查队列是否为空
 */
int os_queue_is_empty(OsQueue_t* pQueue)
{
    if (pQueue == NULL) {
        return -1;
    }
    
    OsQueuePosix_t* pPosixQueue = (OsQueuePosix_t*)pQueue;
    
    // 加锁
    if (pthread_mutex_lock(&pPosixQueue->mutex) != 0) {
        return -1;
    }
    
    int empty = ringbuf_is_empty(&pPosixQueue->ringbuf) ? 1 : 0;
    
    // 解锁
    pthread_mutex_unlock(&pPosixQueue->mutex);
    
    return empty;
}

/**
 * @brief 检查队列是否已满
 */
int os_queue_is_full(OsQueue_t* pQueue)
{
    if (pQueue == NULL) {
        return -1;
    }
    
    OsQueuePosix_t* pPosixQueue = (OsQueuePosix_t*)pQueue;
    
    // 加锁
    if (pthread_mutex_lock(&pPosixQueue->mutex) != 0) {
        return -1;
    }
    
    int full = ringbuf_is_full(&pPosixQueue->ringbuf) ? 1 : 0;
    
    // 解锁
    pthread_mutex_unlock(&pPosixQueue->mutex);
    
    return full;
}

/**
 * @brief 重置队列
 */
ssize_t os_queue_reset(OsQueue_t* pQueue)
{
    if (pQueue == NULL) {
        return -1;
    }
    
    OsQueuePosix_t* pPosixQueue = (OsQueuePosix_t*)pQueue;
    
    // 加锁
    if (pthread_mutex_lock(&pPosixQueue->mutex) != 0) {
        return -1;
    }
    
    // 清空环形缓冲区
    ringbuf_clear(&pPosixQueue->ringbuf);
    
    // 唤醒所有等待的线程
    pthread_cond_broadcast(&pPosixQueue->send_cond);
    pthread_cond_broadcast(&pPosixQueue->recv_cond);
    
    // 解锁
    pthread_mutex_unlock(&pPosixQueue->mutex);
    
    return 0;
}

#ifdef __cplusplus
}
#endif
