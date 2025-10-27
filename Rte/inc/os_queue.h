/***************************************************************************
 *
 * Copyright (c) 2024 OpenIBBOs. All Rights Reserved
 *
 **************************************************************************/

#ifndef _OS_QUEUE_H_
#define _OS_QUEUE_H_

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 队列句柄类型
 * 
 * 各平台实现内部结构体，对外统一使用void*类型
 */
typedef void OsQueue_t;

/**
 * @brief 队列创建
 * 
 * @param queue_length 队列最大长度(元素数量)
 * @param item_size 单个元素大小(字节)
 * @param pName 队列名称(用于调试和命名)
 * @return OsQueue_t* 队列句柄，失败返回NULL
 */
OsQueue_t* os_queue_create(size_t queue_length, size_t item_size, const char* pName);

/**
 * @brief 队列销毁
 * 
 * @param pQueue 队列句柄
 * @return ssize_t 0成功，-1失败
 */
ssize_t os_queue_destroy(OsQueue_t* pQueue);

/**
 * @brief 队列发送(任务模式)
 * 
 * @param pQueue 队列句柄
 * @param pItem 要发送的元素指针
 * @param timeout_ms 超时时间(毫秒)，0表示立即返回，UINT32_MAX表示无限等待
 * @return ssize_t 0成功，-1失败(超时或队列满)
 */
ssize_t os_queue_send(OsQueue_t* pQueue, const void* pItem, size_t timeout_ms);

/**
 * @brief 队列发送(中断模式)
 * 
 * @param pQueue 队列句柄
 * @param pItem 要发送的元素指针
 * @return ssize_t 0成功，-1失败(队列满)
 */
ssize_t os_queue_send_isr(OsQueue_t* pQueue, const void* pItem);

/**
 * @brief 队列接收
 * 
 * @param pQueue 队列句柄
 * @param pItem 接收元素的缓冲区指针
 * @param timeout_ms 超时时间(毫秒)，0表示立即返回，UINT32_MAX表示无限等待
 * @return ssize_t 0成功，-1失败(超时或队列空)
 */
ssize_t os_queue_receive(OsQueue_t* pQueue, void* pItem, size_t timeout_ms);

/**
 * @brief 获取队列当前元素数量
 * 
 * @param pQueue 队列句柄
 * @return ssize_t 元素数量，-1失败
 */
ssize_t os_queue_get_count(OsQueue_t* pQueue);

/**
 * @brief 获取队列剩余空间(元素数量)
 * 
 * @param pQueue 队列句柄
 * @return ssize_t 剩余空间，-1失败
 */
ssize_t os_queue_get_space(OsQueue_t* pQueue);

/**
 * @brief 检查队列是否为空
 * 
 * @param pQueue 队列句柄
 * @return int 1为空，0非空，-1失败
 */
int os_queue_is_empty(OsQueue_t* pQueue);

/**
 * @brief 检查队列是否已满
 * 
 * @param pQueue 队列句柄
 * @return int 1已满，0未满，-1失败
 */
int os_queue_is_full(OsQueue_t* pQueue);

/**
 * @brief 重置队列(清空所有元素)
 * 
 * @param pQueue 队列句柄
 * @return ssize_t 0成功，-1失败
 */
ssize_t os_queue_reset(OsQueue_t* pQueue);

#ifdef __cplusplus
}
#endif

#endif /* _OS_QUEUE_H_ */
