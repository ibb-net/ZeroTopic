/***************************************************************************
 *
 * Copyright (c) 2024 OpenIBBOs. All Rights Reserved
 *
 **************************************************************************/

#include "os_list.h"
#include <string.h>

/**
 * @brief 初始化链表
 * 
 * 参考FreeRTOS的vListInitialise实现
 */
void os_list_initialise(OsList_t* pxList)
{
    if (pxList == NULL) {
        return;
    }
    
    // 设置尾标记项
    pxList->xListEnd.pxNext = &(pxList->xListEnd);
    pxList->xListEnd.pxPrevious = &(pxList->xListEnd);
    pxList->xListEnd.xItemValue = 0;
    
    // 初始化索引和计数
    pxList->pxIndex = &(pxList->xListEnd);
    pxList->uxNumberOfItems = 0;
}

/**
 * @brief 初始化链表项
 * 
 * 参考FreeRTOS的vListInitialiseItem实现
 */
void os_list_item_initialise(OsListItem_t* pxItem)
{
    if (pxItem == NULL) {
        return;
    }
    
    pxItem->pxNext = NULL;
    pxItem->pxPrevious = NULL;
    pxItem->xItemValue = 0;
}

/**
 * @brief 在链表尾部插入项
 * 
 * 参考FreeRTOS的vListInsertEnd实现
 */
void os_list_insert_end(OsList_t* pxList, OsListItem_t* pxNewListItem)
{
    if (pxList == NULL || pxNewListItem == NULL) {
        return;
    }
    
    // 设置新项的指针
    pxNewListItem->pxNext = &(pxList->xListEnd);
    pxNewListItem->pxPrevious = pxList->xListEnd.pxPrevious;
    
    // 更新相邻项的指针
    pxList->xListEnd.pxPrevious->pxNext = pxNewListItem;
    pxList->xListEnd.pxPrevious = pxNewListItem;
    
    // 增加计数
    pxList->uxNumberOfItems++;
}

/**
 * @brief 在链表头部插入项
 * 
 * 参考FreeRTOS的vListInsertHead实现
 */
void os_list_insert_head(OsList_t* pxList, OsListItem_t* pxNewListItem)
{
    if (pxList == NULL || pxNewListItem == NULL) {
        return;
    }
    
    // 设置新项的指针
    pxNewListItem->pxNext = pxList->xListEnd.pxNext;
    pxNewListItem->pxPrevious = &(pxList->xListEnd);
    
    // 更新相邻项的指针
    pxList->xListEnd.pxNext->pxPrevious = pxNewListItem;
    pxList->xListEnd.pxNext = pxNewListItem;
    
    // 增加计数
    pxList->uxNumberOfItems++;
}

/**
 * @brief 从链表中移除项
 * 
 * 参考FreeRTOS的uxListRemove实现
 */
size_t os_list_remove(OsList_t* pxList, OsListItem_t* pxItemToRemove)
{
    if (pxList == NULL || pxItemToRemove == NULL) {
        return 0;
    }
    
    // 检查项是否在链表中
    OsListItem_t* pxItem = pxList->xListEnd.pxNext;
    while (pxItem != &(pxList->xListEnd)) {
        if (pxItem == pxItemToRemove) {
            // 找到要移除的项，更新相邻项的指针
            pxItem->pxPrevious->pxNext = pxItem->pxNext;
            pxItem->pxNext->pxPrevious = pxItem->pxPrevious;
            
            // 如果当前索引指向被移除的项，移动到下一个
            if (pxList->pxIndex == pxItem) {
                pxList->pxIndex = pxItem->pxNext;
            }
            
            // 减少计数
            pxList->uxNumberOfItems--;
            
            // 清理被移除项的指针
            pxItem->pxNext = NULL;
            pxItem->pxPrevious = NULL;
            
            return 1;
        }
        pxItem = pxItem->pxNext;
    }
    
    return 0; // 未找到要移除的项
}

/**
 * @brief 在指定项后插入新项
 */
void os_list_insert_after(OsList_t* pxList, OsListItem_t* pxListItemAfter, OsListItem_t* pxNewListItem)
{
    if (pxList == NULL || pxNewListItem == NULL || pxListItemAfter == NULL) {
        return;
    }
    
    // 设置新项的指针
    pxNewListItem->pxNext = pxListItemAfter->pxNext;
    pxNewListItem->pxPrevious = pxListItemAfter;
    
    // 更新相邻项的指针
    pxListItemAfter->pxNext->pxPrevious = pxNewListItem;
    pxListItemAfter->pxNext = pxNewListItem;
    
    // 增加计数
    pxList->uxNumberOfItems++;
}

/**
 * @brief 在指定项前插入新项
 */
void os_list_insert_before(OsList_t* pxList, OsListItem_t* pxListItemBefore, OsListItem_t* pxNewListItem)
{
    if (pxList == NULL || pxNewListItem == NULL || pxListItemBefore == NULL) {
        return;
    }
    
    // 设置新项的指针
    pxNewListItem->pxNext = pxListItemBefore;
    pxNewListItem->pxPrevious = pxListItemBefore->pxPrevious;
    
    // 更新相邻项的指针
    pxListItemBefore->pxPrevious->pxNext = pxNewListItem;
    pxListItemBefore->pxPrevious = pxNewListItem;
    
    // 增加计数
    pxList->uxNumberOfItems++;
}

/**
 * @brief 查找包含指定值的链表项
 */
OsListItem_t* os_list_find_value(OsList_t* pxList, uintptr_t xValue)
{
    if (pxList == NULL) {
        return NULL;
    }
    
    OsListItem_t* pxItem = pxList->xListEnd.pxNext;
    while (pxItem != &(pxList->xListEnd)) {
        if (pxItem->xItemValue == xValue) {
            return pxItem;
        }
        pxItem = pxItem->pxNext;
    }
    
    return NULL; // 未找到
}

/**
 * @brief 清空链表
 */
void os_list_clear(OsList_t* pxList)
{
    if (pxList == NULL) {
        return;
    }
    
    // 重置尾标记项
    pxList->xListEnd.pxNext = &(pxList->xListEnd);
    pxList->xListEnd.pxPrevious = &(pxList->xListEnd);
    
    // 重置索引和计数
    pxList->pxIndex = &(pxList->xListEnd);
    pxList->uxNumberOfItems = 0;
}
