
#ifndef __VFB_SERVER_H__
#define __VFB_SERVER_H__
#include "vfb_config.h"
#include "FreeRTOS.h"



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

#define VFB_HEAD_SIZE (sizeof(uint32_t) + sizeof(int64_t) + sizeof(uint16_t))
#define VFB_TOTOL_SIZE(payload_size) (VFB_HEAD_SIZE + payload_size)
#define VFB_PAYLOAD_MAX_SIZE NORMAL_PAYLOAD_MAX_SIZE
#define VFB_PAYLOAD_4K_MAX_SIZE LARGE_PAYLOAD_MAX_SIZE
#pragma pack(1)
typedef struct
{
    uint32_t event;  // EVENT_LIST
    int64_t data;
    uint16_t length;
    void *payload;
} PACKED vfb_message;
typedef vfb_message *vfb_message_t;
typedef vfb_message vfb_msg;
typedef vfb_message *vfb_msg_t;
typedef struct
{
    uint32_t event;  // EVENT_LIST
    int64_t data;
    uint16_t length;
    uint8_t payload[VFB_PAYLOAD_MAX_SIZE];
} PACKED vfb_message_buffer;
typedef vfb_message_buffer *vfb_message_buffer_t;
typedef struct
{
    uint32_t event;  // EVENT_LIST
    int64_t data;
    uint16_t length;
    uint8_t payload[VFB_PAYLOAD_4K_MAX_SIZE];
} PACKED vfb_message_4k_buffer;
typedef vfb_message_4k_buffer *vfb_message_4k_buffer_t;

#pragma pack()

typedef struct
{
    uint32_t event;  // EVENT_LIST
    QueueHandle_t queue_handle[MAX_THREAD_NUM];
} EventList_t;

void vfb_server_init(void);
#endif  // __VFB_SERVER_H__