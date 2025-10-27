/***************************************************************************
 *
 * Copyright (c) 2024 OpenIBBOs. All Rights Reserved
 *
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdatomic.h>

// OpenIBBOs RTE头文件
#include "../../Rte/inc/os_init.h"
#include "../../Rte/inc/os_printf.h"
#include "../../Rte/inc/os_thread.h"
#include "../../Rte/inc/os_tick.h"

// VFB头文件
#include "../../Middlewares/vfb/vfb.h"

// 配置文件
#include "config.h"

// 测试事件定义
#define VFB_EVENT_TEST_1    1
#define VFB_EVENT_TEST_2    2
#define VFB_EVENT_TEST_3    3
#define VFB_EVENT_SHUTDOWN  4

// payload数据结构，包含校验信息
// 使用packed确保没有填充，checksum紧跟在数据后面
typedef struct __attribute__((packed)) {
    uint8_t data[32];      // 数据部分
    uint32_t checksum;      // 校验和紧跟在数据后面
} PayloadData32_t;

typedef struct __attribute__((packed)) {
    uint8_t data[256];      // 数据部分
    uint32_t checksum;      // 校验和紧跟在数据后面
} PayloadData256_t;

// 测试数据结构
typedef struct {
    OsQueue_t* queue;
    int thread_id;
    char thread_name[32];  // 线程名称
    int message_count;
    int send_completed;    // 发送完成标志
    atomic_int stop_flag;
} TestThreadData_t;

// 全局变量
TestThreadData_t g_vfb_thread_data[3] = {0};

/**
 * @brief 计算校验和
 */
uint32_t calculate_checksum(const uint8_t* data, size_t len) {
    uint32_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum;
}

/**
 * @brief 生成32字节测试payload
 */
void generate_payload32(PayloadData32_t* payload, uint8_t pattern) {
    if (payload == NULL) {
        return;
    }
    
    for (int i = 0; i < 32; i++) {
        payload->data[i] = pattern + (uint8_t)i;
    }
    payload->checksum = calculate_checksum(payload->data, 32);
}

/**
 * @brief 生成256字节测试payload
 */
void generate_payload256(PayloadData256_t* payload, uint8_t pattern) {
    if (payload == NULL) {
        return;
    }
    
    for (int i = 0; i < 256; i++) {
        payload->data[i] = pattern + (uint8_t)i;
    }
    payload->checksum = calculate_checksum(payload->data, 256);
}

/**
 * @brief 根据队列指针查找线程数据
 */
TestThreadData_t* find_thread_data_by_queue(OsQueue_t* queue) {
    for (int i = 0; i < 3; i++) {
        if (g_vfb_thread_data[i].queue == queue) {
            return &g_vfb_thread_data[i];
        }
    }
    return NULL;
}

/**
 * @brief 消息接收回调函数 - Thread B
 */
void vfb_message_callback_b(void* msg) {
    vfb_message_t vfb_msg = (vfb_message_t)msg;
    TestThreadData_t* pData = &g_vfb_thread_data[1]; // Thread B
    
    os_printf("[%s] 收到事件: %u, 数据: %u, 长度: %u", 
              pData->thread_name,
              vfb_msg->frame->head.event,
              vfb_msg->frame->head.data,
              vfb_msg->frame->head.length);
    
    // 处理payload并验证校验和
    if (vfb_msg->frame->head.length > sizeof(uint32_t) && vfb_msg->frame->head.payload_offset != NULL) {
        uint8_t* payload_bytes = (uint8_t*)vfb_msg->frame->head.payload_offset;
        // length包括数据和校验和，所以数据长度 = length - sizeof(checksum)
        size_t data_len = vfb_msg->frame->head.length - sizeof(uint32_t);
        
        
        // 读取checksum（在数据后面）
        uint32_t received_checksum;
        memcpy(&received_checksum, payload_bytes + data_len, sizeof(uint32_t));
        
        // 计算校验和
        uint32_t calc_checksum = calculate_checksum(payload_bytes, data_len);
        os_printf(", 校验值: 0x%08X", received_checksum);
        if (calc_checksum == received_checksum) {
            os_printf(" [校验通过]\n");
        } else {
            os_printf(" [校验失败! 期望: 0x%08X]\n", calc_checksum);
        }
    } else {
        os_printf("\n");
    }
    
    pData->message_count++;
}

/**
 * @brief 消息接收回调函数 - Thread C
 */
void vfb_message_callback_c(void* msg) {
    vfb_message_t vfb_msg = (vfb_message_t)msg;
    TestThreadData_t* pData = &g_vfb_thread_data[2]; // Thread C
    
    os_printf("[%s] 收到事件: %u, 数据: %u, 长度: %u", 
              pData->thread_name,
              vfb_msg->frame->head.event,
              vfb_msg->frame->head.data,
              vfb_msg->frame->head.length);
    
    // 处理payload并验证校验和
    if (vfb_msg->frame->head.length > sizeof(uint32_t) && vfb_msg->frame->head.payload_offset != NULL) {
        uint8_t* payload_bytes = (uint8_t*)vfb_msg->frame->head.payload_offset;
        // length包括数据和校验和，所以数据长度 = length - sizeof(checksum)
        size_t data_len = vfb_msg->frame->head.length - sizeof(uint32_t);
        
        
        // 读取checksum（在数据后面）
        uint32_t received_checksum;
        memcpy(&received_checksum, payload_bytes + data_len, sizeof(uint32_t));
        
        // 计算校验和
        uint32_t calc_checksum = calculate_checksum(payload_bytes, data_len);
        os_printf(", 校验值: 0x%08X", received_checksum);
        if (calc_checksum == received_checksum) {
            os_printf(" [校验通过]\n");
        } else {
            os_printf(" [校验失败! 期望: 0x%08X]\n", calc_checksum);
        }
    } else {
        os_printf("\n");
    }
    
    pData->message_count++;
}

/**
 * @brief 超时回调函数 - Thread B
 */
void vfb_timeout_callback_b(void) {
    TestThreadData_t* pData = &g_vfb_thread_data[1]; // Thread B
    
    // 检查发送者是否完成
    if (g_vfb_thread_data[0].send_completed) {
        // 发送已完成，正常超时，不打印
    } else {
        os_printf("[ERROR][%s] 消息接收超时（发送未完成）\n", pData->thread_name);
    }
}

/**
 * @brief 超时回调函数 - Thread C
 */
void vfb_timeout_callback_c(void) {
    TestThreadData_t* pData = &g_vfb_thread_data[2]; // Thread C
    
    // 检查发送者是否完成
    if (g_vfb_thread_data[0].send_completed) {
        // 发送已完成，正常超时，不打印
    } else {
        os_printf("[ERROR][%s] 消息接收超时（发送未完成）\n", pData->thread_name);
    }
}

/**
 * @brief 线程A入口函数 - 事件发布者
 */
void* vfb_thread_a_entry(void* pParameter) {
    TestThreadData_t* pData = (TestThreadData_t*)pParameter;
    strcpy(pData->thread_name, "Thread A");
    
    os_printf("[%s] 开始运行，线程ID: %d\n", pData->thread_name, pData->thread_id);
    
    // 等待其他线程准备就绪
    os_thread_sleep_ms(100);
    
    PayloadData32_t payload32_data;
    PayloadData256_t payload256_data;
    
    for (int i = 0; i < 5 && !pData->stop_flag; i++) {
        // 1. 发送无payload消息（0字节）
        uint8_t result = vfb_send(VFB_EVENT_TEST_1, i, NULL, 0);
        if (result == FD_PASS) {
            os_printf("[%s] 发布事件: %u, 数据: %d, 长度: 0\n", 
                      pData->thread_name, VFB_EVENT_TEST_1, i);
            pData->message_count++;
        } else {
            os_printf("[%s] 发布事件失败\n", pData->thread_name);
        }
        
        // 2. 发送32字节payload消息（包括数据+校验和）
        generate_payload32(&payload32_data, 0xAA);
        result = vfb_send(VFB_EVENT_TEST_2, 100 + i, &payload32_data, sizeof(PayloadData32_t));
        if (result == FD_PASS) {
            os_printf("[%s] 发布事件: %u, 数据: %d, 长度: %zu, 校验值: 0x%08X\n",
                      pData->thread_name, VFB_EVENT_TEST_2, 100 + i, sizeof(PayloadData32_t), payload32_data.checksum);
            pData->message_count++;
        }
        
        os_thread_sleep_ms(200);
    }
    
    // 3. 发送256字节payload消息（包括数据+校验和）
    generate_payload256(&payload256_data, 0x55);
    uint8_t result = vfb_send(VFB_EVENT_TEST_3, 999, &payload256_data, sizeof(PayloadData256_t));
    if (result == FD_PASS) {
        os_printf("[%s] 发布事件: %u, 数据: 999, 长度: %zu, 校验值: 0x%08X\n",
                  pData->thread_name, VFB_EVENT_TEST_3, sizeof(PayloadData256_t), payload256_data.checksum);
        pData->message_count++;
    }
    
    pData->send_completed = 1;  // 标记发送完成
    os_printf("[%s] 完成，总共发送了 %d 条消息\n", pData->thread_name, pData->message_count);
    
    return NULL;
}

/**
 * @brief 线程B入口函数 - 事件订阅者1
 */
void* vfb_thread_b_entry(void* pParameter) {
    TestThreadData_t* pData = (TestThreadData_t*)pParameter;
    strcpy(pData->thread_name, "Thread B");
    
    os_printf("[%s] 开始运行，线程ID: %d\n", pData->thread_name, pData->thread_id);
    
    // 订阅事件
    vfb_event_t events[] = {VFB_EVENT_TEST_1, VFB_EVENT_TEST_2};
    OsQueue_t* queue = vfb_subscribe(10, events, 2);
    
    if (queue == NULL) {
        os_printf("[%s] 订阅事件失败\n", pData->thread_name);
        return NULL;
    }
    
    pData->queue = queue;  // 保存队列指针
    os_printf("[%s] 成功订阅事件 VFB_EVENT_TEST_1 和 VFB_EVENT_TEST_2\n", pData->thread_name);
    
    // 接收消息，使用独立的回调函数
    VFB_MsgReceive(queue, VFB_TEST_TIMEOUT_MS, vfb_message_callback_b, vfb_timeout_callback_b);
    
    os_printf("[%s] 完成，总共接收了 %d 条消息\n", pData->thread_name, pData->message_count);
    return NULL;
}

/**
 * @brief 线程C入口函数 - 事件订阅者2
 */
void* vfb_thread_c_entry(void* pParameter) {
    TestThreadData_t* pData = (TestThreadData_t*)pParameter;
    strcpy(pData->thread_name, "Thread C");
    
    os_printf("[%s] 开始运行，线程ID: %d\n", pData->thread_name, pData->thread_id);
    
    // 订阅事件
    vfb_event_t events[] = {VFB_EVENT_TEST_2, VFB_EVENT_TEST_3};
    OsQueue_t* queue = vfb_subscribe(10, events, 2);
    
    if (queue == NULL) {
        os_printf("[%s] 订阅事件失败\n", pData->thread_name);
        return NULL;
    }
    
    pData->queue = queue;  // 保存队列指针
    os_printf("[%s] 成功订阅事件 VFB_EVENT_TEST_2 和 VFB_EVENT_TEST_3\n", pData->thread_name);
    
    // 接收消息，使用独立的回调函数
    VFB_MsgReceive(queue, VFB_TEST_TIMEOUT_MS, vfb_message_callback_c, vfb_timeout_callback_c);
    
    os_printf("[%s] 完成，总共接收了 %d 条消息\n", pData->thread_name, pData->message_count);
    return NULL;
}

/**
 * @brief VFB测试主函数
 */
int vfb_test_main(void) {
    OsThread_t* pThreadA = NULL;
    OsThread_t* pThreadB = NULL;
    OsThread_t* pThreadC = NULL;
    ThreadAttr_t threadAttr = {
        .pName = "VFBTestThread",
        .Priority = 5,
        .StackSize = 4096,
        .ScheduleType = 0
    };
    
    os_printf("=== VFB 消息发布订阅测试 ===\n");
    
    // 初始化VFB服务器
    vfb_server_init();
    os_printf("VFB服务器初始化完成\n");
    
    // 初始化线程数据
    for (int i = 0; i < 3; i++) {
        g_vfb_thread_data[i].queue = NULL;
        g_vfb_thread_data[i].thread_id = i + 1;
        memset(g_vfb_thread_data[i].thread_name, 0, sizeof(g_vfb_thread_data[i].thread_name));
        g_vfb_thread_data[i].message_count = 0;
        g_vfb_thread_data[i].send_completed = 0;
        g_vfb_thread_data[i].stop_flag = 0;
    }
    
    // 创建线程A (发布者)
    threadAttr.pName = "VFBThreadA";
    pThreadA = os_thread_create(vfb_thread_a_entry, &g_vfb_thread_data[0], &threadAttr);
    if (pThreadA == NULL) {
        os_printf("创建线程A失败\n");
        return -1;
    }
    
    // 创建线程B (订阅者1)
    threadAttr.pName = "VFBThreadB";
    pThreadB = os_thread_create(vfb_thread_b_entry, &g_vfb_thread_data[1], &threadAttr);
    if (pThreadB == NULL) {
        os_printf("创建线程B失败\n");
        os_thread_destroy(pThreadA);
        return -1;
    }
    
    // 创建线程C (订阅者2)
    threadAttr.pName = "VFBThreadC";
    pThreadC = os_thread_create(vfb_thread_c_entry, &g_vfb_thread_data[2], &threadAttr);
    if (pThreadC == NULL) {
        os_printf("创建线程C失败\n");
        os_thread_destroy(pThreadA);
        os_thread_destroy(pThreadB);
        return -1;
    }
    
    os_printf("所有线程已创建，开始测试...\n");
    
    // 等待所有线程完成
    os_thread_join(pThreadA);
    os_thread_join(pThreadB);
    os_thread_join(pThreadC);
    
    os_printf("\n=== VFB测试完成 ===\n");
    os_printf("[%s] 发送消息数: %d\n", g_vfb_thread_data[0].thread_name, g_vfb_thread_data[0].message_count);
    os_printf("[%s] 接收消息数: %d\n", g_vfb_thread_data[1].thread_name, g_vfb_thread_data[1].message_count);
    os_printf("[%s] 接收消息数: %d\n", g_vfb_thread_data[2].thread_name, g_vfb_thread_data[2].message_count);
    
    // 清理资源
    os_thread_destroy(pThreadA);
    os_thread_destroy(pThreadB);
    os_thread_destroy(pThreadC);
    
    return 0;
}
