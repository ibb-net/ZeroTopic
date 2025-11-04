#include <string.h>
#include "topic_server.h"
#include "../../Rte/inc/os_timestamp.h"
#include "../../Rte/inc/os_thread.h"
#include "../../Rte/inc/os_printf.h"

#if TOPIC_BUS_ENABLE_ISR
#include "topic_bus.h"
#include "../ring_buffer/ring_buffer.h"
#endif

/* Topic Server任务函数 */
static void* topic_server_task(void* arg) {
    topic_server_t* server = (topic_server_t*)arg;
    if (!server) return NULL;

    server->running = 1;
    while (server->running) {
        uint64_t start_us = os_monotonic_time_get_microsecond();
        int processed = topic_server_run_once(server);
        uint64_t end_us = os_monotonic_time_get_microsecond();
        
#if TOPIC_BUS_ENABLE_STATS
        if (processed > 0) {
            server->total_processed += processed;
            server->total_time_us += (end_us - start_us);
        }
#endif

        /* 等待下一个周期 */
        os_thread_sleep_ms(server->period_ms);
    }

    return NULL;
}

/*
 * @brief 初始化Topic Server
 * @param server Topic Server指针
 * @param bus Topic总线指针
 * @param period_ms 运行周期（毫秒）
 * @return 0成功，-1失败
 */
int topic_server_init(topic_server_t* server, topic_bus_t* bus, uint32_t period_ms) {
    if (!server || !bus) return -1;
    
    memset(server, 0, sizeof(topic_server_t));
    server->bus = bus;
    server->period_ms = period_ms;
    server->running = 0;
    
    return 0;
}

/*
 * @brief 运行Topic Server一次（周期性调用）
 * @param server Topic Server指针
 * @return 处理的事件数量
 */
int topic_server_run_once(topic_server_t* server) {
    if (!server || !server->bus) return 0;
    
    int processed = 0;
    
#if TOPIC_BUS_ENABLE_ISR
    /* 处理ISR队列中的事件 */
    if (server->bus->isr_queue) {
        /* 统计处理的事件数量 */
        topic_bus_isr_event_t evt;
        while (ring_buffer_read(server->bus->isr_queue, &evt) == 0) {
            /* 在任务上下文中处理事件 */
            (void)topic_publish_event(server->bus, evt.event_key);
            processed++;
        }
    }
#else
    /* 非ISR模式下，可以添加其他处理逻辑 */
    processed = 0;
#endif

    /* 检查Topic Bus性能（可选） */
    /* 这里可以添加性能检测逻辑 */
    
    return processed;
}

/*
 * @brief 启动Topic Server（创建任务周期性运行）
 * @param server Topic Server指针
 * @return 0成功，-1失败
 */
int topic_server_start(topic_server_t* server) {
    if (!server || server->running) return -1;
    
    ThreadAttr_t attr = {
        .pName = "TopicServer",
        .Priority = 5,
        .StackSize = 4096,
        .ScheduleType = 0
    };
    
    OsThread_t* thread = os_thread_create(topic_server_task, server, &attr);
    if (!thread) return -1;
    
    return 0;
}

/*
 * @brief 停止Topic Server
 * @param server Topic Server指针
 */
void topic_server_stop(topic_server_t* server) {
    if (!server) return;
    server->running = 0;
}

#if TOPIC_BUS_ENABLE_STATS
/*
 * @brief 获取Topic Server性能统计
 * @param server Topic Server指针
 * @param avg_time_us 平均处理时间（微秒，输出参数）
 * @return 总处理事件数
 */
uint64_t topic_server_get_stats(topic_server_t* server, uint64_t* avg_time_us) {
    if (!server) {
        if (avg_time_us) *avg_time_us = 0;
        return 0;
    }
    
    if (avg_time_us) {
        if (server->total_processed > 0) {
            *avg_time_us = server->total_time_us / server->total_processed;
        } else {
            *avg_time_us = 0;
        }
    }
    
    return server->total_processed;
}
#endif

