/***************************************************************************
 *
 * Copyright (c) 2024 OpenIBBOs. All Rights Reserved
 *
 **************************************************************************/

#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief 初始化环形缓冲区
 */
int ringbuf_init(ringbuf_t* pxRingBuf, uint8_t* pBuffer, size_t uxLength, size_t uxItemSize)
{
    if (pxRingBuf == NULL || pBuffer == NULL || uxItemSize == 0) {
        return -1;
    }
    
    // 计算最大元素数量
    size_t uxMaxItems = uxLength / uxItemSize;
    if (uxMaxItems == 0) {
        return -1;
    }
    
    pxRingBuf->pBuffer = pBuffer;
    pxRingBuf->uxLength = uxLength;
    pxRingBuf->uxItemSize = uxItemSize;
    pxRingBuf->uxMaxItems = uxMaxItems;
    pxRingBuf->uxHead = 0;
    pxRingBuf->uxTail = 0;
    pxRingBuf->uxCount = 0;
    
    return 0;
}

/**
 * @brief 初始化环形缓冲区(内部分配内存)
 */
int ringbuf_init_malloc(ringbuf_t* pxRingBuf, size_t uxMaxItems, size_t uxItemSize)
{
    if (pxRingBuf == NULL || uxMaxItems == 0 || uxItemSize == 0) {
        return -1;
    }
    
    // 分配缓冲区内存
    size_t uxLength = uxMaxItems * uxItemSize;
    uint8_t* pBuffer = (uint8_t*)malloc(uxLength);
    if (pBuffer == NULL) {
        return -1;
    }
    
    return ringbuf_init(pxRingBuf, pBuffer, uxLength, uxItemSize);
}

/**
 * @brief 销毁环形缓冲区
 */
void ringbuf_destroy(ringbuf_t* pxRingBuf, bool bFreeBuffer)
{
    if (pxRingBuf == NULL) {
        return;
    }
    
    if (bFreeBuffer && pxRingBuf->pBuffer != NULL) {
        free(pxRingBuf->pBuffer);
    }
    
    // 清零结构体
    memset(pxRingBuf, 0, sizeof(ringbuf_t));
}

/**
 * @brief 向环形缓冲区推入元素
 */
int ringbuf_push(ringbuf_t* pxRingBuf, const void* pItem)
{
    if (pxRingBuf == NULL || pItem == NULL) {
        return -1;
    }
    
    // 检查是否已满
    if (ringbuf_is_full(pxRingBuf)) {
        return -1;
    }
    
    // 计算写入位置
    size_t uxWritePos = pxRingBuf->uxHead * pxRingBuf->uxItemSize;
    
    // 复制数据
    memcpy(&pxRingBuf->pBuffer[uxWritePos], pItem, pxRingBuf->uxItemSize);
    
    // 更新头指针和计数
    pxRingBuf->uxHead = (pxRingBuf->uxHead + 1) % pxRingBuf->uxMaxItems;
    pxRingBuf->uxCount++;
    
    return 0;
}

/**
 * @brief 从环形缓冲区弹出元素
 */
int ringbuf_pop(ringbuf_t* pxRingBuf, void* pItem)
{
    if (pxRingBuf == NULL || pItem == NULL) {
        return -1;
    }
    
    // 检查是否为空
    if (ringbuf_is_empty(pxRingBuf)) {
        return -1;
    }
    
    // 计算读取位置
    size_t uxReadPos = pxRingBuf->uxTail * pxRingBuf->uxItemSize;
    
    // 复制数据
    memcpy(pItem, &pxRingBuf->pBuffer[uxReadPos], pxRingBuf->uxItemSize);
    
    // 更新尾指针和计数
    pxRingBuf->uxTail = (pxRingBuf->uxTail + 1) % pxRingBuf->uxMaxItems;
    pxRingBuf->uxCount--;
    
    return 0;
}

/**
 * @brief 查看环形缓冲区头部元素(不移除)
 */
int ringbuf_peek(ringbuf_t* pxRingBuf, void* pItem)
{
    if (pxRingBuf == NULL || pItem == NULL) {
        return -1;
    }
    
    // 检查是否为空
    if (ringbuf_is_empty(pxRingBuf)) {
        return -1;
    }
    
    // 计算读取位置
    size_t uxReadPos = pxRingBuf->uxTail * pxRingBuf->uxItemSize;
    
    // 复制数据
    memcpy(pItem, &pxRingBuf->pBuffer[uxReadPos], pxRingBuf->uxItemSize);
    
    return 0;
}

/**
 * @brief 清空环形缓冲区
 */
void ringbuf_clear(ringbuf_t* pxRingBuf)
{
    if (pxRingBuf == NULL) {
        return;
    }
    
    pxRingBuf->uxHead = 0;
    pxRingBuf->uxTail = 0;
    pxRingBuf->uxCount = 0;
}

/**
 * @brief 获取环形缓冲区使用率
 */
float ringbuf_usage(ringbuf_t* pxRingBuf)
{
    if (pxRingBuf == NULL || pxRingBuf->uxMaxItems == 0) {
        return 0.0f;
    }
    
    return (float)pxRingBuf->uxCount / (float)pxRingBuf->uxMaxItems;
}
