/* LOG DEFINE */
// #define LOG_TAG      "VFB"
// #define LOG_LVL      LOG_LVL_INFO
// #include "ulog.h"

#include "vfb/vfb.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "vfb/vfb_config.h"

static vfb_info_struct __vfb_info;

static List_t *__vfb_list_get_head(uint16_t event);
static int __vfb_list_add_queue(List_t *queue_list, QueueHandle_t queue_handle);

/**
 * @brief Get the head of the event list
 *
 * @param event The event to get the head of
 * @return List_t* The head of the event list, or NULL if not found
 */
static List_t *__vfb_list_get_head(uint16_t event) {
    ListItem_t *first_item = listGET_HEAD_ENTRY(&(__vfb_info.event_list));
    for (uint16_t i = 0; i < __vfb_info.event_num; i++) {
        // from __vfb_info.event_list
        EventList_t *event_item = (EventList_t *)listGET_LIST_ITEM_VALUE(first_item);
        if (event_item->event == event) {
            return &event_item->queue_list;
        }
        first_item = first_item->pxNext;
    }
    /* Event首次注册 */
    EventList_t *new_event_item = (EventList_t *)pvPortMalloc(sizeof(EventList_t));
    if (new_event_item == NULL) {
        printf("Failed to malloc memory for event %u", event);
        xSemaphoreGive(__vfb_info.xFDSemaphore);
        return NULL;
    }
    vListInitialise(&new_event_item->queue_list);
    new_event_item->event = event;
    
    ListItem_t *item = (ListItem_t *)pvPortMalloc(sizeof(ListItem_t));
    if (item == NULL) {
        printf("Failed to malloc memory for ListItem_t");
        vPortFree(new_event_item);
        xSemaphoreGive(__vfb_info.xFDSemaphore);
        return NULL;
    }
    vListInitialiseItem(item);
    // set the item value to new_event_item
    listSET_LIST_ITEM_VALUE(item, (uint32_t)new_event_item);
    vListInsertEnd(&(__vfb_info.event_list), item);
    __vfb_info.event_num++;
    return &new_event_item->queue_list;
}
static int __vfb_list_add_queue(List_t *queue_list, QueueHandle_t queue_handle) {
    if (queue_list == NULL || queue_handle == NULL) {
        printf("Queue list or queue handle is NULL");
        return -1;
    }
    ListItem_t *item = (ListItem_t *)pvPortMalloc(sizeof(ListItem_t));
    if (item == NULL) {
        printf("Failed to malloc memory for ListItem_t");
        return -1;
    }
    vListInitialiseItem(item);
    // set the item value to queue_handle
    listSET_LIST_ITEM_VALUE(item, (uint32_t)queue_handle);
    vListInsertEnd(queue_list, item);
    return 0;
}

void vfb_event_register(uint16_t event) {
}
void vfb_event_unregister(uint16_t event) {
}
void vfb_event_counter(uint16_t event, uint16_t *counter) {
}
void vfb_event_get_queue(uint16_t event, uint16_t index, void *item) {
}
/**
 * @brief Initialize the FD server
 *
 */
void vfb_server_init(void) {
    __vfb_info.event_num    = 0;
    __vfb_info.xFDSemaphore = xSemaphoreCreateBinary();
    if (__vfb_info.xFDSemaphore == NULL) {
        printf("Failed to create semaphore for FD server");
        return;
    }
    vListInitialise(&(__vfb_info.event_list));

    xSemaphoreGive(__vfb_info.xFDSemaphore);
    VBF_I("FD server initialized successfully");
}
void vfg_server_info(void) {
    printf("VFB Server Info:\n");
    printf("  Event List Count: %u\n", __vfb_info.event_num);
    printf("  Semaphore Handle: %p\n", (void *)__vfb_info.xFDSemaphore);
    printf("  Event List Head: %p\n", (void *)&(__vfb_info.event_list));
}
// 注册event
QueueHandle_t vfb_subscribe(uint16_t queue_num, uint16_t *event_list, uint16_t event_num) {
    uint16_t valiad_counter  = 0;
    uint16_t invalid_counter = 0;
    /* 获取Task的信息 */
    TaskHandle_t curTaskHandle = xTaskGetCurrentTaskHandle();
    const char *taskName       = "Unknown";
    if (curTaskHandle != NULL) {
        taskName = pcTaskGetName(curTaskHandle);
    } else {
        printf("[ERROR]Failed to get current task handle");
        return NULL;
    }
    QueueHandle_t queue_handle = xQueueCreate(queue_num, sizeof(vfb_message));
    if (queue_handle == NULL) {
        printf("queue_handle is NULL");
        return NULL;
    }
    printf("Task %s Queue %p created, queue_num: %u\r\n", taskName, queue_handle,queue_num);
    if (xSemaphoreTake(__vfb_info.xFDSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (uint16_t i = 0; i < event_num; i++) {  // TODO 当event为空时会有问题,需要改为do while
            List_t *queue_list = __vfb_list_get_head(event_list[i]);
            if (queue_list == NULL) {
                printf("Failed to get queue list for event %u\r\n", event_list[i]);
                invalid_counter++;
                continue;
            } else {
                valiad_counter++;
            }
            __vfb_list_add_queue(queue_list, queue_handle);
        }
        xSemaphoreGive(__vfb_info.xFDSemaphore);
    } else {
        printf("Failed to take semaphore");
        return NULL;
    }
    VBF_I("Task %s subscribe Event Success,valid %d,invalid %d\r\n", taskName, valiad_counter, invalid_counter);
    return queue_handle;
}
/**
 * @brief Send a message to the event queue
 *
 * @param msg The message to send
 * @return uint8_t FD_PASS on success, FD_FAIL on failure
 */
uint8_t vfb_send(uint16_t event, int64_t data, uint16_t length, void *payload) {
    vfb_message tmp_msg;
    tmp_msg.event      = event;
    tmp_msg.data       = data;
    tmp_msg.length     = length;
    tmp_msg.payload    = payload;
    List_t *event_list = __vfb_list_get_head(event);
    if (event_list == NULL) {
        printf("Failed to get event list for event %u", event);
        return FD_FAIL;
    }
    printf("Event %u, registered queue count: %u\r\n", event, listCURRENT_LIST_LENGTH(event_list));
    if (xSemaphoreTake(__vfb_info.xFDSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (uint16_t i = 0; i < listCURRENT_LIST_LENGTH(event_list); i++) {
            ListItem_t *item = listGET_HEAD_ENTRY(event_list);
            if (item == listGET_END_MARKER(event_list)) {
                printf("No queue found for event %u", event);
                xSemaphoreGive(__vfb_info.xFDSemaphore);
                break;
            }
            QueueHandle_t queue_handle = (QueueHandle_t)listGET_LIST_ITEM_VALUE(item);
            if (queue_handle == NULL) {
                printf("Queue handle is NULL for event %u", event);
                xSemaphoreGive(__vfb_info.xFDSemaphore);
                return FD_FAIL;
            }
            printf("Task %s sending message to queue %p for event %u, data: %ld, length: %u\r\n", 
                   pcTaskGetName(xTaskGetCurrentTaskHandle()), queue_handle, event, data, length);
            if (xQueueSend(queue_handle, &tmp_msg, pdMS_TO_TICKS(100)) != pdPASS) {
                printf("Failed to send message to queue for event %u", event);
                xSemaphoreGive(__vfb_info.xFDSemaphore);
                return FD_FAIL;
            }
        }
        printf("Task %s sent message for event %u, data: %ld, length: %u\r\n", pcTaskGetName(xTaskGetCurrentTaskHandle()), event, data, length);
        xSemaphoreGive(__vfb_info.xFDSemaphore);
        return FD_PASS;
    } else {
        printf("Failed to take semaphore");
        return FD_FAIL;
    }
}

#if 0
/**
 * @brief
 *
 * @param thread_info
 * @param is_prvite
 * @param msg
 */
uint8_t vfb_send(vfb_message_t msg) {
    uint32_t index  = vfb_event2index(msg->event);
    uint8_t isfound = FD_FAIL;
    // void *payload    = NULL;
    if (xSemaphoreTake(__vfb_info.xFDSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (uint16_t i = 0; i < MAX_THREAD_NUM; i++) {
            if (event_matrix[index].queue_handle[i] == NULL) {
                break;
            }
            isfound = FD_PASS;
            if (xQueueSend(event_matrix[index].queue_handle[i], msg, pdMS_TO_TICKS(100)) != pdPASS) {
                xSemaphoreGive(__vfb_info.xFDSemaphore);
                printf("Failed to send message to prvite queue");
                return FD_FAIL;
            }
        }
        if (isfound == FD_FAIL) {
            VBF_D("Failed to find event %u,Not found TASK HANDLE", msg->event);
        }
        xSemaphoreGive(__vfb_info.xFDSemaphore);
    } else {
        printf("Failed to take semaphore");
        return FD_FAIL;
    }
    return FD_PASS;
}

uint8_t vfb_send_with_buffer(vfb_message_buffer_t msg) {
    vfb_message tmp_msg;
    tmp_msg.data    = msg->data;
    tmp_msg.event   = msg->event;
    tmp_msg.length  = msg->length;
    tmp_msg.payload = NULL;

    uint32_t index  = vfb_event2index(msg->event);
    uint8_t isfound = FD_FAIL;
    // void *payload    = NULL;
    if (xSemaphoreTake(__vfb_info.xFDSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (uint16_t i = 0; i < MAX_THREAD_NUM; i++) {
            if (event_matrix[index].queue_handle[i] == NULL) {
                break;
            }
            isfound = FD_PASS;

            if ((msg->length > 0) /*&& (msg->payload != NULL)*/ && (msg->length < VFB_PAYLOAD_MAX_SIZE)) {
                tmp_msg.payload = pvPortMalloc(msg->length);
                if (tmp_msg.payload == NULL) {
                    printf("Failed to malloc memory,event %u size %d", msg->event, msg->length);
                    xSemaphoreGive(__vfb_info.xFDSemaphore);
                    return FD_FAIL;
                } else {
                    memcpy(tmp_msg.payload, msg->payload, msg->length);
                }
            }

            if (xQueueSend(event_matrix[index].queue_handle[i], &tmp_msg, pdMS_TO_TICKS(100)) != pdPASS) {
                vPortFree(tmp_msg.payload);
                xSemaphoreGive(__vfb_info.xFDSemaphore);
                printf("Failed to send message to prvite queue");
                return FD_FAIL;
            }
        }
        if (isfound == FD_FAIL) {
            VBF_D("Failed to find event %u,Not found TASK HANDLE", msg->event);
        }
        xSemaphoreGive(__vfb_info.xFDSemaphore);
    } else {
        printf("Failed to take semaphore");
        return FD_FAIL;
    }
    return FD_PASS;
}
uint8_t vfb_publish(uint32_t event) {
    vfb_message msg;
    msg.event   = event,
    msg.data    = 0;
    msg.length  = 0;
    msg.payload = NULL;
    VBF_D("publist event %u", event);
    return vfb_send(&msg);
}

uint8_t vfb_publish_data(uint32_t event, int64_t data) {
    vfb_message msg;
    msg.event   = event,
    msg.data    = data;
    msg.length  = 0;
    msg.payload = NULL;
    VBF_D("publist event %u data %lld", event, data);
    return vfb_send(&msg);
}
uint8_t vfb_wait_event(QueueHandle_t queue, uint32_t *event, int num, uint32_t timeout) {
    uint32_t start_time        = xTaskGetTickCount();
    TaskHandle_t curTaskHandle = xTaskGetCurrentTaskHandle();
    uint32_t event_tmp[8]      = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    int get_num                = 0;
    char *taskName             = NULL;

    if (curTaskHandle != NULL) {
        taskName = pcTaskGetName(curTaskHandle);
    }
    if (num > 8) {
        printf("vfb_wait_event %s  num is too large", taskName);
        return FD_FAIL;
    }
    for (size_t i = 0; i < num; i++) {
        event_tmp[i] = event[i];
    }

    vfb_message msg;
    while (xTaskGetTickCount() - start_time < timeout) {
        if (xQueueReceive(queue, &msg, pdMS_TO_TICKS(100)) == pdTRUE) {
            for (size_t i = 0; i < num; i++) {
                if (msg.event == event_tmp[i]) {
                    get_num++;
                    VBF_I("Task %s get event %u ", taskName, event[i]);
                    event[i] = 0xFFFFFFFF;
                    if (get_num == num) {
                        VBF_I("Task %s wait all event success", taskName);
                        return FD_PASS;
                    }
                }
            }
        }
    }
    for (size_t i = 0; i < num; i++) {
        if (event[i] != 0xFFFFFFFF) {
            printf("Task %s wait event %u timeout", taskName, event[i]);
        }
    }
    return FD_FAIL;
}

// uint8_t VFB_QueueReceive(QueueHandle_t xQueue,
//                          void *const pvBuffer,
//                          TickType_t xTicksToWait) {
//     uint8_t ref;
//     ref = xQueueReceive(xQueue, pvBuffer, xTicksToWait)
// }
#endif  // #if 0