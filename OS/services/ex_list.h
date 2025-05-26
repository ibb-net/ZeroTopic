#ifndef __LIB_LIST_H__
#define __LIB_LIST_H__

#include <stdio.h>
#include <stdlib.h>
#include "string.h"
 /* RTOS */
#include "FreeRTOS.h"
#include "queue.h"

#ifndef TRACE_ERROR	
#define TRACE_ERROR printf
#endif
 typedef struct {
    size_t item_num;
    size_t pre_item_size;
    size_t counter;
    List_t free_list;
    List_t tmp_list;
    List_t use_list;
    ListItem_t * item_list;
    void *buffer;

} list_buffer_struct;
typedef list_buffer_struct *list_buffer_t;

list_buffer_t xCreateList(size_t item_num, size_t pre_item_size);
void vDeleteList(list_buffer_t list_handle);
void * vInsertList(list_buffer_t list_handle, void *data) ;
void * vGetItem(list_buffer_t list_handle,size_t index) ;
void * vGetLastItem(list_buffer_t list_handle) ;
size_t vGetItemPerSize(list_buffer_t list_handle);
size_t vGetItemMaxNum(list_buffer_t list_handle);
size_t vDeleteLastItem(list_buffer_t list_handle) ;
size_t vRemoveItem(list_buffer_t list_handle, size_t index) ;
size_t vGetListNum(list_buffer_t list_handle);


#endif /* __LIB_LIST_H__ */

