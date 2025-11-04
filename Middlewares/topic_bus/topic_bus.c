#include <string.h>
#include "topic_bus.h"
#include "../../Rte/inc/os_heap.h"

/* ============================================================
 * 内部函数声明 (Internal Functions Declaration)
 * ============================================================ */

static topic_entry_t* __find_topic(topic_bus_t* bus, uint16_t topic_id);
static topic_entry_t* __find_free_topic_slot(topic_bus_t* bus);
static obj_dict_entry_t* __find_dict_entry(obj_dict_t* dict, obj_dict_key_t key);
static void __trigger_topic_callbacks(topic_entry_t* entry, obj_dict_key_t event_key, topic_bus_t* bus);

/* ============================================================
 * 函数实现 (Function Implementation)
 * ============================================================ */

/* ---------------- 辅助函数 ---------------- */

/*
 * @brief 查找指定topic_id的Topic条目
 * @param bus Topic总线指针
 * @param topic_id Topic ID
 * @return 返回Topic条目指针，否则返回NULL
 */
static topic_entry_t* __find_topic(topic_bus_t* bus, uint16_t topic_id) {
    if (!bus || !bus->topics) return NULL;
    for (size_t i = 0; i < bus->max_topics; ++i) {
        if (bus->topics[i].topic_id == topic_id) {
            return &bus->topics[i];
        }
    }
    return NULL;
}

/*
 * @brief 查找空闲Topic槽位
 * @param bus Topic总线指针
 * @return 返回空槽的Topic条目指针，若无空槽返回NULL
 */
static topic_entry_t* __find_free_topic_slot(topic_bus_t* bus) {
    if (!bus || !bus->topics) return NULL;
    for (size_t i = 0; i < bus->max_topics; ++i) {
        if (bus->topics[i].topic_id == 0xFFFF) {  /* 使用0xFFFF作为空槽标识 */
            return &bus->topics[i];
        }
    }
    return NULL;
}

/*
 * @brief 查找obj_dict_entry（内部使用）
 */
static obj_dict_entry_t* __find_dict_entry(obj_dict_t* dict, obj_dict_key_t key) {
    if (!dict || !dict->entries) return NULL;
    for (size_t i = 0; i < dict->max_keys; ++i) {
        if (dict->entries[i].value != NULL && dict->entries[i].key == key) {
            return &dict->entries[i];
        }
    }
    return NULL;
}

/*
 * @brief 触发Topic回调
 * @param entry Topic条目指针
 * @param event_key 触发的事件键（用于从obj_dict获取数据）
 * @param bus Topic总线指针
 */
static void __trigger_topic_callbacks(topic_entry_t* entry, obj_dict_key_t event_key, topic_bus_t* bus) {
    if (!entry) return;

    /* 从对象字典获取最新数据 */
    void* data = NULL;
    size_t data_len = 0;
    
    if (bus && bus->obj_dict) {
        obj_dict_entry_t* dict_entry = __find_dict_entry(bus->obj_dict, event_key);
        if (dict_entry && dict_entry->value) {
            data = dict_entry->value;
            data_len = dict_entry->value_len;
        }
    }

    /* 触发所有订阅者 */
    if (entry->subscribers) {
        topic_subscription_t* sub = entry->subscribers;
        while (sub) {
            if (sub->callback) {
                sub->callback(entry->topic_id, data, data_len, sub->user_data);
            }
            sub = sub->next;
        }
    }

#if TOPIC_BUS_ENABLE_ROUTER
    /* 触发Router处理 */
    if (bus && bus->router && data && data_len > 0) {
        topic_router_route(bus->router, entry->topic_id, data, data_len);
    }
#endif
}

/* ---------------- 初始化与销毁 ---------------- */

/*
 * @brief 初始化Topic总线
 * @param bus Topic总线指针
 * @param topics Topic条目数组
 * @param max_topics 最大Topic数量
 * @param dict 关联的对象字典
 * @return 0成功，-1失败
 */
int topic_bus_init(topic_bus_t* bus, topic_entry_t* topics, size_t max_topics, obj_dict_t* dict) {
    if (!bus || !topics || max_topics == 0 || !dict) return -1;

    bus->topics = topics;
    bus->max_topics = max_topics;
    bus->obj_dict = dict;

    /* 初始化Topic数组 */
    memset(topics, 0, sizeof(topic_entry_t) * max_topics);
    for (size_t i = 0; i < max_topics; ++i) {
        topics[i].topic_id = 0xFFFF;  /* 标记为空 */
#if TOPIC_BUS_ENABLE_STATS
#if TOPIC_BUS_ENABLE_ATOMICS
        atomic_init(&topics[i].event_count, 0);
#else
        topics[i].event_count = 0;
#endif
#endif
    }

#if TOPIC_BUS_ENABLE_ROUTER
    bus->router = NULL;  /* 初始化Router为NULL */
#endif

    /* 创建锁 */
    bus->lock = os_semaphore_create(1, "topic_bus_lock");
    if (!bus->lock) return -1;

#if TOPIC_BUS_ENABLE_ISR
    /* 创建ISR队列 */
    bus->isr_queue = ring_buffer_create(TOPIC_BUS_ISR_QUEUE_SIZE, sizeof(topic_bus_isr_event_t));
    if (!bus->isr_queue) {
        os_semaphore_destroy(bus->lock);
        return -1;
    }
#endif

    return 0;
}

/* ---------------- 规则管理 ---------------- */

/*
 * @brief 创建Topic规则
 * @param bus Topic总线指针
 * @param topic_id Topic ID
 * @param rule 规则指针
 * @return 0成功，-1失败
 */
int topic_rule_create(topic_bus_t* bus, uint16_t topic_id, const topic_rule_t* rule) {
    if (!bus || !rule) return -1;
    if (os_semaphore_take(bus->lock, 100) < 0) return -1;

    topic_entry_t* entry = __find_topic(bus, topic_id);
    if (!entry) {
        entry = __find_free_topic_slot(bus);
        if (!entry) {
            os_semaphore_give(bus->lock);
            return -1;
        }
        entry->topic_id = topic_id;
    }

    /* 释放旧的事件数组和超时数组（如果存在） */
    if (entry->rule.events) {
        os_free(entry->rule.events);
        entry->rule.events = NULL;
    }
    if (entry->rule.event_timeouts_ms) {
        os_free(entry->rule.event_timeouts_ms);
        entry->rule.event_timeouts_ms = NULL;
    }

    /* 复制规则基本字段 */
    entry->rule.type = rule->type;
    entry->rule.event_count = rule->event_count;
    
    /* 初始化trigger_mask（确保原子类型正确初始化） */
#if TOPIC_BUS_ENABLE_ATOMICS
    atomic_init(&entry->rule.trigger_mask, 0);
#else
    entry->rule.trigger_mask = 0;
#endif
    topic_rule_reset_mask(&entry->rule);

    /* 复制事件数组 */
    if (rule->events && rule->event_count > 0) {
        entry->rule.events = (obj_dict_key_t*)os_malloc(sizeof(obj_dict_key_t) * rule->event_count);
        if (!entry->rule.events) {
            os_semaphore_give(bus->lock);
            return -1;
        }
        memcpy(entry->rule.events, rule->events, sizeof(obj_dict_key_t) * rule->event_count);
    } else {
        entry->rule.events = NULL;
    }

    /* 复制超时数组 */
    if (rule->event_timeouts_ms && rule->event_count > 0) {
        entry->rule.event_timeouts_ms = (uint32_t*)os_malloc(sizeof(uint32_t) * rule->event_count);
        if (!entry->rule.event_timeouts_ms) {
            if (entry->rule.events) {
                os_free(entry->rule.events);
                entry->rule.events = NULL;
            }
            os_semaphore_give(bus->lock);
            return -1;
        }
        memcpy(entry->rule.event_timeouts_ms, rule->event_timeouts_ms, sizeof(uint32_t) * rule->event_count);
    } else {
        entry->rule.event_timeouts_ms = NULL;
    }

    os_semaphore_give(bus->lock);
    return 0;
}

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
                    void* user_data) {
    if (!bus || !callback) return -1;
    if (os_semaphore_take(bus->lock, 100) < 0) return -1;

    topic_entry_t* entry = __find_topic(bus, topic_id);
    if (!entry) {
        os_semaphore_give(bus->lock);
        return -1;
    }

    /* 分配订阅者节点 */
    topic_subscription_t* sub = (topic_subscription_t*)os_malloc(sizeof(topic_subscription_t));
    if (!sub) {
        os_semaphore_give(bus->lock);
        return -1;
    }

    sub->callback = callback;
    sub->user_data = user_data;
    sub->next = entry->subscribers;
    entry->subscribers = sub;

    os_semaphore_give(bus->lock);
    return 0;
}

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
                      void* user_data) {
    if (!bus || !callback) return -1;
    if (os_semaphore_take(bus->lock, 100) < 0) return -1;

    topic_entry_t* entry = __find_topic(bus, topic_id);
    if (!entry) {
        os_semaphore_give(bus->lock);
        return -1;
    }

    /* 从链表中删除订阅者 */
    topic_subscription_t* prev = NULL;
    topic_subscription_t* curr = entry->subscribers;
    while (curr) {
        if (curr->callback == callback && curr->user_data == user_data) {
            if (prev) {
                prev->next = curr->next;
            } else {
                entry->subscribers = curr->next;
            }
            os_free(curr);
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    os_semaphore_give(bus->lock);
    return (curr != NULL) ? 0 : -1;
}

/* ---------------- 事件发布 ---------------- */

/*
 * @brief 发布事件（任务模式）
 * @param bus Topic总线指针
 * @param event_key 事件键
 * @return 0成功，-1失败
 */
int topic_publish_event(topic_bus_t* bus, obj_dict_key_t event_key) {
    if (!bus) return -1;
    if (os_semaphore_take(bus->lock, 100) < 0) return -1;

    int triggered_count = 0;

    /* 遍历所有Topic，检查是否匹配 */
    for (size_t i = 0; i < bus->max_topics; ++i) {
        topic_entry_t* entry = &bus->topics[i];
        if (entry->topic_id == 0xFFFF) continue;  /* 空槽跳过 */
        
        /* 检查规则是否有效 */
        if (!entry->rule.events || entry->rule.event_count == 0) continue;  /* 规则无效，跳过 */

        /* 检查规则是否匹配 */
        if (topic_rule_can_trigger(&entry->rule, event_key)) {
            /* 检查时效性 */
            if (!topic_rule_check_timeout(&entry->rule, event_key, bus->obj_dict)) {
                continue;  /* 时效性不满足，跳过该Topic */
            }
            
            if (entry->rule.type == TOPIC_RULE_AND) {
                /* AND规则：更新掩码并检查是否全部触发 */
                topic_rule_update_mask(&entry->rule, event_key, 1);
                if (topic_rule_matches(&entry->rule, event_key)) {
                    /* 检查所有事件的时效性 */
                    int all_timeout_ok = 1;
                    for (size_t i = 0; i < entry->rule.event_count; ++i) {
                        if (!topic_rule_check_timeout(&entry->rule, entry->rule.events[i], bus->obj_dict)) {
                            all_timeout_ok = 0;
                            break;
                        }
                    }
                    
                    if (all_timeout_ok) {
                        /* 所有事件都在时效内，触发Topic回调 */
                        __trigger_topic_callbacks(entry, event_key, bus);
                        triggered_count++;
#if TOPIC_BUS_ENABLE_STATS
#if TOPIC_BUS_ENABLE_ATOMICS
                        atomic_fetch_add_explicit(&entry->event_count, 1, memory_order_relaxed);
#else
                        entry->event_count++;
#endif
#endif
                        /* 重置掩码，准备下次触发 */
                        topic_rule_reset_mask(&entry->rule);
                    } else {
                        /* 有事件超时，不触发，重置掩码 */
                        topic_rule_reset_mask(&entry->rule);
                    }
                }
            } else if (entry->rule.type == TOPIC_RULE_OR) {
                /* OR规则：直接触发 */
                __trigger_topic_callbacks(entry, event_key, bus);
                triggered_count++;
#if TOPIC_BUS_ENABLE_STATS
#if TOPIC_BUS_ENABLE_ATOMICS
                atomic_fetch_add_explicit(&entry->event_count, 1, memory_order_relaxed);
#else
                entry->event_count++;
#endif
#endif
            }
        }
    }

    os_semaphore_give(bus->lock);
    return 0;
}

/*
 * @brief 手动发布Topic
 * @param bus Topic总线指针
 * @param topic_id Topic ID
 * @return 0成功，-1失败
 */
int topic_publish_manual(topic_bus_t* bus, uint16_t topic_id) {
    if (!bus) return -1;
    if (os_semaphore_take(bus->lock, 100) < 0) return -1;

    topic_entry_t* entry = __find_topic(bus, topic_id);
    if (!entry) {
        os_semaphore_give(bus->lock);
        return -1;
    }

    /* 检查是否为手动触发类型 */
    if (entry->rule.type == TOPIC_RULE_MANUAL) {
        /* 手动触发时，使用规则的第一个事件（如果有） */
        obj_dict_key_t first_event = (entry->rule.events && entry->rule.event_count > 0) 
                                     ? entry->rule.events[0] : 0;
        __trigger_topic_callbacks(entry, first_event, bus);
#if TOPIC_BUS_ENABLE_STATS
#if TOPIC_BUS_ENABLE_ATOMICS
        atomic_fetch_add_explicit(&entry->event_count, 1, memory_order_relaxed);
#else
        entry->event_count++;
#endif
#endif
    }

    os_semaphore_give(bus->lock);
    return 0;
}

#if TOPIC_BUS_ENABLE_ISR
/*
 * @brief ISR安全发布事件
 * @param bus Topic总线指针
 * @param event_key 事件键
 * @return 0成功，-1失败
 */
int topic_publish_isr(topic_bus_t* bus, obj_dict_key_t event_key) {
    if (!bus || !bus->isr_queue) return -1;

    topic_bus_isr_event_t evt = { .event_key = event_key };
    if (ring_buffer_write_isr(bus->isr_queue, &evt) < 0) {
        return -1;
    }

    return 0;
}
#endif

/* ---------------- 查询与管理 ---------------- */

#if TOPIC_BUS_ENABLE_ISR
/*
 * @brief 处理ISR队列中的事件（任务模式调用）
 * @param bus Topic总线指针
 */
void topic_bus_process_isr_queue(topic_bus_t* bus) {
    if (!bus || !bus->isr_queue) return;

    topic_bus_isr_event_t evt;
    while (ring_buffer_read(bus->isr_queue, &evt) == 0) {
        /* 在任务上下文中处理事件 */
        (void)topic_publish_event(bus, evt.event_key);
    }
}
#endif

#if TOPIC_BUS_ENABLE_STATS
/*
 * @brief 获取Topic事件计数
 * @param bus Topic总线指针
 * @param topic_id Topic ID
 * @return 事件计数，失败返回-1
 */
int64_t topic_bus_get_event_count(topic_bus_t* bus, uint16_t topic_id) {
    if (!bus) return -1;
    if (os_semaphore_take(bus->lock, 100) < 0) return -1;

    topic_entry_t* entry = __find_topic(bus, topic_id);
    int64_t count = -1;
    if (entry) {
#if TOPIC_BUS_ENABLE_ATOMICS
        count = (int64_t)atomic_load_explicit(&entry->event_count, memory_order_acquire);
#else
        count = entry->event_count;
#endif
    }

    os_semaphore_give(bus->lock);
    return count;
}
#endif

#if TOPIC_BUS_ENABLE_ROUTER
/*
 * @brief 设置Topic Router
 * @param bus Topic总线指针
 * @param router Router指针
 * @return 0成功，-1失败
 */
int topic_bus_set_router(topic_bus_t* bus, topic_router_t* router) {
    if (!bus) return -1;
    if (os_semaphore_take(bus->lock, 100) < 0) return -1;
    
    bus->router = router;
    
    os_semaphore_give(bus->lock);
    return 0;
}
#endif

