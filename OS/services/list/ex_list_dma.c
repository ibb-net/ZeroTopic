/* LOG DEFINE */
#define LOG_TAG "LIST"
#define LOG_LVL LOG_LVL_INFO

/* RTOS */
#include "ex_list_dma.h"

/* 初始化链表和BUFFER */
list_dma_buffer_t xCreateListDMA(size_t item_num, size_t pre_item_size) {
    configASSERT(item_num != 0);
    configASSERT(pre_item_size != 0);

    list_dma_buffer_t list_handle = (list_dma_buffer_t)pvPortMalloc(sizeof(list_dma_buffer_struct));
    memset(list_handle, 0, sizeof(list_dma_buffer_struct));
    if (list_handle == NULL) {
        return NULL;
    }
    size_t item_size = item_num * pre_item_size;
    list_handle->item_num      = item_num;
    list_handle->pre_item_size = pre_item_size;
    list_handle->lock          = xSemaphoreCreateBinary();
    if (list_handle->lock == NULL) {
        TRACE_ERROR("create mutex failed!\r\n");
        return NULL;
    }
    vListInitialise(&(list_handle->use_list));
    vListInitialise(&(list_handle->free_list));
    vListInitialise(&(list_handle->tmp_list)); /* 用于DMA缓存 */
    for (size_t i = 0; i < item_num; i++) {
        ListItem_t *item = (ListItem_t *)pvPortMalloc(sizeof(ListItem_t));
        vListInitialiseItem(item);
        /* 将申请的buffer进行分段,并将地址保存在freelist中 */
        void *buffer = pvPortMalloc(item_size);
        if (buffer == NULL) {
            vPortFree(list_handle);
            TRACE_ERROR("xCreateList buffer %d malloc failed %zu", i, item_size);
            return NULL;
        }
        listSET_LIST_ITEM_VALUE(item, (uint32_t)buffer);
        vListInsertEnd(&(list_handle->free_list), item);
    }
    xSemaphoreGive(list_handle->lock);
    return list_handle;
}

BaseType_t vReqestListDMABuffer(list_dma_buffer_t list_handle, steam_info_t item_steam_info) {
    // printf("request dma \r\n");
    BaseType_t ref = pdFALSE;
    configASSERT(list_handle != NULL);
    memset(item_steam_info, 0, sizeof(steam_info_struct));
    if (pdFALSE == xSemaphoreTake(list_handle->lock, (TickType_t)0)) {
        return pdFALSE;
    }
    if (listLIST_IS_EMPTY(&list_handle->free_list)) {
        TRACE_ERROR("vReqestListDMABuffer size empty\r\n");
        xSemaphoreGive(list_handle->lock);
        return pdFALSE;
    }

    ListItem_t *item = listGET_HEAD_ENTRY(&list_handle->free_list);
    uxListRemove(item);
    vListInsertEnd(&list_handle->tmp_list, item);
    item_steam_info->item   = item;
    item_steam_info->size   = 0;
    item_steam_info->buffer = (ptrBuffer *)listGET_LIST_ITEM_VALUE(item);

    ref = pdTRUE;
    return ref;
}
BaseType_t vReqestListDMABufferFromISR(list_dma_buffer_t list_handle, steam_info_t item_steam_info) {
    BaseType_t ref = pdFALSE;
    // printf("request dma isr\r\n");
		
    configASSERT(list_handle != NULL);
    memset(item_steam_info, 0, sizeof(steam_info_struct));
    if (pdFALSE == xSemaphoreTakeFromISR(list_handle->lock, NULL)) {
				TRACE_ERROR("Request ISR DMA buffer error\r\n");
        return pdFALSE;
    }
    if (listLIST_IS_EMPTY(&list_handle->free_list)) {
        TRACE_ERROR("vReqestListDMABufferFromISR size empty\r\n");
        xSemaphoreGiveFromISR(list_handle->lock, NULL);
        return pdFALSE;
    }

    ListItem_t *item = listGET_HEAD_ENTRY(&list_handle->free_list);
    uxListRemove(item);
    vListInsertEnd(&list_handle->tmp_list, item);
    item_steam_info->item   = item;
    item_steam_info->size   = 0;
    item_steam_info->buffer = (ptrBuffer *)listGET_LIST_ITEM_VALUE(item);

    ref = pdTRUE;
    return ref;
}
// 需要传入item_info size
// 返回item 和buffer
BaseType_t __sActive(list_dma_buffer_t list_handle, steam_info_t item_steam_info) {
    // printf("\r\n >>>>>\r\nActive\r\n");
    configASSERT(list_handle != NULL);
    item_steam_info->buffer = NULL;
    item_steam_info->item   = NULL;

    if (listLIST_IS_EMPTY(&list_handle->tmp_list)) {
        TRACE_ERROR("vActiveListDMABuffer size empty");
        return pdFALSE;
    }
    ListItem_t *item = listGET_HEAD_ENTRY(&list_handle->tmp_list);
    uxListRemove(item);
    vListInsertEnd(&list_handle->use_list, item);
    // printf("insert uselist item %p\r\n", item);
    item_steam_info->item   = item;
    item_steam_info->buffer = (ptrBuffer *)listGET_LIST_ITEM_VALUE(item);
    return pdTRUE;
}
BaseType_t vActiveListDMABuffer(list_dma_buffer_t list_handle, steam_info_t item_steam_info) {
    BaseType_t ref = pdFALSE;
    ref            = __sActive(list_handle, item_steam_info);
	
    xSemaphoreGive(list_handle->lock);
    // printf("active done\r\n");
    // printf("use_list num %lu\r\n", listCURRENT_LIST_LENGTH(&list_handle->use_list));

    return ref;
}

BaseType_t vActiveListDMABufferFromISR(list_dma_buffer_t list_handle, steam_info_t item_steam_info) {
    BaseType_t ref = pdFALSE;
    ref            = __sActive(list_handle, item_steam_info);

    xSemaphoreGiveFromISR(list_handle->lock, NULL);
    // printf("active isr done\r\n");
    // printf("use_list num %lu\r\n", listCURRENT_LIST_LENGTH(&list_handle->use_list));

    return ref;
}
BaseType_t vFreeListDMABuffer(list_dma_buffer_t list_handle, steam_info_t item_steam_info) {
    configASSERT(list_handle != NULL);
    if (item_steam_info == NULL) {
        TRACE_ERROR("vFreeListDMABuffer item_steam_info is NULL");
        return pdFALSE;
    }
    if (item_steam_info->item == NULL) {
        TRACE_ERROR("vFreeListDMABuffer item is NULL\r\n");
        return pdFALSE;
    }
    // printf("use_list num %lu\r\n", listCURRENT_LIST_LENGTH(&list_handle->use_list));
    if (listLIST_IS_EMPTY(&list_handle->use_list)) {
        TRACE_ERROR("vFreeListDMABuffer size empty\r\n");
        return pdFALSE;
    }
    uxListRemove(item_steam_info->item);
    vListInsertEnd(&list_handle->free_list, item_steam_info->item);
    // printf("free uselist item %p\r\n", item_steam_info->item);
    return pdTRUE;
}