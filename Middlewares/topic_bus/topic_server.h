#ifndef TOPIC_SERVER_H_
#define TOPIC_SERVER_H_

#include <stdint.h>
#include "topic_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Topic Server结构 */
typedef struct {
    topic_bus_t* bus;              /* Topic总线指针 */
    uint32_t period_ms;            /* 运行周期（毫秒） */
    uint32_t last_run_time_ms;     /* 上次运行时间 */
    int running;                    /* 运行标志 */
#if TOPIC_BUS_ENABLE_STATS
    uint64_t total_processed;       /* 总处理事件数 */
    uint64_t total_time_us;        /* 总耗时（微秒） */
#endif
} topic_server_t;

/*
 * @brief 初始化Topic Server
 * @param server Topic Server指针
 * @param bus Topic总线指针
 * @param period_ms 运行周期（毫秒）
 * @return 0成功，-1失败
 */
int topic_server_init(topic_server_t* server, topic_bus_t* bus, uint32_t period_ms);

/*
 * @brief 运行Topic Server一次（周期性调用）
 * @param server Topic Server指针
 * @return 处理的事件数量
 */
int topic_server_run_once(topic_server_t* server);

/*
 * @brief 启动Topic Server（创建任务周期性运行）
 * @param server Topic Server指针
 * @return 0成功，-1失败
 */
int topic_server_start(topic_server_t* server);

/*
 * @brief 停止Topic Server
 * @param server Topic Server指针
 */
void topic_server_stop(topic_server_t* server);

#if TOPIC_BUS_ENABLE_STATS
/*
 * @brief 获取Topic Server性能统计
 * @param server Topic Server指针
 * @param avg_time_us 平均处理时间（微秒，输出参数）
 * @return 总处理事件数
 */
uint64_t topic_server_get_stats(topic_server_t* server, uint64_t* avg_time_us);
#endif

#ifdef __cplusplus
}
#endif

#endif /* TOPIC_SERVER_H_ */

