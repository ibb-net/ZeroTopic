#ifndef TOPIC_BUS_H_
#define TOPIC_BUS_H_

#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>
#include "topic_bus_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "../obj_dict/obj_dict.h"
#include "../ring_buffer/ring_buffer.h"
#include "topic_rule.h"
#include "../../Rte/inc/os_semaphore.h"

/* Topic订阅者结构 */
typedef struct topic_subscription {
    void (*callback)(uint16_t topic_id, const void* data, size_t data_len, void* user);
    void* user_data;
    struct topic_subscription* next;
} topic_subscription_t;

/* Topic条目结构 */
typedef struct {
    uint16_t topic_id;              /* Topic ID */
    topic_rule_t rule;              /* 触发规则 */
    topic_subscription_t* subscribers;  /* 订阅者链表 */
#if TOPIC_BUS_ENABLE_STATS
#if TOPIC_BUS_ENABLE_ATOMICS
    atomic_uint_fast32_t event_count;  /* 事件计数（原子操作） */
#else
    uint32_t event_count;           /* 事件计数 */
#endif
#endif
} topic_entry_t;

/* Topic总线结构 */
typedef struct topic_bus topic_bus_t;  /* 前向声明 */

#if TOPIC_BUS_ENABLE_ROUTER
/* 前向声明，避免循环依赖 */
typedef struct topic_router topic_router_t;
#endif

/* Topic总线结构 */
struct topic_bus {
    topic_entry_t* topics;          /* Topic条目数组 */
    size_t max_topics;              /* 最大Topic数量 */
    obj_dict_t* obj_dict;           /* 关联对象字典 */
    OsSemaphore_t* lock;            /* 线程安全锁 */
#if TOPIC_BUS_ENABLE_ISR
    ring_buffer_t* isr_queue;       /* ISR路径缓冲 */
#endif
#if TOPIC_BUS_ENABLE_ROUTER
    topic_router_t* router;        /* Router处理器 */
#endif
};

#if TOPIC_BUS_ENABLE_ROUTER
#include "topic_router.h"
#endif

/* ISR事件结构（用于ISR队列） */
typedef struct {
    obj_dict_key_t event_key;
} topic_bus_isr_event_t;

/* ---------------- 初始化与销毁 ---------------- */

/*
 * @brief 初始化Topic总线
 * @param bus Topic总线指针
 * @param topics Topic条目数组
 * @param max_topics 最大Topic数量
 * @param dict 关联的对象字典
 * @return 0成功，-1失败
 */
int topic_bus_init(topic_bus_t* bus, topic_entry_t* topics, size_t max_topics, obj_dict_t* dict);

/* ---------------- 规则管理 ---------------- */

/*
 * @brief 创建Topic规则
 * @param bus Topic总线指针
 * @param topic_id Topic ID
 * @param rule 规则指针
 * @return 0成功，-1失败
 */
int topic_rule_create(topic_bus_t* bus, uint16_t topic_id, const topic_rule_t* rule);

/* ---------------- 订阅管理 ---------------- */

/*
 * @brief 订阅Topic
 * @param bus Topic总线指针
 * @param topic_id Topic ID
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return 0成功，-1失败
 */
int topic_subscribe(topic_bus_t* bus, uint16_t topic_id,
                    void (*callback)(uint16_t, const void*, size_t, void*),
                    void* user_data);

/*
 * @brief 取消订阅Topic
 * @param bus Topic总线指针
 * @param topic_id Topic ID
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return 0成功，-1失败
 */
int topic_unsubscribe(topic_bus_t* bus, uint16_t topic_id,
                      void (*callback)(uint16_t, const void*, size_t, void*),
                      void* user_data);

/* ---------------- 事件发布 ---------------- */

/*
 * @brief 发布事件（任务模式）
 * @param bus Topic总线指针
 * @param event_key 事件键
 * @return 0成功，-1失败
 */
int topic_publish_event(topic_bus_t* bus, obj_dict_key_t event_key);

/*
 * @brief 手动发布Topic
 * @param bus Topic总线指针
 * @param topic_id Topic ID
 * @return 0成功，-1失败
 */
int topic_publish_manual(topic_bus_t* bus, uint16_t topic_id);

#if TOPIC_BUS_ENABLE_ISR
/*
 * @brief ISR安全发布事件
 * @param bus Topic总线指针
 * @param event_key 事件键
 * @return 0成功，-1失败
 */
int topic_publish_isr(topic_bus_t* bus, obj_dict_key_t event_key);
#endif

/* ---------------- 查询与管理 ---------------- */

/*
 * @brief 处理ISR队列中的事件（任务模式调用）
 * @param bus Topic总线指针
 */
void topic_bus_process_isr_queue(topic_bus_t* bus);

/*
 * @brief 获取Topic事件计数
 * @param bus Topic总线指针
 * @param topic_id Topic ID
 * @return 事件计数，失败返回-1
 */
#if TOPIC_BUS_ENABLE_STATS
int64_t topic_bus_get_event_count(topic_bus_t* bus, uint16_t topic_id);
#endif

#if TOPIC_BUS_ENABLE_ROUTER
/*
 * @brief 设置Topic Router
 * @param bus Topic总线指针
 * @param router Router指针
 * @return 0成功，-1失败
 */
int topic_bus_set_router(topic_bus_t* bus, topic_router_t* router);
#endif

#ifdef __cplusplus
}
#endif

#endif /* TOPIC_BUS_H_ */

