#include "topic_rule.h"
#include "../obj_dict/obj_dict.h"
#include "../../Rte/inc/os_timestamp.h"
#include <string.h>

/*
 * @brief 检查规则是否可触发
 * @param rule 规则指针
 * @param event_key 事件键
 * @return 1表示可触发，0表示否
 */
int topic_rule_can_trigger(const topic_rule_t* rule, obj_dict_key_t event_key) {
    if (!rule || !rule->events) return 0;

#if TOPIC_BUS_ENABLE_RULE_CACHE
    /* 命中最近一次查询缓存可快速返回（高频重复事件） */
    if (rule->last_event_key_cached == event_key) {
        return rule->last_can_trigger_cached ? 1 : 0;
    }
#endif

    switch (rule->type) {
        case TOPIC_RULE_OR:
            /* OR规则：任意事件匹配即可触发 */
        {
            int matched = 0;
            for (size_t i = 0; i < rule->event_count; ++i) {
                if (rule->events[i] == event_key) {
                    matched = 1;
                    break;
                }
            }
#if TOPIC_BUS_ENABLE_RULE_CACHE
            ((topic_rule_t*)rule)->last_event_key_cached = event_key;
            ((topic_rule_t*)rule)->last_can_trigger_cached = (uint8_t)matched;
#endif
            return matched;
        }

        case TOPIC_RULE_AND:
            /* AND规则：需要检查触发掩码，在publish中处理 */
        {
            int matched = 0;
            for (size_t i = 0; i < rule->event_count; ++i) {
                if (rule->events[i] == event_key) {
                    matched = 1;
                    break;
                }
            }
#if TOPIC_BUS_ENABLE_RULE_CACHE
            ((topic_rule_t*)rule)->last_event_key_cached = event_key;
            ((topic_rule_t*)rule)->last_can_trigger_cached = (uint8_t)matched;
#endif
            return matched;
        }

        case TOPIC_RULE_MANUAL:
            /* 手动触发：不响应事件 */
            return 0;

        default:
            return 0;
    }
}

/*
 * @brief 更新触发掩码（用于AND规则）
 * @param rule 规则指针
 * @param event_key 触发的事件键
 * @param triggered 是否触发（1=触发，0=重置）
 */
void topic_rule_update_mask(topic_rule_t* rule, obj_dict_key_t event_key, int triggered) {
    if (!rule || !rule->events) return;

    /* 查找事件在列表中的位置 */
    for (size_t i = 0; i < rule->event_count && i < 32; ++i) {
        if (rule->events[i] == event_key) {
            uint32_t bit_mask = 1U << i;
#if TOPIC_BUS_ENABLE_ATOMICS
            if (triggered) {
                atomic_fetch_or_explicit(&rule->trigger_mask, bit_mask, memory_order_acq_rel);
            } else {
                atomic_fetch_and_explicit(&rule->trigger_mask, ~bit_mask, memory_order_acq_rel);
            }
#else
            if (triggered) {
                rule->trigger_mask |= bit_mask;
            } else {
                rule->trigger_mask &= ~bit_mask;
            }
#endif
            break;
        }
    }
}

/*
 * @brief 重置触发掩码
 * @param rule 规则指针
 */
void topic_rule_reset_mask(topic_rule_t* rule) {
    if (!rule) return;
#if TOPIC_BUS_ENABLE_ATOMICS
    atomic_store_explicit(&rule->trigger_mask, 0, memory_order_release);
#else
    rule->trigger_mask = 0;
#endif
}

/*
 * @brief 检查规则是否完全匹配（用于AND规则）
 * @param rule 规则指针
 * @param event_key 事件键
 * @return 1表示规则完全匹配可触发，0表示否
 */
int topic_rule_matches(const topic_rule_t* rule, obj_dict_key_t event_key) {
    if (!rule || !rule->events) return 0;

    switch (rule->type) {
        case TOPIC_RULE_OR:
            /* OR规则：任意匹配即可 */
            return topic_rule_can_trigger(rule, event_key);

        case TOPIC_RULE_AND: {
            /* AND规则：需要检查是否所有位都已置位 */
            uint32_t full_mask = (1U << rule->event_count) - 1;
#if TOPIC_BUS_ENABLE_ATOMICS
            uint32_t current_mask = atomic_load_explicit(&rule->trigger_mask, memory_order_acquire);
#else
            uint32_t current_mask = rule->trigger_mask;
#endif
            return (current_mask == full_mask);
        }

        case TOPIC_RULE_MANUAL:
            /* 手动触发：不自动匹配 */
            return 0;

        default:
            return 0;
    }
}

/*
 * @brief 检查事件是否满足时效性要求
 * @param rule 规则指针
 * @param event_key 事件键
 * @param dict 对象字典（用于获取事件时间戳）
 * @return 1表示满足时效性，0表示超时或失败
 */
int topic_rule_check_timeout(const topic_rule_t* rule, obj_dict_key_t event_key, void* dict) {
    if (!rule || !dict) return 0;
    
    obj_dict_t* obj_dict = (obj_dict_t*)dict;
    
    /* 查找事件在列表中的位置 */
    size_t event_index = SIZE_MAX;
    for (size_t i = 0; i < rule->event_count; ++i) {
        if (rule->events[i] == event_key) {
            event_index = i;
            break;
        }
    }
    
    if (event_index == SIZE_MAX) return 0;
    
    /* 获取该事件的超时配置 */
    uint32_t timeout_ms = TOPIC_BUS_DEFAULT_EVENT_TIMEOUT_MS;
    if (rule->event_timeouts_ms && event_index < rule->event_count) {
        timeout_ms = rule->event_timeouts_ms[event_index];
    }
    
    /* 如果为最大值，表示不判断时效性 */
    if (timeout_ms >= TOPIC_BUS_EVENT_TIMEOUT_MAX) {
        return 1;
    }
    
    /* 从对象字典获取事件时间戳 */
    uint64_t event_ts_us = 0;
    if (obj_dict_get(obj_dict, event_key, NULL, 0, &event_ts_us, NULL, NULL) < 0) {
        return 0;  /* 事件不存在 */
    }
    
    /* 检查时效性 */
    uint64_t now_us = os_monotonic_time_get_microsecond();
    uint64_t elapsed_us = (now_us >= event_ts_us) ? (now_us - event_ts_us) : 0;
    uint64_t timeout_us = (uint64_t)timeout_ms * 1000ULL;
    
    return (elapsed_us <= timeout_us) ? 1 : 0;
}


