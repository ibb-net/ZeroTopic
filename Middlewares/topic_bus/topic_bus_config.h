#ifndef TOPIC_BUS_CONFIG_H_
#define TOPIC_BUS_CONFIG_H_

/* 最大Topic数量 */
#ifndef TOPIC_BUS_MAX_TOPICS
#define TOPIC_BUS_MAX_TOPICS 64
#endif

/* 最大订阅者数量（每个Topic） */
#ifndef TOPIC_BUS_MAX_SUBSCRIBERS_PER_TOPIC
#define TOPIC_BUS_MAX_SUBSCRIBERS_PER_TOPIC 16
#endif

/* ISR队列容量（必须是2的幂） */
#ifndef TOPIC_BUS_ISR_QUEUE_SIZE
#define TOPIC_BUS_ISR_QUEUE_SIZE 32
#endif

/* 是否启用统计信息（事件计数等） */
#ifndef TOPIC_BUS_ENABLE_STATS
#define TOPIC_BUS_ENABLE_STATS 1
#endif

/* 是否启用C11原子操作优化 */
#ifndef TOPIC_BUS_ENABLE_ATOMICS
#define TOPIC_BUS_ENABLE_ATOMICS 1
#endif

/* 是否启用Topic规则支持 */
#ifndef TOPIC_BUS_ENABLE_RULES
#define TOPIC_BUS_ENABLE_RULES 1
#endif

/* 是否启用ISR安全发布路径 */
#ifndef TOPIC_BUS_ENABLE_ISR
#define TOPIC_BUS_ENABLE_ISR 1
#endif

/* 最大规则事件数量（每个Topic规则） */
#ifndef TOPIC_BUS_MAX_RULE_EVENTS
#define TOPIC_BUS_MAX_RULE_EVENTS 16
#endif

/* 事件时效性默认超时时间（毫秒） */
#ifndef TOPIC_BUS_DEFAULT_EVENT_TIMEOUT_MS
#define TOPIC_BUS_DEFAULT_EVENT_TIMEOUT_MS 3000
#endif

/* 最大延迟宏：设置为该值时表示不判断时效性 */
#ifndef TOPIC_BUS_EVENT_TIMEOUT_MAX
#define TOPIC_BUS_EVENT_TIMEOUT_MAX 0xFFFFFFFFUL
#endif

/* 是否启用Topic Server周期性运行 */
#ifndef TOPIC_BUS_ENABLE_SERVER
#define TOPIC_BUS_ENABLE_SERVER 1
#endif

/* Topic Server运行周期（毫秒） */
#ifndef TOPIC_BUS_SERVER_PERIOD_MS
#define TOPIC_BUS_SERVER_PERIOD_MS 100
#endif

/* 是否启用Topic Router功能 */
#ifndef TOPIC_BUS_ENABLE_ROUTER
#define TOPIC_BUS_ENABLE_ROUTER 1
#endif

/* 最大Router数量（每个Topic） */
#ifndef TOPIC_BUS_MAX_ROUTERS_PER_TOPIC
#define TOPIC_BUS_MAX_ROUTERS_PER_TOPIC 8
#endif

#endif /* TOPIC_BUS_CONFIG_H_ */

