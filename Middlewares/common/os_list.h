/***************************************************************************
 *
 * Copyright (c) 2024 OpenIBBOs. All Rights Reserved
 *
 **************************************************************************/

#ifndef _OS_LIST_H_
#define _OS_LIST_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 链表项结构体
 * 
 * 参考FreeRTOS的ListItem_t设计，提供双向链表节点
 */
typedef struct OsListItem {
    struct OsListItem* pxNext;     // 指向下一个节点
    struct OsListItem* pxPrevious; // 指向上一个节点
    uintptr_t xItemValue;          // 存储的值(可以是指针或数值)
} OsListItem_t;

/**
 * @brief 链表结构体
 * 
 * 参考FreeRTOS的List_t设计，提供双向链表容器
 */
typedef struct OsList {
    OsListItem_t* pxIndex;         // 当前遍历索引
    OsListItem_t xListEnd;         // 链表尾标记(哨兵节点)
    size_t uxNumberOfItems;        // 链表项数量
} OsList_t;

/* 链表操作宏定义 - 参考FreeRTOS风格 */

/**
 * @brief 获取链表头节点
 */
#define os_list_get_head_entry(pxList) ((OsListItem_t*)((pxList)->xListEnd.pxNext))

/**
 * @brief 获取链表尾标记
 */
#define os_list_get_end_marker(pxList) ((OsListItem_t*)(&(pxList)->xListEnd))

/**
 * @brief 获取下一个节点
 */
#define os_list_get_next(pxListItem) ((OsListItem_t*)((pxListItem)->pxNext))

/**
 * @brief 获取上一个节点
 */
#define os_list_get_previous(pxListItem) ((OsListItem_t*)((pxListItem)->pxPrevious))

/**
 * @brief 获取链表长度
 */
#define os_list_get_length(pxList) ((pxList)->uxNumberOfItems)

/**
 * @brief 获取链表项的值
 */
#define os_list_item_get_value(pxListItem) ((pxListItem)->xItemValue)

/**
 * @brief 设置链表项的值
 */
#define os_list_item_set_value(pxListItem, xValue) ((pxListItem)->xItemValue = (xValue))

/**
 * @brief 检查链表是否为空
 */
#define os_list_is_empty(pxList) ((pxList)->uxNumberOfItems == 0)

/* 函数声明 */

/**
 * @brief 初始化链表
 * 
 * @param pxList 要初始化的链表指针
 */
void os_list_initialise(OsList_t* pxList);

/**
 * @brief 初始化链表项
 * 
 * @param pxItem 要初始化的链表项指针
 */
void os_list_item_initialise(OsListItem_t* pxItem);

/**
 * @brief 在链表尾部插入项
 * 
 * @param pxList 目标链表
 * @param pxNewListItem 要插入的链表项
 */
void os_list_insert_end(OsList_t* pxList, OsListItem_t* pxNewListItem);

/**
 * @brief 在链表头部插入项
 * 
 * @param pxList 目标链表
 * @param pxNewListItem 要插入的链表项
 */
void os_list_insert_head(OsList_t* pxList, OsListItem_t* pxNewListItem);

/**
 * @brief 从链表中移除项
 * 
 * @param pxList 目标链表
 * @param pxItemToRemove 要移除的链表项
 * @return size_t 移除的项数量(0表示未找到,1表示成功移除)
 */
size_t os_list_remove(OsList_t* pxList, OsListItem_t* pxItemToRemove);

/**
 * @brief 在指定项后插入新项
 * 
 * @param pxList 目标链表
 * @param pxListItemAfter 参考项(在其后插入)
 * @param pxNewListItem 要插入的新项
 */
void os_list_insert_after(OsList_t* pxList, OsListItem_t* pxListItemAfter, OsListItem_t* pxNewListItem);

/**
 * @brief 在指定项前插入新项
 * 
 * @param pxList 目标链表
 * @param pxListItemBefore 参考项(在其前插入)
 * @param pxNewListItem 要插入的新项
 */
void os_list_insert_before(OsList_t* pxList, OsListItem_t* pxListItemBefore, OsListItem_t* pxNewListItem);

/**
 * @brief 查找包含指定值的链表项
 * 
 * @param pxList 目标链表
 * @param xValue 要查找的值
 * @return OsListItem_t* 找到的链表项指针，未找到返回NULL
 */
OsListItem_t* os_list_find_value(OsList_t* pxList, uintptr_t xValue);

/**
 * @brief 清空链表(移除所有项，但保留链表结构)
 * 
 * @param pxList 目标链表
 */
void os_list_clear(OsList_t* pxList);

#ifdef __cplusplus
}
#endif

#endif /* _OS_LIST_H_ */
