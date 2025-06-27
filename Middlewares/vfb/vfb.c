/* LOG DEFINE */

#define TAG       "VFB"
#define VFBLogLvl ELOG_LVL_INFO
#include "elog.h"
#include "vfb/vfb.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "queue.h"
#include "task.h"
#include "vfb/vfb_config.h"

static vfb_info_struct __vfb_info;

static List_t *__vfb_list_get_head(vfb_event_t event);
static int __vfb_list_add_queue(List_t *queue_list, QueueHandle_t queue_handle);
ListItem_t *__vfb_list_find_queue(List_t *queue_list, QueueHandle_t queue_handle);

/**
 * @brief Get the head of the event list
 *
 * @param event The event to get the head of
 * @return List_t* The head of the event list, or NULL if not found
 */
static List_t *__vfb_list_get_head(vfb_event_t event) {
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
        VFB_E("Failed to malloc memory for event %u\r\n", event);
        xSemaphoreGive(__vfb_info.xFDSemaphore);
        return NULL;
    }
    vListInitialise(&new_event_item->queue_list);
    new_event_item->event = event;

    ListItem_t *item = (ListItem_t *)pvPortMalloc(sizeof(ListItem_t));
    if (item == NULL) {
        VFB_E("Failed to malloc memory for ListItem_t\r\n");
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
ListItem_t *__vfb_list_find_queue(List_t *queue_list, QueueHandle_t queue_handle) {
    if (queue_list == NULL || queue_handle == NULL) {
        VFB_E("Queue list or queue handle is NULL");
        return NULL;
    }
    ListItem_t *item = listGET_HEAD_ENTRY(queue_list);
    for (uint16_t i = 0; i < listCURRENT_LIST_LENGTH(queue_list); i++) {
        if (item == listGET_END_MARKER(queue_list)) {
            return NULL;  // Queue list is empty
        }
        if ((QueueHandle_t)listGET_LIST_ITEM_VALUE(item) == queue_handle) {
            return item;
        }
        item = item->pxNext;
    }
    return NULL;
}
static int __vfb_list_add_queue(List_t *queue_list, QueueHandle_t queue_handle) {
    if (queue_list == NULL || queue_handle == NULL) {
        VFB_E("Queue list or queue handle is NULL\r\n");
        return -1;
    }
    // Check if the queue already exists in the list
    ListItem_t *existing_item = __vfb_list_find_queue(queue_list, queue_handle);
    if (existing_item != NULL) {
        VFB_W("Queue %p already exists in the list\r\n", queue_handle);
        return 0;  // Queue already exists, no need to add again
    }
    ListItem_t *item = (ListItem_t *)pvPortMalloc(sizeof(ListItem_t));
    if (item == NULL) {
        VFB_E("Failed to malloc memory for ListItem_t\r\n");
        return -1;
    }
    vListInitialiseItem(item);
    // set the item value to queue_handle
    listSET_LIST_ITEM_VALUE(item, (uint32_t)queue_handle);
    vListInsertEnd(queue_list, item);
    return 0;
}

void vfb_event_register(vfb_event_t event) {
}
void vfb_event_unregister(vfb_event_t event) {
}
void vfb_event_counter(vfb_event_t event, uint16_t *counter) {
}
void vfb_event_get_queue(vfb_event_t event, uint16_t index, void *item) {
}
/**
 * @brief Initialize the FD server
 *
 */
void vfb_server_init(void) {
    __vfb_info.event_num    = 0;
    __vfb_info.xFDSemaphore = xSemaphoreCreateBinary();
    if (__vfb_info.xFDSemaphore == NULL) {
        VFB_E("Failed to create semaphore for FD server");
        return;
    }
    vListInitialise(&(__vfb_info.event_list));
    elog_set_filter_tag_lvl(TAG, VFBLogLvl);

    xSemaphoreGive(__vfb_info.xFDSemaphore);
    VFB_I("FD server initialized successfully\r\n");
}
void vfg_server_info(void) {
    printf("VFB Server Info:\n");
    printf("  Event List Count: %u\n", __vfb_info.event_num);
    printf("  Semaphore Handle: %p\n", (void *)__vfb_info.xFDSemaphore);
    printf("  Event List Head: %p\n", (void *)&(__vfb_info.event_list));
}
// 注册event
QueueHandle_t vfb_subscribe(uint16_t queue_num, const vfb_event_t *event_list, uint16_t event_num) {
    uint16_t valiad_counter  = 0;
    uint16_t invalid_counter = 0;
    /* 获取Task的信息 */
    TaskHandle_t curTaskHandle = xTaskGetCurrentTaskHandle();
    const char *taskName       = "Unknown";
    if (curTaskHandle != NULL) {
        taskName = pcTaskGetName(curTaskHandle);
    } else {
        VFB_E("[ERROR]Failed to get current task handle\r\n");
        return NULL;
    }
    QueueHandle_t queue_handle = xQueueCreate(queue_num, sizeof(vfb_message_t));
    if (queue_handle == NULL) {
        VFB_E("queue_handle is NULL");
        return NULL;
    }
    VFB_D("Task %s Queue %p created, queue_num: %u\r\n", taskName, queue_handle, queue_num);
    if (xSemaphoreTake(__vfb_info.xFDSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (uint16_t i = 0; i < event_num; i++) {
            List_t *queue_list = __vfb_list_get_head(event_list[i]);
            if (queue_list == NULL) {
                VFB_E("Failed to get queue list for event %u\r\n", event_list[i]);
                invalid_counter++;
                continue;
            } else {
                valiad_counter++;
            }
            __vfb_list_add_queue(queue_list, queue_handle);
        }
        xSemaphoreGive(__vfb_info.xFDSemaphore);
    } else {
        VFB_E("Failed to take semaphore\r\n");
        return NULL;
    }
    // VFB_I("Task %s subscribe Event Success,valid %d,invalid %d\r\n", taskName, valiad_counter, invalid_counter);
    return queue_handle;
}
uint8_t __vfb_takelock(vfb_msg_mode_t mode) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (mode == VFB_MSG_MODE_ISR) {
        return xSemaphoreTakeFromISR(__vfb_info.xFDSemaphore, &xHigherPriorityTaskWoken) == pdTRUE ? FD_PASS : FD_FAIL;
    } else if (mode == VFB_MSG_MODE_TASK) {
        return xSemaphoreTake(__vfb_info.xFDSemaphore, pdMS_TO_TICKS(100)) == pdTRUE ? FD_PASS : FD_FAIL;
    } else {
        return FD_FAIL;  // Invalid mode
    }
}
uint8_t __vfb_givelock(vfb_msg_mode_t mode) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (mode == VFB_MSG_MODE_ISR) {
        xSemaphoreGiveFromISR(__vfb_info.xFDSemaphore, &xHigherPriorityTaskWoken);
        return FD_PASS;
    } else if (mode == VFB_MSG_MODE_TASK) {
        xSemaphoreGive(__vfb_info.xFDSemaphore);
        return FD_PASS;
    } else {
        return FD_FAIL;  // Invalid mode
    }
}
uint8_t __vfb_send_queue(vfb_msg_mode_t mode, QueueHandle_t queue_handle,
                         const void *const pvItemToQueue) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (queue_handle == NULL || pvItemToQueue == NULL) {
        VFB_E("Queue handle or item to queue is NULL");
        return FD_FAIL;
    }
    if (mode == VFB_MSG_MODE_ISR) {
        return xQueueSendFromISR(queue_handle, pvItemToQueue, &xHigherPriorityTaskWoken) == pdPASS ? FD_PASS : FD_FAIL;
    } else if (mode == VFB_MSG_MODE_TASK) {
        return xQueueSend(queue_handle, pvItemToQueue, pdMS_TO_TICKS(100)) == pdPASS ? FD_PASS : FD_FAIL;
    } else {
        VFB_E("Invalid mode for sending to queue");
        return FD_FAIL;  // Invalid mode
    }
}
/**
 * @brief Send a message to the event queue
 *
 * @param msg The message to send
 * @return uint8_t FD_PASS on success, FD_FAIL on failure
 */

uint8_t __vfb_send_core(vfb_msg_mode_t mode, vfb_event_t event, uint32_t data, void *payload, uint16_t length) {
    vfb_message tmp_msg;
    if (length > 0 && payload == NULL) {
        VFB_E("Payload is NULL but length is %u for event %u", length, event);
        return FD_FAIL;
    }
    if (length == 0 && payload != NULL) {
        VFB_E("length is 0,but payload is empty for event %u", event);
        return FD_FAIL;
    }

    if (__vfb_takelock(mode) == FD_PASS) {
        List_t *event_list = __vfb_list_get_head(event);
        if (event_list == NULL) {
            VFB_E("Failed to get event list for event %u", event);
            __vfb_givelock(mode);
            return FD_FAIL;
        }
        if (listCURRENT_LIST_LENGTH(event_list) == 0) {
            VFB_W("No queues subscribed for event %u\r\n", event);
            __vfb_givelock(mode);
            return FD_FAIL;
        }
        /* Notice:
 使用 length+head的方式 实际申请的内存空间会比 实际使用多一个1字节,
 在传输字符 等,多出'\0' 不容易溢出 */
        tmp_msg.frame = pvPortMalloc(length + sizeof(vfb_buffer_union));
        if (tmp_msg.frame == NULL) {
            VFB_E("Failed to allocate memory for message frame for event %u\r\n", event);
            __vfb_givelock(mode);
            return FD_FAIL;
        }
        memset(tmp_msg.frame, 0, length + sizeof(vfb_buffer_union));
        tmp_msg.frame->head.event   = event;
        tmp_msg.frame->head.use_cnt = listCURRENT_LIST_LENGTH(event_list);
        tmp_msg.frame->head.data    = data;
        tmp_msg.frame->head.length  = length;
        if (length > 0 && payload != NULL) {
            memcpy(&(tmp_msg.frame->head.payload_offset), payload, length);

        } else {
            tmp_msg.frame->head.payload_offset = NULL;
        }
        // printf("use_cnt %u\r\n", tmp_msg.frame->head.use_cnt);
        ListItem_t *item = listGET_HEAD_ENTRY(event_list);
        for (uint16_t i = 0; i < listCURRENT_LIST_LENGTH(event_list); i++) {
            if (item == listGET_END_MARKER(event_list)) {
                VFB_W("No queue found for event %u\r\n", event);
                break;
            }
            QueueHandle_t queue_handle = (QueueHandle_t)listGET_LIST_ITEM_VALUE(item);
            if (queue_handle == NULL) {
                VFB_E("Queue handle is NULL for event %u", event);
                tmp_msg.frame->head.use_cnt--;
                if (tmp_msg.frame->head.use_cnt == 0) {
                    VFB_W("No queue found for event %u, use count is 0\r\n", event);
                    vPortFree(tmp_msg.frame);
                }
                while (1) {
                    /* code */
                }

                continue;  // Skip this queue
            }

            if (__vfb_send_queue(mode, queue_handle, &tmp_msg) != FD_PASS) {
                VFB_E("Failed to send message to queue for event %u", event);
                tmp_msg.frame->head.use_cnt--;
                if (tmp_msg.frame->head.use_cnt == 0) {
                    VFB_W("No queue found for event %u, use count is 0\r\n", event);
                    vPortFree(tmp_msg.frame);
                }
                /* Notice:当需要检查发送队列有问题的时候就开启这个,方便定位问题 */
                // while (1) {
                //     /* code */
                // }

                continue;  // Skip this queue
            }
            // VFB_D("Message use count incremented, current use count: %u\r\n", tmp_msg.frame->head.use_cnt);
            item = listGET_NEXT(item);
        }
        // printf("use_cnt %u\r\n", tmp_msg.frame->head.use_cnt);
        // VFB_W("Task %s sent message for event %u, data: %ld, length: %u use_cnt %u\r\n", pcTaskGetName(xTaskGetCurrentTaskHandle()), event, data, length, tmp_msg.frame->head.use_cnt);

        // VFB_W("Message sent for event %u, use count: %u\r\n", event, tmp_msg.frame->head.use_cnt);

        __vfb_givelock(mode);
        return FD_PASS;
    } else {
        VFB_E("Failed to take semaphore,event %u\r\n",event);
        return FD_FAIL;
    }
}

uint8_t vfb_send(vfb_event_t event, uint32_t data, void *payload, uint16_t length) {
    return __vfb_send_core(VFB_MSG_MODE_TASK, event, data, payload, length);
}
uint8_t vfb_send_from_isr(vfb_event_t event, uint32_t data, void *payload, uint16_t length) {
    return __vfb_send_core(VFB_MSG_MODE_ISR, event, data, payload, length);
}
uint8_t vfb_publish(vfb_event_t event) {
    return vfb_send(event, 0, NULL, 0);
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

#endif  // #if 0
// uint8_t __vfb_wait_event(QueueHandle_t queue, vfb_event_t *event, int num, uint32_t timeout) {
//     uint32_t start_time        = xTaskGetTickCount();
//     TaskHandle_t curTaskHandle = xTaskGetCurrentTaskHandle();
//     vfb_event_t event_tmp[8]      = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
//     int get_num                = 0;
//     char *taskName             = NULL;

//     if (curTaskHandle != NULL) {
//         taskName = pcTaskGetName(curTaskHandle);
//     }
//     if (num > 8) {
//         printf("vfb_wait_event %s  num is too large", taskName);
//         return FD_FAIL;
//     }
//     for (size_t i = 0; i < num; i++) {
//         event_tmp[i] = event[i];
//     }

//     vfb_message msg;
//     while (xTaskGetTickCount() - start_time < timeout) {
//         if (xQueueReceive(queue, &msg, pdMS_TO_TICKS(100)) == pdTRUE) {
//             for (size_t i = 0; i < num; i++) {
//                 if (msg.event == event_tmp[i]) {
//                     get_num++;
//                     VFB_I("Task %s get event %u ", taskName, event[i]);
//                     event[i] = 0xFFFFFFFF;
//                     if (get_num == num) {
//                         VFB_I("Task %s wait all event success", taskName);
//                         return FD_PASS;
//                     }
//                 }
//             }
//         }
//     }
//     for (size_t i = 0; i < num; i++) {
//         if (event[i] != 0xFFFFFFFF) {
//             printf("Task %s wait event %u timeout", taskName, event[i]);
//         }
//     }
//     return FD_FAIL;
// }

void VFB_MsgReceive(QueueHandle_t xQueue,
                    TickType_t xTicksToWait,
                    void (*rcv_msg_cb)(void *),
                    void (*rcv_timeout_cb)(void)) {
    vfb_message tmp_msg;
    for (;;) {
        if (xQueueReceive(xQueue, &tmp_msg, xTicksToWait) == pdTRUE) {
            vfb_message_t msg = &tmp_msg;
            if (rcv_msg_cb != NULL) {
                rcv_msg_cb(msg);
            }
            if (msg->frame->head.use_cnt == 0) {
                // VFB_E("Message use count is zero, Force freeing message\r\n");
                // vPortFree(msg);
            } else {
                msg->frame->head.use_cnt--;
                // VFB_E("Message use count decremented, current use count: %u\r\n", msg->frame->head.use_cnt);
            }
            if (msg->frame->head.use_cnt == 0) {
                // VFB_E("Message use count is zero, freeing message\r\n");
                vPortFree(msg->frame);
            } else {
                // VFB_E("Message use count is %u, not freeing message\r\n", (msg->frame->head.use_cnt));
            }
            msg = NULL;  // Reset msg pointer to avoid dangling pointer issues
        } else {
            if (rcv_timeout_cb != NULL) {
                rcv_timeout_cb();
            }
        }
    }
}

void VFBTaskFrame(void *pvParameters) {
    VFBTaskStruct *task_cfg = (VFBTaskStruct *)pvParameters;
    if (task_cfg == NULL) {
        VFB_E("Task configuration is NULL");
        return;
    }
    // printf("Create Task %s started\r\n", task_cfg->name);
    //  printf("Task Parameters: pvParameters = %p, queue_num = %u, event_num = %u, xTicksToWait = %d\r\n",
    //         task_cfg->pvParameters, task_cfg->queue_num, task_cfg->event_num, task_cfg->xTicksToWait);
    QueueHandle_t queue_handle = NULL;
    queue_handle               = vfb_subscribe(task_cfg->queue_num, task_cfg->event_list, task_cfg->event_num);
    if (queue_handle == NULL) {
        VFB_E("Failed to subscribe to event queue");
        VFB_E("Task %s will not run, waiting for events forever", task_cfg->name);
        while (1) {
            /* code */
        }

        return;
    }
    if (task_cfg->init_msg_cb != NULL) {
        task_cfg->init_msg_cb(NULL);  // Call the initialization callback if provided
    }
    VFB_MsgReceive(queue_handle, task_cfg->xTicksToWait, task_cfg->rcv_msg_cb, task_cfg->rcv_timeout_cb);
}
