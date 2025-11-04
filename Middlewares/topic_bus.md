# topic_bus 模块说明（规划）

## 概述
- 基于规则的 Topic 事件框架：Topic 可由单一 event 或组合规则(OR/AND/手动)定义。
- 采用回调订阅机制，替代任务队列订阅；与 `obj_dict` 集成以提供触发时的最新值访问。

## 规则模型
```c
typedef enum { TOPIC_RULE_OR, TOPIC_RULE_AND, TOPIC_RULE_MANUAL } topic_rule_type_t;

typedef struct {
    topic_rule_type_t type;
    obj_dict_key_t*   events;
    size_t            event_count;
    uint32_t          trigger_mask; // AND 用
} topic_rule_t;
```

## 预期 API（实现中）
```c
int topic_bus_init(void);
int topic_rule_create(uint16_t topic_id, const topic_rule_t* rule);
int topic_subscribe(uint16_t topic_id,
                    void (*cb)(uint16_t topic_id, const void* user),
                    void* user);
int topic_unsubscribe(uint16_t topic_id, void (*cb)(uint16_t, const void*), void* user);
int topic_publish_event(obj_dict_key_t key);   // 事件发布触发相关Topic
int topic_publish_manual(uint16_t topic_id);   // 手动发布
```

## 与 VFB 的关系
- 作为新一代发布订阅核心，逐步替换队列多播方案；兼容 ISR 安全路径（内部可复用 ring_buffer）。

## 与 microROS 的关系
- 与 `obj_dict` 协同可实现微ROS消息桥接、聚合触发等高级用例。


