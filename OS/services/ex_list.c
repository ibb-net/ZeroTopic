/* LOG DEFINE */
#define LOG_TAG "LIST"
#define LOG_LVL LOG_LVL_INFO

/* RTOS */
#include "ex_list.h"

/* 初始化链表和BUFFER */
list_buffer_t xCreateList(size_t item_num, size_t pre_item_size) {
    configASSERT(item_num != 0);
    configASSERT(pre_item_size != 0);

    list_buffer_t list_handle = (list_buffer_t)pvPortMalloc(sizeof(list_buffer_struct));
    memset(list_handle, 0, sizeof(list_buffer_struct));
    if (list_handle == NULL) {
        return NULL;
    }
    size_t item_size = item_num * pre_item_size;

    list_handle->item_num      = item_num;
    list_handle->pre_item_size = pre_item_size;

    void *buffer = pvPortMalloc(item_size);
    if (buffer == NULL) {
        vPortFree(list_handle);
        TRACE_ERROR("xCreateList buffer malloc failed %zu", item_size);
        return NULL;
    }
    list_handle->buffer    = buffer;
    ListItem_t *item       = (ListItem_t *)pvPortMalloc(sizeof(ListItem_t) * item_num);
    list_handle->item_list = item;
    vListInitialise(&(list_handle->use_list));
    vListInitialise(&(list_handle->free_list));
    vListInitialise(&(list_handle->tmp_list)); /* 用于DMA缓存 */
    for (size_t i = 0; i < item_num; i++) {
        vListInitialiseItem(item + i);
        /* 将申请的buffer进行分段,并将地址保存在freelist中 */
        listSET_LIST_ITEM_VALUE(item + i, (uint32_t)buffer + i * pre_item_size);
        vListInsertEnd(&(list_handle->free_list), item + i);
    }
    return list_handle;
}
size_t vGetItemPerSize(list_buffer_t list_handle) {
    configASSERT(list_handle != NULL);
    return list_handle->pre_item_size;
}
size_t vGetItemMaxNum(list_buffer_t list_handle) {
    configASSERT(list_handle != NULL);
    return list_handle->item_num;
}
void vDeleteList(list_buffer_t list_handle) {
    configASSERT(list_handle != NULL);
    vPortFree(list_handle->buffer);
    vPortFree(list_handle);
}

void *vInsertList(list_buffer_t list_handle, void *data) {
    configASSERT(data != NULL);
    configASSERT(list_handle != NULL);

    if (!listLIST_IS_EMPTY(&list_handle->free_list)) {
        ListItem_t *item = listGET_HEAD_ENTRY(&list_handle->free_list);
        memcpy((void *)listGET_LIST_ITEM_VALUE(item), data, list_handle->pre_item_size);
        uxListRemove(item);
        vListInsertEnd(&list_handle->use_list, item);
        list_handle->counter++;
        return (void *)listGET_LIST_ITEM_VALUE(item);
    } else {
        TRACE_ERROR("vInsertList size full");
    }
    return NULL;
}

void *vGetItem(list_buffer_t list_handle, size_t index) {
    configASSERT(list_handle != NULL);
    if (listLIST_IS_EMPTY(&list_handle->use_list)) {
        TRACE_ERROR("vGetLastItem size empty");
        return NULL;
    } else {
        ListItem_t *item = listGET_HEAD_ENTRY(&list_handle->use_list);
        if (index >= listCURRENT_LIST_LENGTH(&list_handle->use_list)) {
            TRACE_ERROR("vGetItem index error");
            return NULL;
        }
        for (int i = 0; i < index; i++) {
            item = listGET_NEXT(item);
        }
        return (void *)listGET_LIST_ITEM_VALUE(item);
    }
}

void *vGetLastItem(list_buffer_t list_handle) {
    configASSERT(list_handle != NULL);
    if (listLIST_IS_EMPTY(&list_handle->use_list)) {
        TRACE_ERROR("vGetLastItem size empty");
        return NULL;
    } else {
        ListItem_t *item = listGET_HEAD_ENTRY(&list_handle->use_list);
        for (int i = 0; i < listCURRENT_LIST_LENGTH(&list_handle->use_list) - 1; i++) {
            item = listGET_NEXT(item);
        }
        return (void *)listGET_LIST_ITEM_VALUE(item);
    }
}

size_t vGetListNum(list_buffer_t list_handle) {
    configASSERT(list_handle != NULL);
    return list_handle->counter;
}
// vRemoveItem
size_t vRemoveItem(list_buffer_t list_handle, size_t index) {
    configASSERT(list_handle != NULL);
    if (listLIST_IS_EMPTY(&list_handle->use_list)) {
        TRACE_ERROR("vRemoveItem size empty");
    } else {
        ListItem_t *item = listGET_HEAD_ENTRY(&list_handle->use_list);
        if (index >= listCURRENT_LIST_LENGTH(&list_handle->use_list)) {
            TRACE_ERROR("vRemoveItem index error");
            return list_handle->counter;
        }
        for (int i = 0; i < index; i++) {
            item = listGET_NEXT(item);
        }
        uxListRemove(item);
        vListInsertEnd(&list_handle->free_list, item);
        list_handle->counter--;
    }
    return list_handle->counter;
}

size_t vDeleteLastItem(list_buffer_t list_handle) {
    configASSERT(list_handle != NULL);
    if (listLIST_IS_EMPTY(&list_handle->use_list)) {
        TRACE_ERROR("vDeleteLastItem size empty");
    } else {
        ListItem_t *item = listGET_HEAD_ENTRY(&list_handle->use_list);
        for (int i = 0; i < listCURRENT_LIST_LENGTH(&list_handle->use_list) - 1; i++) {
            item = listGET_NEXT(item);
        }
        uxListRemove(item);
        vListInsertEnd(&list_handle->free_list, item);
        list_handle->counter--;
    }
    return list_handle->counter;
}

