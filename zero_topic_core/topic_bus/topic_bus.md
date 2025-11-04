# topic_bus 模块说明

## 概述

Topic总线是基于规则的事件发布订阅框架，用于替代队列多播方案。支持OR/AND/MANUAL三种规则类型，采用回调订阅机制，与对象字典深度集成，提供高效的事件驱动通信。

## 目录结构

- `topic_bus.h/.c`：核心实现
- `topic_rule.h/.c`：规则引擎实现
- `topic_bus_config.h`：编译期配置
- `perf_test_topic_bus.c`：功能与性能测试

## 设计理念

### 规则驱动
- **OR规则**：任一事件触发即可发布Topic
- **AND规则**：全部事件触发后才发布Topic
- **MANUAL规则**：需手动调用`topic_publish_manual`发布

### 回调订阅
- 使用回调函数替代队列机制，降低延迟和内存开销
- 支持多个订阅者同时订阅同一Topic

### 与对象字典集成
- 发布事件时自动从obj_dict获取最新数据
- 回调函数接收数据指针和长度，实现零拷贝访问

### ISR安全
- 支持ISR上下文发布事件（通过环形队列缓冲）
- 任务模式处理ISR队列中的事件

## 配置

```c
#define TOPIC_BUS_MAX_TOPICS                  64    // 最大Topic数量
#define TOPIC_BUS_MAX_SUBSCRIBERS_PER_TOPIC   16    // 每个Topic最大订阅者数
#define TOPIC_BUS_ISR_QUEUE_SIZE              32    // ISR队列容量
#define TOPIC_BUS_ENABLE_STATS                1     // 启用统计信息
#define TOPIC_BUS_ENABLE_ATOMICS              1     // 启用C11原子优化
#define TOPIC_BUS_ENABLE_RULES                1     // 启用规则支持
#define TOPIC_BUS_ENABLE_ISR                  1     // 启用ISR安全路径
#define TOPIC_BUS_MAX_RULE_EVENTS             16    // 每个规则最大事件数
```

## 核心API

### 初始化与管理

```c
int topic_bus_init(topic_bus_t* bus, topic_entry_t* entries, 
                   size_t max_topics, obj_dict_t* dict);
```

初始化Topic总线，需要提供Topic条目数组和关联的对象字典。

### 规则管理

```c
int topic_rule_create(topic_bus_t* bus, uint16_t topic_id, const topic_rule_t* rule);
```

创建Topic规则，定义触发条件和事件列表。

### 订阅管理

```c
int topic_subscribe(topic_bus_t* bus, uint16_t topic_id,
                    void (*callback)(uint16_t, const void*, size_t, void*),
                    void* user_data);

int topic_unsubscribe(topic_bus_t* bus, uint16_t topic_id,
                      void (*callback)(uint16_t, const void*, size_t, void*),
                      void* user_data);
```

订阅/取消订阅Topic，注册/移除回调函数。

### 事件发布

```c
int topic_publish_event(topic_bus_t* bus, obj_dict_key_t event_key);
int topic_publish_manual(topic_bus_t* bus, uint16_t topic_id);
#if TOPIC_BUS_ENABLE_ISR
int topic_publish_isr(topic_bus_t* bus, obj_dict_key_t event_key);
void topic_bus_process_isr_queue(topic_bus_t* bus);
#endif
```

发布事件（任务模式）、手动发布Topic、ISR安全发布、处理ISR队列。

## 使用示例

### 基础示例：OR规则

```c
/* 初始化对象字典 */
obj_dict_entry_t dict_entries[100];
obj_dict_t dict;
obj_dict_init(&dict, dict_entries, 100);

/* 初始化Topic总线 */
topic_entry_t topic_entries[16];
topic_bus_t bus;
topic_bus_init(&bus, topic_entries, 16, &dict);

/* 创建OR规则：任一事件触发 */
obj_dict_key_t events[] = {10, 20, 30};
topic_rule_t rule = {
    .type = TOPIC_RULE_OR,
    .events = events,
    .event_count = 3,
};
#if TOPIC_BUS_ENABLE_ATOMICS
    atomic_init(&rule.trigger_mask, 0);
#endif
topic_rule_create(&bus, 1, &rule);

/* 订阅Topic */
void my_callback(uint16_t topic_id, const void* data, size_t len, void* user) {
    printf("Topic %d triggered, data_len=%zu\n", topic_id, len);
}
topic_subscribe(&bus, 1, my_callback, NULL);

/* 发布事件触发Topic */
my_data_t data = {.value = 123};
obj_dict_set(&dict, 10, &data, sizeof(data), 0);
topic_publish_event(&bus, 10);  // 触发回调
```

### AND规则示例

```c
/* 创建AND规则：全部事件触发 */
obj_dict_key_t events[] = {40, 50};
topic_rule_t rule = {
    .type = TOPIC_RULE_AND,
    .events = events,
    .event_count = 2,
};
#if TOPIC_BUS_ENABLE_ATOMICS
    atomic_init(&rule.trigger_mask, 0);
#endif
topic_rule_create(&bus, 2, &rule);

/* 订阅 */
topic_subscribe(&bus, 2, my_callback, NULL);

/* 发布第一个事件（不触发） */
obj_dict_set(&dict, 40, &data, sizeof(data), 0);
topic_publish_event(&bus, 40);  // 不触发

/* 发布第二个事件（触发） */
obj_dict_set(&dict, 50, &data, sizeof(data), 0);
topic_publish_event(&bus, 50);  // 触发回调
```

### ISR安全发布

```c
/* 在ISR中发布事件 */
void gpio_isr_handler(void) {
    topic_publish_isr(&bus, 100);
}

/* 在任务中处理ISR队列 */
void task_process_events(void) {
    while (1) {
        topic_bus_process_isr_queue(&bus);
        os_delay_ms(10);
    }
}
```

### MANUAL规则示例

```c
/* 创建MANUAL规则 */
obj_dict_key_t events[] = {60};
topic_rule_t rule = {
    .type = TOPIC_RULE_MANUAL,
    .events = events,
    .event_count = 1,
};
#if TOPIC_BUS_ENABLE_ATOMICS
    atomic_init(&rule.trigger_mask, 0);
#endif
topic_rule_create(&bus, 3, &rule);
topic_subscribe(&bus, 3, my_callback, NULL);

/* 手动触发（事件发布不会触发） */
topic_publish_event(&bus, 60);  // 不触发
topic_publish_manual(&bus, 3);  // 手动触发
```

## 性能指标

### 设计目标
- **事件发布延迟**：< 10us（非阻塞模式）
- **规则匹配**：< 5us（OR规则），< 8us（AND规则）
- **ISR路径**：< 3us（入队）
- **回调执行**：取决于用户回调，框架开销 < 2us

### 优化特性
- **C11原子操作**：使用原子操作优化版本号和掩码管理
- **无锁设计**：SPSC场景下的无锁订阅列表遍历
- **零拷贝**：回调直接访问obj_dict中的数据，无需拷贝

## 测试

运行性能测试：
```c
int topic_bus_perf_test_main(void);
```

测试内容：
- 基础功能测试：OR/AND/MANUAL规则、订阅/发布
- 事件发布延迟测试
- 规则匹配性能测试

## 与obj_dict的关系

Topic总线与对象字典深度集成：
1. 发布事件时，先通过`obj_dict_set`更新数据
2. 规则匹配后，自动从obj_dict获取最新数据
3. 回调函数接收数据指针，实现零拷贝访问

## 与ring_buffer的关系

Topic总线使用ring_buffer实现ISR安全路径：
- ISR发布时入队到ring_buffer
- 任务模式从ring_buffer出队并处理

## 与microROS的关系

Topic总线可对接microROS：
- Topic ID映射到ROS Topic名称
- 回调机制适配ROS订阅者
- 对象字典支持ROS消息类型

## 内存模型

- 配置期静态分配：Topic条目数组、订阅者链表节点
- 线程安全：使用信号量保护关键路径
- 并发模型：SPSC/MPMC支持，ISR安全保证

## 注意事项

1. **规则事件数组**：需要在外部持久化存储events数组
2. **回调执行时间**：回调执行时间应尽量短，避免阻塞发布线程
3. **内存对齐**：数据应按照平台对齐要求对齐
4. **线程安全**：订阅/取消订阅需要加锁保护

## 扩展性

可通过配置开关灵活裁剪功能：
- 关闭统计信息减少内存开销
- 关闭ISR支持简化实现
- 关闭原子操作回退到锁保护

