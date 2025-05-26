/* LOG DEFINE */
// #define LOG_TAG      "VFB"
// #define LOG_LVL      LOG_LVL_INFO
// #include "ulog.h"

#include "vfb.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "vfb_config.h"

#include "api_vfb_signal.h"
#include "queue.h"


// static QueueHandle_t broadcast_queue;
SemaphoreHandle_t xFDSemaphore = NULL;
EventList_t event_matrix[VFB_MAX_EVENT_NUM];

QueueHandle_t vfb_queue_create(uint16_t num) {
    QueueHandle_t queue_handle = xQueueCreate(num, sizeof(vfb_message));
    if (queue_handle == NULL) {
        VBF_E("Failed to create queue");
        return NULL;
    }
    return queue_handle;
}

static uint32_t vfb_event2index(uint32_t event) {
    if (event < SIGNAL_HMI_END) {
        return (event - SIGNAL_HMI_FIRST);
    } else if (event < SIGNAL_GW_END) {
        return (event - SIGNAL_GW_FIRST);
    } else if (event < SIGNAL_GUI_END) {
        return (event - SIGNAL_GUI_FIRST);
    } else if (event < CAN_SIGNAL_ID_END) {
        return (event - CAN_SIGNAL_ID_START);
    } else {
        return 0;
    }
}
/**
 * @brief Initialize the FD server
 *
 */
void vfb_server_init(void) {
    // Create a semaphore
    if ((xFDSemaphore = xSemaphoreCreateBinary()) == NULL) {
        VBF_E("Failed to create semaphore");
        return;
    }
    // Initialize the semaphore
    memset(event_matrix, 0, sizeof(event_matrix));
    xSemaphoreGive(xFDSemaphore);
    VBF_I("FD server initialized successfully,num %d ,size %dbyte %.2f KB",
          VFB_MAX_EVENT_NUM,
          sizeof(event_matrix),
          sizeof(event_matrix) / 1024.0);
}

// 注册event
QueueHandle_t vfb_subscribe(uint16_t queue_num, uint32_t *event_list, uint16_t event_num) {
    uint32_t index           = 0;
    uint32_t valiad_counter  = 0;
    uint32_t invalid_counter = 0;
    uint32_t empty_counter   = 0;
    if (event_num > VFB_MAX_EVENT_NUM) {
        VBF_E("register event num is too large");
        return NULL;
    }
    // size_t queue_size = sizeof(EventList_t) * VFB_MAX_EVENT_NUM;

    TaskHandle_t curTaskHandle = xTaskGetCurrentTaskHandle();
    const char *taskName       = "Unknown";
    if (curTaskHandle != NULL) {
        taskName = pcTaskGetName(curTaskHandle);
    }

    QueueHandle_t queue_handle = xQueueCreate(queue_num, sizeof(vfb_message));
    if (queue_handle == NULL) {
        VBF_E("queue_handle is NULL");
        return NULL;
    }
    if (xSemaphoreTake(xFDSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (uint16_t i = 0; i < event_num; i++) {
            index = vfb_event2index(event_list[i]);
            if (index >= VFB_MAX_EVENT_NUM) {
                VBF_E("event %u is out of range", index);
                invalid_counter++;
                continue;
            }
            if (index == 0) {
                empty_counter++;
                continue;
            }
            valiad_counter++;
            for (uint16_t j = 0; j < MAX_THREAD_NUM; j++) {
                if (event_matrix[index].queue_handle[j] == NULL) {
                    event_matrix[index].queue_handle[j] = queue_handle;
                    // VBF_I("\tevent %lu queue_handle %d", index, j);
                    break;
                } else if (event_matrix[index].queue_handle[j] == queue_handle) {
                    // VBF_I("event %lu queue_handle %d already exist", index, j);
                    break;
                }
            }
        }
        xSemaphoreGive(xFDSemaphore);
    } else {
        VBF_E("Failed to take semaphore");
        return NULL;
    }
    VBF_I("Task %s subscribe Event Success,valid %d,invalid %d,empty %d", taskName, valiad_counter, invalid_counter, empty_counter);
    return queue_handle;
}

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
    if (xSemaphoreTake(xFDSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (uint16_t i = 0; i < MAX_THREAD_NUM; i++) {
            if (event_matrix[index].queue_handle[i] == NULL) {
                break;
            }
            isfound = FD_PASS;
            if (xQueueSend(event_matrix[index].queue_handle[i], msg, pdMS_TO_TICKS(100)) != pdPASS) {
                xSemaphoreGive(xFDSemaphore);
                VBF_E("Failed to send message to prvite queue");
                return FD_FAIL;
            }
        }
        if (isfound == FD_FAIL) {
            VBF_D("Failed to find event %u,Not found TASK HANDLE", msg->event);
        }
        xSemaphoreGive(xFDSemaphore);
    } else {
        VBF_E("Failed to take semaphore");
        return FD_FAIL;
    }
    return FD_PASS;
}
uint8_t vfb_send_auto(vfb_message_t msg) {
    vfb_message tmp_msg;
    tmp_msg.data    = msg->data;
    tmp_msg.event   = msg->event;
    tmp_msg.length  = msg->length;
    tmp_msg.payload = NULL;

    uint32_t index  = vfb_event2index(msg->event);
    uint8_t isfound = FD_FAIL;
    // void *payload    = NULL;
    if (xSemaphoreTake(xFDSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (uint16_t i = 0; i < MAX_THREAD_NUM; i++) {
            if (event_matrix[index].queue_handle[i] == NULL) {
                break;
            }
            isfound = FD_PASS;

            if ((msg->length > 0) && (msg->payload != NULL) && (msg->length < VFB_PAYLOAD_MAX_SIZE)) {
                tmp_msg.payload = pvPortMalloc(msg->length);
                if (tmp_msg.payload == NULL) {
                    VBF_E("Failed to malloc memory,event %u size %d", msg->event, msg->length);
                    xSemaphoreGive(xFDSemaphore);
                    return FD_FAIL;
                } else {
                    memcpy(tmp_msg.payload, msg->payload, msg->length);
                }
            }

            if (xQueueSend(event_matrix[index].queue_handle[i], &tmp_msg, pdMS_TO_TICKS(100)) != pdPASS) {
                vPortFree(tmp_msg.payload);
                xSemaphoreGive(xFDSemaphore);
                VBF_E("Failed to send message to prvite queue");
                return FD_FAIL;
            }
        }
        if (isfound == FD_FAIL) {
            VBF_D("Failed to find event %u,Not found TASK HANDLE", msg->event);
        }
        xSemaphoreGive(xFDSemaphore);
    } else {
        VBF_E("Failed to take semaphore");
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
    if (xSemaphoreTake(xFDSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (uint16_t i = 0; i < MAX_THREAD_NUM; i++) {
            if (event_matrix[index].queue_handle[i] == NULL) {
                break;
            }
            isfound = FD_PASS;

            if ((msg->length > 0) /*&& (msg->payload != NULL)*/ && (msg->length < VFB_PAYLOAD_MAX_SIZE)) {
                tmp_msg.payload = pvPortMalloc(msg->length);
                if (tmp_msg.payload == NULL) {
                    VBF_E("Failed to malloc memory,event %u size %d", msg->event, msg->length);
                    xSemaphoreGive(xFDSemaphore);
                    return FD_FAIL;
                } else {
                    memcpy(tmp_msg.payload, msg->payload, msg->length);
                }
            }

            if (xQueueSend(event_matrix[index].queue_handle[i], &tmp_msg, pdMS_TO_TICKS(100)) != pdPASS) {
                vPortFree(tmp_msg.payload);
                xSemaphoreGive(xFDSemaphore);
                VBF_E("Failed to send message to prvite queue");
                return FD_FAIL;
            }
        }
        if (isfound == FD_FAIL) {
            VBF_D("Failed to find event %u,Not found TASK HANDLE", msg->event);
        }
        xSemaphoreGive(xFDSemaphore);
    } else {
        VBF_E("Failed to take semaphore");
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

uint8_t vfb_publish_data(uint32_t event,int64_t data) {
    vfb_message msg;
    msg.event   = event,
    msg.data    = data;
    msg.length  = 0;
    msg.payload = NULL;
    VBF_D("publist event %u data %lld", event,data);
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
        VBF_E("vfb_wait_event %s  num is too large", taskName);
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
        if(event[i] != 0xFFFFFFFF){
            VBF_E("Task %s wait event %u timeout", taskName, event[i]);
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
