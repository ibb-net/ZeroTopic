#ifndef TOPIC_RULE_H_
#define TOPIC_RULE_H_

#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>
#include "topic_bus_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "../obj_dict/obj_dict.h"

/* Topic规则类型 */
typedef enum {
    TOPIC_RULE_OR,      /* 任一事件触发 */
    TOPIC_RULE_AND,     /* 全部事件触发 */
    TOPIC_RULE_MANUAL   /* 手动触发 */
} topic_rule_type_t;

/* Topic规则结构 */
typedef struct {
    topic_rule_type_t type;          /* 规则类型 */
    obj_dict_key_t* events;          /* 事件列表 */
    size_t event_count;              /* 事件数量 */
    uint32_t* event_timeouts_ms;     /* 每个事件的超时时间（毫秒），若为NULL则使用默认值 */
#if TOPIC_BUS_ENABLE_ATOMICS
    atomic_uint_fast32_t trigger_mask;  /* 触发掩码（AND规则用，原子操作） */
#else
    uint32_t trigger_mask;           /* 触发掩码（AND规则用） */
#endif
#if TOPIC_BUS_ENABLE_RULE_CACHE
    /* 简单规则缓存：缓存最近一次匹配的event_key及结果 */
    obj_dict_key_t last_event_key_cached;
    uint8_t        last_can_trigger_cached; /* 1/0，表示最近一次can_trigger的结果 */
#endif
} topic_rule_t;

/* 检查规则是否可触发 */
int topic_rule_can_trigger(const topic_rule_t* rule, obj_dict_key_t event_key);

/* 更新触发掩码（用于AND规则） */
void topic_rule_update_mask(topic_rule_t* rule, obj_dict_key_t event_key, int triggered);

/* 重置触发掩码 */
void topic_rule_reset_mask(topic_rule_t* rule);

/* 检查规则是否匹配 */
int topic_rule_matches(const topic_rule_t* rule, obj_dict_key_t event_key);

/* 检查事件是否满足时效性要求
 * @param rule 规则指针
 * @param event_key 事件键
 * @param dict 对象字典（用于获取事件时间戳）
 * @return 1表示满足时效性，0表示超时或失败
 */
int topic_rule_check_timeout(const topic_rule_t* rule, obj_dict_key_t event_key, void* dict);

#ifdef __cplusplus
}
#endif

#endif /* TOPIC_RULE_H_ */

