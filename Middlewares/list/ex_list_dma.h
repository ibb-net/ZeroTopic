#ifndef __LIB_LIST_H__
#define __LIB_LIST_H__

#include <stdio.h>
#include <stdlib.h>

#include "string.h"
/* RTOS */
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#ifndef TRACE_ERROR
#define TRACE_ERROR printf
#endif
typedef struct {
    size_t item_num;
    size_t pre_item_size;
    SemaphoreHandle_t lock;
    size_t counter;
    List_t free_list;
    List_t tmp_list;
    List_t use_list;
    ListItem_t *item_list;//NOT used

} list_dma_buffer_struct;
typedef list_dma_buffer_struct *list_dma_buffer_t;
typedef void ptrBuffer;
typedef struct
{
    ListItem_t *item;
    size_t size;
    ptrBuffer *buffer;
} steam_info_struct;
typedef steam_info_struct *steam_info_t;

list_dma_buffer_t xCreateListDMA(size_t item_num, size_t pre_item_size);
BaseType_t vReqestListDMABuffer(list_dma_buffer_t list_handle, steam_info_t item_steam_info);
BaseType_t vReqestListDMABufferFromISR(list_dma_buffer_t list_handle, steam_info_t item_steam_info);
BaseType_t vActiveListDMABuffer(list_dma_buffer_t list_handle, steam_info_t item_steam_info);
BaseType_t vActiveListDMABufferFromISR(list_dma_buffer_t list_handle, steam_info_t item_steam_info);
BaseType_t vFreeListDMABuffer(list_dma_buffer_t list_handle, steam_info_t item_steam_info);

#endif /* __LIB_LIST_H__ */
