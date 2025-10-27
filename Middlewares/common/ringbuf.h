/***************************************************************************
 *
 * Copyright (c) 2024 OpenIBBOs. All Rights Reserved
 *
 **************************************************************************/

#ifndef _RINGBUF_H_
#define _RINGBUF_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 环形缓冲区结构体
 * 
 * 提供线程不安全的环形队列实现，由上层os_queue负责同步
 */
typedef struct {
    uint8_t* pBuffer;      // 数据缓冲区指针
    size_t uxLength;       // 缓冲区总长度(字节)
    size_t uxItemSize;     // 单个元素大小(字节)
    size_t uxMaxItems;     // 最大元素数量
    size_t uxHead;         // 头指针(下一个写入位置)
    size_t uxTail;         // 尾指针(下一个读取位置)
    size_t uxCount;        // 当前元素数量
} ringbuf_t;

/* 环形缓冲区操作宏定义 */

/**
 * @brief 检查环形缓冲区是否为空
 */
#define ringbuf_is_empty(pxRingBuf) ((pxRingBuf)->uxCount == 0)

/**
 * @brief 检查环形缓冲区是否已满
 */
#define ringbuf_is_full(pxRingBuf) ((pxRingBuf)->uxCount >= (pxRingBuf)->uxMaxItems)

/**
 * @brief 获取环形缓冲区当前元素数量
 */
#define ringbuf_count(pxRingBuf) ((pxRingBuf)->uxCount)

/**
 * @brief 获取环形缓冲区剩余空间(元素数量)
 */
#define ringbuf_space(pxRingBuf) ((pxRingBuf)->uxMaxItems - (pxRingBuf)->uxCount)

/**
 * @brief 获取环形缓冲区容量(最大元素数量)
 */
#define ringbuf_capacity(pxRingBuf) ((pxRingBuf)->uxMaxItems)

/* 函数声明 */

/**
 * @brief 初始化环形缓冲区
 * 
 * @param pxRingBuf 环形缓冲区指针
 * @param pBuffer 数据缓冲区指针(用户提供)
 * @param uxLength 缓冲区总长度(字节)
 * @param uxItemSize 单个元素大小(字节)
 * @return int 0成功，-1失败
 */
int ringbuf_init(ringbuf_t* pxRingBuf, uint8_t* pBuffer, size_t uxLength, size_t uxItemSize);

/**
 * @brief 初始化环形缓冲区(内部分配内存)
 * 
 * @param pxRingBuf 环形缓冲区指针
 * @param uxMaxItems 最大元素数量
 * @param uxItemSize 单个元素大小(字节)
 * @return int 0成功，-1失败
 */
int ringbuf_init_malloc(ringbuf_t* pxRingBuf, size_t uxMaxItems, size_t uxItemSize);

/**
 * @brief 销毁环形缓冲区
 * 
 * @param pxRingBuf 环形缓冲区指针
 * @param bFreeBuffer 是否释放内部分配的缓冲区
 */
void ringbuf_destroy(ringbuf_t* pxRingBuf, bool bFreeBuffer);

/**
 * @brief 向环形缓冲区推入元素
 * 
 * @param pxRingBuf 环形缓冲区指针
 * @param pItem 要推入的元素指针
 * @return int 0成功，-1失败(缓冲区满)
 */
int ringbuf_push(ringbuf_t* pxRingBuf, const void* pItem);

/**
 * @brief 从环形缓冲区弹出元素
 * 
 * @param pxRingBuf 环形缓冲区指针
 * @param pItem 接收元素的缓冲区指针
 * @return int 0成功，-1失败(缓冲区空)
 */
int ringbuf_pop(ringbuf_t* pxRingBuf, void* pItem);

/**
 * @brief 查看环形缓冲区头部元素(不移除)
 * 
 * @param pxRingBuf 环形缓冲区指针
 * @param pItem 接收元素的缓冲区指针
 * @return int 0成功，-1失败(缓冲区空)
 */
int ringbuf_peek(ringbuf_t* pxRingBuf, void* pItem);

/**
 * @brief 清空环形缓冲区
 * 
 * @param pxRingBuf 环形缓冲区指针
 */
void ringbuf_clear(ringbuf_t* pxRingBuf);

/**
 * @brief 获取环形缓冲区使用率(0.0-1.0)
 * 
 * @param pxRingBuf 环形缓冲区指针
 * @return float 使用率
 */
float ringbuf_usage(ringbuf_t* pxRingBuf);

#ifdef __cplusplus
}
#endif

#endif /* _RINGBUF_H_ */
