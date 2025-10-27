
#ifndef __VFB_SERVER_H__
#define __VFB_SERVER_H__
#include "vfb_config.h"
#include "os_list.h"
#include "os_queue.h"
#include "os_thread.h"
#include "os_semaphore.h"
#include "os_tick.h"

#define QUEUE_LENGTH 10
#define QUEUE_ITEM_SIZE sizeof(char)

#define MAX_EVENT 256
#define MAX_THREAD_NUM 32  // 最大线程数量

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

// #define VFB_I(...) elog_i(TAG, __VA_ARGS__)
#define VFB_I printf

#define VFB_E printf
#define VFB_W printf
#define VFB_D(...) do { if(0) printf(__VA_ARGS__); } while(0)
#define VFB_SUBSCRIBE(num, list) vfb_subscribe(num, list, sizeof(list) / sizeof(vfb_event_t))

// TODO 改为指针偏移
#define MSG_GET_EVENT(msg) (((vfb_message_t)msg)->frame->head.event)
#define MSG_GET_DATA(msg) (((vfb_message_t)msg)->frame->head.data)
#define MSG_GET_LENGTH(msg) (((vfb_message_t)msg)->frame->head.length)
#define MSG_GET_PAYLOAD(msg) (&(((vfb_message_t)msg)->frame->head.payload_offset))
#define MSG_GET_USE_CNT(msg) (((vfb_message_t)msg)->frame->head.use_cnt)
typedef enum {
    VFB_MSG_MODE_TASK,  // Task mode
    VFB_MSG_MODE_ISR,   // ISR mode
} vfb_msg_mode_t;
typedef uint16_t vfb_event_t;
typedef union {
    uintptr_t *buffer;
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
    OsList_t queue_list;  // List of OsQueue_t* queue_handle
} EventList_t;

typedef struct {
    OsList_t event_list;  // List of events EventList_t list
    uint16_t event_num;
    OsSemaphore_t* xFDSemaphore;
} vfb_info_struct;
typedef vfb_info_struct *vfb_info_t;

typedef struct {
    const char *name;
    int uxPriority;  // not used
    void *pvParameters;
    uint8_t queue_num;
    const vfb_event_t *event_list;
    uint8_t event_num;
    const vfb_event_t *startup_wait_event_list;
    uint8_t startup_wait_event_num;
    uint32_t xTicksToWait;
    void (*init_msg_cb)(void *msg);
    void (*rcv_msg_cb)(void *msg);
    void (*rcv_timeout_cb)(void);
} VFBTaskStruct;

void vfb_server_init(void);

OsQueue_t* vfb_subscribe(uint16_t queue_num, const vfb_event_t *event_list, uint16_t event_num);
void VFB_MsgReceive(OsQueue_t* xQueue,
                    uint32_t xTicksToWait,
                    void (*rcv_msg_cb)(void *),
                    void (*rcv_timeout_cb)(void));
uint8_t vfb_send(vfb_event_t event, uint32_t data, void *payload, uint16_t length);
uint8_t vfb_send_from_isr(vfb_event_t event, uint32_t data, void *payload, uint16_t length);
uint8_t vfb_publish(vfb_event_t event);
void VFBTaskFrame(void *pvParameters);
#endif  // __VFB_SERVER_H__