
#ifndef __VFB_SERVER_H__
#define __VFB_SERVER_H__
#include "FreeRTOS.h"
#include "list.h"
#include "queue.h"
#include "semphr.h"
#include "vfb_config.h"

#define QUEUE_LENGTH 10
#define QUEUE_ITEM_SIZE sizeof(char)

#define MAX_EVENT 256
#define MAX_THREAD_NUM MAX_THEAD_COUNT  // 32

#define FD_PASS 0
#define FD_FAIL 1
#define FD_BUSY 2

#ifndef PACKED
#if defined(__GNUC__) || defined(__clang__)

#define PACKED __attribute__((packed))
#elif defined(_MSC_VER)
#define PACKED __pragma(pack(push, 1))
#else
#define PACKED
#endif
#endif

#define VFB_I printf
#define VFB_E printf
#define VFB_W printf
#define VFB_D printf
#define VFB_SUBSCRIBE(num, list) vfb_subscribe(num, list, sizeof(list) / sizeof(vfb_event_t))

typedef uint16_t vfb_event_t;
typedef union PACKED {
    uint8_t *buffer;
    struct {
        vfb_event_t event;  // EVENT_LIST
        uint16_t use_cnt;
        uint32_t data;  // Data associated with the event
        uint16_t length;
        uintptr_t *payload_offset;  // Pointer to the payload data, offset from the start of the struct
    } head;

} vfb_buffer_union;
typedef vfb_buffer_union *vfb_buffer_union_t;
typedef struct
{
    vfb_buffer_union *frame;
} vfb_message;
typedef vfb_message *vfb_message_t;

typedef struct
{
    vfb_event_t event;  // EVENT_LIST
    List_t queue_list;  // List of QueueHandle_t queue_handle
} EventList_t;

typedef struct {
    List_t event_list;  // List of events EventList_t list
    uint16_t event_num;
    SemaphoreHandle_t xFDSemaphore;
} vfb_info_struct;
typedef vfb_info_struct *vfb_info_t;

typedef struct {
    const char *name;
    void *pvParameters;
    uint8_t queue_num;
    const vfb_event_t *event_list;
    uint8_t event_num;
    const vfb_event_t *startup_wait_event_list;
    uint8_t startup_wait_event_num;
    TickType_t xTicksToWait;
    void (*rcv_msg_cb)(void *msg);
    void (*rcv_timeout_cb)(void);
} VFBTaskStruct;

void vfb_server_init(void);

QueueHandle_t vfb_subscribe(uint16_t queue_num, const vfb_event_t *event_list, uint16_t event_num);
void VFB_MsgReceive(QueueHandle_t xQueue,
                    TickType_t xTicksToWait,
                    void (*rcv_msg_cb)(void *),
                    void (*rcv_timeout_cb)(void));
uint8_t vfb_send(vfb_event_t event, uint32_t data, uint16_t length, void *payload);
void VFBTaskFrame(void *pvParameters);
#endif  // __VFB_SERVER_H__