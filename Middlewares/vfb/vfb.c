/* LOG DEFINE */

#define TAG       "VFB"
#define VFBLogLvl ELOG_LVL_INFO

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

/* Rte抽象层接口 */
#include "os_heap.h"
#include "os_printf.h"
#include "os_semaphore.h"
#include "os_thread.h"
#include "os_timestamp.h"
#include "os_interrupt.h"
#include "os_tick.h"

#include "vfb.h"

#define ERR_HEAD os_printf("                                           \t\n")

volatile vfb_info_struct __vfb_info;

/*
 * VFB 轻量适配层：为队列与时间提供统一封装，便于后续切换到 POSIX 实现。
 * 当前映射到 RTE 抽象层，以保持行为一致。
 */
static inline uint32_t vfb_ms_to_ticks(uint32_t timeout_ms) {
    return os_ms_to_ticks(timeout_ms);
}

static inline OsQueue_t* vfb_queue_create(uint16_t queue_length, size_t item_size) {
    return os_queue_create(queue_length, item_size, "vfb_queue");
}

static inline int vfb_queue_send_task(OsQueue_t* q, const void *item, uint32_t timeout_ms) {
    return os_queue_send(q, item, timeout_ms);
}

static inline int vfb_queue_send_isr(OsQueue_t* q, const void *item) {
    return os_queue_send_isr(q, item);
}

static OsList_t *__vfb_list_get_head(vfb_event_t event);
static int __vfb_list_add_queue(OsList_t *queue_list, OsQueue_t* queue_handle);
OsListItem_t *__vfb_list_find_queue(OsList_t *queue_list, OsQueue_t* queue_handle);
uint8_t error_report(uint8_t state) {
    // #include "gpio.h"
    //     if(state == 0) {
    //         gpio_bit_write(GPIOC, GPIO_PIN_14, GPIO_PIN_RESET);
    //         return state;
    //     }
    //     else
    //     {
    //         gpio_bit_write(GPIOC, GPIO_PIN_14, GPIO_PIN_SET);
    //     }
    return state;
}
/**
 * @brief Get the head of the event list
 *
 * @param event The event to get the head of
 * @return List_t* The head of the event list, or NULL if not found
 */
static OsList_t *__vfb_list_get_head(vfb_event_t event) {
    OsListItem_t *first_item = os_list_get_head_entry(&(__vfb_info.event_list));
    for (uint16_t i = 0; i < __vfb_info.event_num; i++) {
        // from __vfb_info.event_list
        EventList_t *event_item = (EventList_t *)os_list_item_get_value(first_item);
        if (event_item->event == event) {
            return &event_item->queue_list;
        }
        first_item = os_list_get_next(first_item);
    }
    /* Event首次注册 */
    EventList_t *new_event_item = (EventList_t *)os_malloc(sizeof(EventList_t));
    if (new_event_item == NULL) {
        os_printf("[E][%s] Failed to malloc memory for event %u\r\n", TAG, event);
        os_semaphore_give(__vfb_info.xFDSemaphore);
        return NULL;
    }
    os_list_initialise(&new_event_item->queue_list);
    new_event_item->event = event;

    OsListItem_t *item = (OsListItem_t *)os_malloc(sizeof(OsListItem_t));
    if (item == NULL) {
        os_printf("[E][%s] Failed to malloc memory for OsListItem_t\r\n", TAG);
        os_free(new_event_item);
        os_semaphore_give(__vfb_info.xFDSemaphore);
        return NULL;
    }
    os_list_item_initialise(item);
    // set the item value to new_event_item
    os_list_item_set_value(item, (uintptr_t)new_event_item);
    os_list_insert_end(&(__vfb_info.event_list), item);
    __vfb_info.event_num++;
    return &new_event_item->queue_list;
}
OsListItem_t *__vfb_list_find_queue(OsList_t *queue_list, OsQueue_t* queue_handle) {
    if (queue_list == NULL || queue_handle == NULL) {
        os_printf("[E][%s] Queue list or queue handle is NULL\r\n", TAG);
        return NULL;
    }
    OsListItem_t *item = os_list_get_head_entry(queue_list);
    for (uint16_t i = 0; i < os_list_get_length(queue_list); i++) {
        if (item == os_list_get_end_marker(queue_list)) {
            return NULL;  // Queue list is empty
        }
        if ((OsQueue_t*)os_list_item_get_value(item) == queue_handle) {
            return item;
        }
        item = os_list_get_next(item);
    }
    return NULL;
}
static int __vfb_list_add_queue(OsList_t *queue_list, OsQueue_t* queue_handle) {
    if (queue_list == NULL || queue_handle == NULL) {
        os_printf("[E][%s] Queue list or queue handle is NULL\r\n", TAG);
        return -1;
    }
    // Check if the queue already exists in the list
    OsListItem_t *existing_item = __vfb_list_find_queue(queue_list, queue_handle);
    if (existing_item != NULL) {
        os_printf("[W][%s] Queue %p already exists in the list\r\n", TAG, queue_handle);
        return 0;  // Queue already exists, no need to add again
    }
    OsListItem_t *item = (OsListItem_t *)os_malloc(sizeof(OsListItem_t));
    if (item == NULL) {
        os_printf("[E][%s] Failed to malloc memory for OsListItem_t\r\n", TAG);
        return -1;
    }
    os_list_item_initialise(item);
    // set the item value to queue_handle
    os_list_item_set_value(item, (uintptr_t)queue_handle);
    os_list_insert_end(queue_list, item);
    return 0;
}

void vfb_event_register(vfb_event_t event) {}
void vfb_event_unregister(vfb_event_t event) {}
void vfb_event_counter(vfb_event_t event, uint16_t *counter) {}
void vfb_event_get_queue(vfb_event_t event, uint16_t index, void *item) {}
/**
 * @brief Initialize the FD server
 *
 */
void vfb_server_init(void) {
    __vfb_info.event_num    = 0;
    __vfb_info.xFDSemaphore = os_semaphore_create(1, "vfb_fd_semaphore");
    if (__vfb_info.xFDSemaphore == NULL) {
        os_printf("[E][%s] Failed to create semaphore for FD server\r\n", TAG);
        return;
    }
    os_list_initialise(&(__vfb_info.event_list));
    //elog_set_filter_tag_lvl(TAG, VFBLogLvl);

    os_semaphore_give(__vfb_info.xFDSemaphore);
    os_printf("[I][%s] FD server initialized successfully\r\n", TAG);
}
void vfg_server_info(void) {
    printf("VFB Server Info:\n");
    printf("  Event List Count: %u\n", __vfb_info.event_num);
    printf("  Semaphore Handle: %p\n", (void *)__vfb_info.xFDSemaphore);
    printf("  Event List Head: %p\n", (void *)&(__vfb_info.event_list));
}
// 注册event
OsQueue_t* vfb_subscribe(uint16_t queue_num, const vfb_event_t *event_list, uint16_t event_num) {
    uint16_t valiad_counter  = 0;
    uint16_t invalid_counter = 0;
    /* 获取Task的信息 */
    OsThread_t* curTaskHandle = os_thread_get_current();
    const char *taskName       = "Unknown";
    if (curTaskHandle != NULL) {
        // 注意：Rte层没有直接提供获取任务名称的接口，这里需要扩展
        taskName = "VFB_Task"; // 临时使用固定名称
    } else {
        ERR_HEAD;
        os_printf("[E][%s] [ERROR]Failed to get current task handle\r\n", TAG);
        return NULL;
    }
    OsQueue_t* queue_handle = vfb_queue_create(queue_num, sizeof(vfb_message_t));
    if (queue_handle == NULL) {
        ERR_HEAD;
        os_printf("[E][%s] queue_handle is NULL\r\n", TAG);
        return NULL;
    }
    os_printf("[D][%s] Task %s Queue %p created, queue_num: %u\r\n", TAG, taskName, queue_handle, queue_num);
    if (os_semaphore_take(__vfb_info.xFDSemaphore, 300) >= 0) {
        for (uint16_t i = 0; i < event_num; i++) {
            OsList_t *queue_list = __vfb_list_get_head(event_list[i]);
            if (queue_list == NULL) {
                ERR_HEAD;
                os_printf("[E][%s] Failed to get queue list for event %u\r\n", TAG, event_list[i]);
                invalid_counter++;
                continue;
            } else {
                valiad_counter++;
            }
            __vfb_list_add_queue(queue_list, queue_handle);
        }
        os_semaphore_give(__vfb_info.xFDSemaphore);
    } else {
        ERR_HEAD;
        os_printf("[E][%s] Failed to take semaphore\r\n", TAG);
        return NULL;
    }
    // VFB_I("Task %s subscribe Event Success,valid %d,invalid %d\r\n", taskName, valiad_counter,
    // invalid_counter);
    return queue_handle;
}
uint8_t __vfb_takelock(vfb_msg_mode_t mode) {
    if (mode == VFB_MSG_MODE_ISR) {
        // 注意：Rte层没有提供中断模式的信号量获取接口，这里需要扩展
        // 临时使用任务模式
        return os_semaphore_take(__vfb_info.xFDSemaphore, 0) >= 0 ? FD_PASS : FD_FAIL;
    } else if (mode == VFB_MSG_MODE_TASK) {
        return os_semaphore_take(__vfb_info.xFDSemaphore, 100) >= 0 ? FD_PASS : FD_FAIL;
    } else {
        return FD_FAIL;  // Invalid mode
    }
}
uint8_t __vfb_givelock(vfb_msg_mode_t mode) {
    if (mode == VFB_MSG_MODE_ISR) {
        os_semaphore_give_isr(__vfb_info.xFDSemaphore);
        return FD_PASS;
    } else if (mode == VFB_MSG_MODE_TASK) {
        os_semaphore_give(__vfb_info.xFDSemaphore);
        return FD_PASS;
    } else {
        return FD_FAIL;  // Invalid mode
    }
}
uint8_t __vfb_send_queue(vfb_msg_mode_t mode, OsQueue_t* queue_handle,
                         const void *const pvItemToQueue) {
    if (queue_handle == NULL || pvItemToQueue == NULL) {
        os_printf("[E][%s] Queue handle or item to queue is NULL\r\n", TAG);
        return FD_FAIL;
    }
    if (mode == VFB_MSG_MODE_ISR) {
        // 使用适配封装，后续可切换至 POSIX 非阻塞语义
        return vfb_queue_send_isr(queue_handle, pvItemToQueue) == 0 ? FD_PASS : FD_FAIL;
    } else if (mode == VFB_MSG_MODE_TASK) {
        return vfb_queue_send_task(queue_handle, pvItemToQueue, 100) == 0 ? FD_PASS : FD_FAIL;
    } else {
        os_printf("[E][%s] Invalid mode for sending to queue\r\n", TAG);
        return FD_FAIL;  // Invalid mode
    }
}
/**
 * @brief Send a message to the event queue
 *
 * @param msg The message to send
 * @return uint8_t FD_PASS on success, FD_FAIL on failure
 */

uint8_t __vfb_send_core(vfb_msg_mode_t mode, vfb_event_t event, uint32_t data, void *payload,
                        uint16_t length) {
    vfb_message tmp_msg;
    if (length > 0 && payload == NULL) {
        os_printf("[E][%s] Payload is NULL but length is %u for event %u\r\n", TAG, length, event);
        return FD_FAIL;
    }
    if (length == 0 && payload != NULL) {
        os_printf("[E][%s] length is 0,but payload is empty for event %u\r\n", TAG, event);
        return FD_FAIL;
    }

    if (__vfb_takelock(mode) == FD_PASS) {
        OsList_t *event_list = __vfb_list_get_head(event);
        if (event_list == NULL) {
            os_printf("[E][%s] Failed to get event list for event %u\r\n", TAG, event);
            error_report(1);
            __vfb_givelock(mode);
            return FD_FAIL;
        }
        if (os_list_get_length(event_list) == 0) {
            os_printf("[W][%s] No queues subscribed for event %u\r\n", TAG, event);
            error_report(1);
            __vfb_givelock(mode);
            return FD_FAIL;
        }
        /* Notice:
 使用 length+head的方式 实际申请的内存空间会比 实际使用多一个1字节,
 在传输字符 等,多出'\0' 不容易溢出 */
        tmp_msg.frame = os_malloc(length + sizeof(vfb_buffer_union));
        if (tmp_msg.frame == NULL) {
            os_printf("[E][%s] Failed to allocate memory for message frame for event %u\r\n", TAG, event);
            __vfb_givelock(mode);
            return FD_FAIL;
        }
        memset(tmp_msg.frame, 0, length + sizeof(vfb_buffer_union));
        tmp_msg.frame->head.event   = event;
        tmp_msg.frame->head.use_cnt = os_list_get_length(event_list);
        tmp_msg.frame->head.data    = data;
        tmp_msg.frame->head.length  = length;
        if (length > 0 && payload != NULL) {
            // payload数据紧跟在header后面，让payload_offset指向那个位置
            tmp_msg.frame->head.payload_offset = (uintptr_t*)((uint8_t*)tmp_msg.frame + sizeof(vfb_buffer_union));
            // 将payload数据复制到payload_offset指向的位置
            memcpy(tmp_msg.frame->head.payload_offset, payload, length);
        } else {
            tmp_msg.frame->head.payload_offset = NULL;
        }
        // printf("use_cnt %u\r\n", tmp_msg.frame->head.use_cnt);
        OsListItem_t *item = os_list_get_head_entry(event_list);
        for (uint16_t i = 0; i < os_list_get_length(event_list); i++) {
            if (item == os_list_get_end_marker(event_list)) {
                os_printf("[W][%s] No queue found for event %u\r\n", TAG, event);
                break;
            }
            OsQueue_t* queue_handle = (OsQueue_t*)os_list_item_get_value(item);
            if (queue_handle == NULL) {
                os_printf("[E][%s] Queue handle is NULL for event %u\r\n", TAG, event);
                tmp_msg.frame->head.use_cnt--;
                if (tmp_msg.frame->head.use_cnt == 0) {
                    os_printf("[W][%s] No queue found for event %u, use count is 0\r\n", TAG, event);
                    os_free(tmp_msg.frame);
                }
                while (1) {
                    /* code */
                }

                continue;  // Skip this queue
            }

            if (__vfb_send_queue(mode, queue_handle, &tmp_msg) != FD_PASS) {
                os_printf("[E][%s] Failed to send message to queue for event %u\r\n", TAG, event);
                tmp_msg.frame->head.use_cnt--;
                if (tmp_msg.frame->head.use_cnt == 0) {
                    os_printf("[W][%s] No queue found for event %u, use count is 0\r\n", TAG, event);
                    os_free(tmp_msg.frame);
                }
                /* Notice:当需要检查发送队列有问题的时候就开启这个,方便定位问题 */
                // while (1) {
                //     /* code */
                // }

                continue;  // Skip this queue
            }
            item = os_list_get_next(item);
        }

        __vfb_givelock(mode);
        return FD_PASS;
    } else {
        os_printf("[E][%s] Failed to take semaphore,event %u\r\n", TAG, event);
        return FD_FAIL;
    }
}

uint8_t vfb_send(vfb_event_t event, uint32_t data, void *payload, uint16_t length) {
    return __vfb_send_core(VFB_MSG_MODE_TASK, event, data, payload, length);
}
uint8_t vfb_send_from_isr(vfb_event_t event, uint32_t data, void *payload, uint16_t length) {
    return __vfb_send_core(VFB_MSG_MODE_ISR, event, data, payload, length);
}
uint8_t vfb_publish(vfb_event_t event) { return vfb_send(event, 0, NULL, 0); }
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
//     vfb_event_t event_tmp[8]      = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
//     0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF}; int get_num                = 0; char *taskName = NULL;

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

void VFB_MsgReceive(OsQueue_t* xQueue, uint32_t xTicksToWait, void (*rcv_msg_cb)(void *),
                    void (*rcv_timeout_cb)(void)) {
    vfb_message tmp_msg;
    int timeout_count = 0;
    const int max_timeouts = 10; // 最大超时次数，避免无限循环
    
    for (;;) {
        if (os_queue_receive(xQueue, &tmp_msg, xTicksToWait) == 0) {
            vfb_message_t msg = &tmp_msg;
            timeout_count = 0; // 重置超时计数
            if (rcv_msg_cb != NULL) {
                rcv_msg_cb(msg);
            }
            if (msg->frame->head.use_cnt == 0) {
                // os_printf("[E][%s] Message use count is zero, Force freeing message\r\n", TAG);
                // os_free(msg);
            } else {
                msg->frame->head.use_cnt--;
                // os_printf("[E][%s] Message use count decremented, current use count: %u\r\n", TAG,
                // msg->frame->head.use_cnt);
            }
            if (msg->frame->head.use_cnt == 0) {
                // os_printf("[E][%s] Message use count is zero, freeing message\r\n", TAG);
                os_free(msg->frame);
            } else {
                // os_printf("[E][%s] Message use count is %u, not freeing message\r\n", TAG,
                // (msg->frame->head.use_cnt));
            }
            msg = NULL;  // Reset msg pointer to avoid dangling pointer issues
        } else {
            timeout_count++;
            if (rcv_timeout_cb != NULL) {
                rcv_timeout_cb();
            }
            // 如果连续超时次数过多，退出循环
            if (timeout_count >= max_timeouts) {
                os_printf("[I][%s] Too many timeouts, exiting message receive loop\r\n", TAG);
                break;
            }
        }
    }
}

void VFBTaskFrame(void *pvParameters) {
    VFBTaskStruct *task_cfg = (VFBTaskStruct *)pvParameters;
    if (task_cfg == NULL) {
        os_printf("[E][%s] Task configuration is NULL\r\n", TAG);
        return;
    }
    // printf("Create Task %s started\r\n", task_cfg->name);
    //  printf("Task Parameters: pvParameters = %p, queue_num = %u, event_num = %u, xTicksToWait =
    //  %d\r\n",
    //         task_cfg->pvParameters, task_cfg->queue_num, task_cfg->event_num,
    //         task_cfg->xTicksToWait);
    OsQueue_t* queue_handle = NULL;
    queue_handle = vfb_subscribe(task_cfg->queue_num, task_cfg->event_list, task_cfg->event_num);
    if (queue_handle == NULL) {
        os_printf("[E][%s] Failed to subscribe to event queue\r\n", TAG);
        os_printf("[E][%s] Task %s will not run, waiting for events forever\r\n", TAG, task_cfg->name);
        while (1) {
            /* code */
        }

        return;
    }
    if (task_cfg->init_msg_cb != NULL) {
        task_cfg->init_msg_cb(NULL);  // Call the initialization callback if provided
    }
    VFB_MsgReceive(queue_handle, task_cfg->xTicksToWait, task_cfg->rcv_msg_cb,
                   task_cfg->rcv_timeout_cb);
}
