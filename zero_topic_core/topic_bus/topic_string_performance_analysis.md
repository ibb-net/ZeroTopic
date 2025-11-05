# Topic 字符串 vs 整数 ID 性能分析

## 概述

本文档分析如果将 topic 从整数 ID（`uint16_t`）改为字符串形式的性能损耗差异。

## 当前实现分析

### 当前实现（整数 ID）

```27:35:OpenIBBOs/zero_topic_core/topic_bus/topic_bus.c
static topic_entry_t* __find_topic(topic_bus_t* bus, uint16_t topic_id) {
    if (!bus || !bus->topics) return NULL;
    for (size_t i = 0; i < bus->max_topics; ++i) {
        if (bus->topics[i].topic_id == topic_id) {
            return &bus->topics[i];
        }
    }
    return NULL;
}
```

**特点：**
- Topic ID 为 `uint16_t`（2 字节）
- 查找使用简单的整数比较（1 个 CPU 周期）
- 时间复杂度：O(n)，n 为 Topic 数量
- 空间复杂度：每个 Topic 条目 2 字节

### 如果改为字符串实现

```c
// 假设改为字符串
typedef struct {
    const char* topic_name;  // 字符串指针
    // ... 其他字段
} topic_entry_t;

static topic_entry_t* __find_topic(topic_bus_t* bus, const char* topic_name) {
    if (!bus || !bus->topics || !topic_name) return NULL;
    for (size_t i = 0; i < bus->max_topics; ++i) {
        if (strcmp(bus->topics[i].topic_name, topic_name) == 0) {
            return &bus->topics[i];
        }
    }
    return NULL;
}
```

## 性能损耗分析

### 1. 查找性能对比

#### 1.1 时间复杂度

| 操作 | 整数 ID | 字符串 |
|------|---------|--------|
| 单次比较 | O(1) - 1 个 CPU 周期 | O(m) - m 为字符串长度 |
| 平均查找 | O(n/2) - n 为 Topic 数量 | O(n×m/2) |
| 最坏查找 | O(n) | O(n×m) |

**示例计算（64 个 Topic，平均字符串长度 20 字符）：**
- 整数 ID：平均 32 次比较 × 1 周期 = **32 周期**
- 字符串：平均 32 次比较 × 20 字符 = **640 周期**

**性能差异：约 20 倍**

#### 1.2 实际性能测试估算

假设典型场景：
- Topic 数量：64
- 平均字符串长度：20 字符
- CPU：ARM Cortex-M4 @ 168MHz

**整数 ID 查找：**
```
平均查找次数：32 次
每次比较：1 个周期
总周期：32 周期
时间：32 / 168MHz ≈ 0.19 us
```

**字符串查找：**
```
平均查找次数：32 次
每次 strcmp：平均 20 字符比较 = 20 周期
总周期：32 × 20 = 640 周期
时间：640 / 168MHz ≈ 3.81 us
```

**性能差异：约 20 倍（3.81 / 0.19）**

### 2. 内存开销对比

#### 2.1 存储空间

**整数 ID：**
```c
typedef struct {
    uint16_t topic_id;  // 2 字节
    // ...
} topic_entry_t;
```

**字符串：**
```c
typedef struct {
    const char* topic_name;  // 指针：4/8 字节（32/64位系统）
    // 加上字符串本身：平均 20 字节
    // 总计：24-28 字节
} topic_entry_t;
```

**内存开销对比：**

| 项目 | 整数 ID | 字符串 | 差异 |
|------|---------|--------|------|
| Topic 条目字段 | 2 字节 | 4-8 字节 | +2-6 字节 |
| 字符串存储 | 0 | 20 字节（平均） | +20 字节 |
| **总计** | **2 字节** | **24-28 字节** | **+12-14 倍** |

**示例：64 个 Topic**
- 整数 ID：64 × 2 = **128 字节**
- 字符串：64 × 24 = **1536 字节**
- **内存增加：12 倍**

#### 2.2 缓存友好性

**整数 ID：**
- 数据紧凑，缓存命中率高
- 一次缓存行（64 字节）可容纳 32 个 Topic ID
- 缓存友好

**字符串：**
- 字符串分散存储，缓存命中率低
- 字符串比较需要多次内存访问
- 缓存不友好

**缓存性能影响：**
- 整数 ID：L1 缓存命中率 > 95%
- 字符串：L1 缓存命中率 < 80%（估计）
- **性能进一步下降约 20%**

### 3. 字符串比较开销

#### 3.1 strcmp 函数开销

```c
// strcmp 实现（简化版）
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}
```

**每次比较的开销：**
- 字符加载：1 周期
- 字符比较：1 周期
- 指针递增：1 周期
- 分支判断：1 周期（如果分支预测失败）
- **平均每个字符：3-4 周期**

**20 字符字符串比较：**
- 最佳情况（前几个字符不同）：3-4 周期
- 平均情况（中间字符不同）：30-40 周期
- 最坏情况（完全相同）：60-80 周期

#### 3.2 优化方案对比

**方案 A：字符串哈希**
```c
uint32_t topic_hash(const char* name) {
    uint32_t hash = 0;
    while (*name) {
        hash = hash * 31 + *name++;
    }
    return hash;
}
```
- 优点：可将查找降为 O(n)
- 缺点：需要存储哈希值，仍有字符串比较开销
- 性能提升：约 5-10 倍（但仍不如整数 ID）

**方案 B：字符串池**
```c
// 使用字符串池，只比较指针
static const char* topic_pool[] = {
    "sensor_data",
    "motor_control",
    // ...
};
```
- 优点：查找时只需比较指针（O(1)）
- 缺点：需要预先定义所有 Topic 名称
- 性能：接近整数 ID，但灵活性降低

**方案 C：混合方案**
```c
typedef struct {
    uint16_t topic_id;      // 内部使用整数 ID
    const char* topic_name; // 外部使用字符串（仅用于调试/日志）
} topic_entry_t;
```
- 优点：内部高效，外部兼容字符串
- 缺点：需要维护 ID 到名称的映射表

### 4. 实际场景性能影响

#### 4.1 事件发布性能

**当前实现（整数 ID）：**
```c
int topic_publish_event(topic_bus_t* bus, obj_dict_key_t event_key) {
    // ... 遍历所有 Topic，检查规则匹配
    for (size_t i = 0; i < bus->max_topics; ++i) {
        topic_entry_t* entry = &bus->topics[i];
        if (entry->topic_id == 0xFFFF) continue;
        // 规则匹配：使用整数比较
        if (topic_rule_can_trigger(&entry->rule, event_key)) {
            // ...
        }
    }
}
```

**关键路径：**
- 遍历：O(n)
- 规则匹配：O(m)，m 为规则事件数
- **总开销：O(n×m)**

**如果改为字符串：**
- 每次查找都需要字符串比较
- 在规则匹配中也需要字符串比较
- **总开销：O(n×m×strlen)**

**性能影响估算：**
- 当前：64 Topic × 16 事件 × 1 周期 = 1024 周期 ≈ 6.1 us
- 字符串：64 Topic × 16 事件 × 20 周期 = 20480 周期 ≈ 121.9 us
- **性能下降：约 20 倍**

#### 4.2 订阅/取消订阅性能

**当前实现：**
```c
int topic_subscribe(topic_bus_t* bus, uint16_t topic_id, ...) {
    topic_entry_t* entry = __find_topic(bus, topic_id);
    // 查找：O(n)，平均 32 次整数比较
}
```

**字符串实现：**
```c
int topic_subscribe(topic_bus_t* bus, const char* topic_name, ...) {
    topic_entry_t* entry = __find_topic(bus, topic_name);
    // 查找：O(n×m)，平均 32 × 20 = 640 次字符比较
}
```

**性能影响：**
- 订阅操作不是高频操作（初始化时执行）
- 但字符串查找仍然慢 20 倍

#### 4.3 ISR 路径性能

**当前实现（ISR 安全）：**
```c
int topic_publish_isr(topic_bus_t* bus, obj_dict_key_t event_key) {
    // 仅入队，不查找
    topic_bus_isr_event_t evt = { .event_key = event_key };
    return ring_buffer_write_isr(bus->isr_queue, &evt);
}
```

**如果改为字符串：**
- ISR 中不能使用字符串比较（可能触发页错误）
- 需要在 ISR 中先进行字符串到 ID 的转换
- 或者完全禁用 ISR 路径

**影响：**
- ISR 路径性能不受影响（如果使用混合方案）
- 但如果完全使用字符串，ISR 路径可能不可用

### 5. 综合性能对比

#### 5.1 性能指标对比表

| 指标 | 整数 ID | 字符串 | 性能差异 |
|------|---------|--------|----------|
| **查找性能** | 0.19 us | 3.81 us | **20 倍** |
| **内存占用** | 128 字节 | 1536 字节 | **12 倍** |
| **缓存命中率** | > 95% | < 80% | **-15%** |
| **事件发布** | 6.1 us | 121.9 us | **20 倍** |
| **订阅操作** | 0.19 us | 3.81 us | **20 倍** |
| **代码复杂度** | 低 | 高 | - |
| **ISR 安全** | 是 | 否（纯字符串） | - |

#### 5.2 实际影响评估

**高频操作（事件发布）：**
- 当前：6.1 us
- 字符串：121.9 us
- **延迟增加：115.8 us**
- **影响：对于 100Hz 的事件发布，延迟增加可能导致实时性问题**

**低频操作（订阅）：**
- 当前：0.19 us
- 字符串：3.81 us
- **影响：可接受（仅在初始化时执行）**

**内存占用：**
- 当前：128 字节
- 字符串：1536 字节
- **影响：对于资源受限的 MCU，内存增加可能不可接受**

### 6. 优化建议

#### 6.1 混合方案（推荐）

**实现方式：**
```c
typedef struct {
    uint16_t topic_id;              // 内部使用整数 ID（高效）
    const char* topic_name;         // 外部使用字符串（兼容性）
    // ...
} topic_entry_t;

// 初始化时建立映射
int topic_bus_register(topic_bus_t* bus, const char* name, uint16_t* id) {
    // 查找或分配新的 topic_id
    // 存储字符串名称
}

// 发布时使用整数 ID（高效）
int topic_publish_event_by_id(topic_bus_t* bus, uint16_t topic_id, ...);

// 订阅时使用字符串（方便）
int topic_subscribe_by_name(topic_bus_t* bus, const char* name, ...);
```

**优点：**
- 内部使用整数 ID，保持高性能
- 外部 API 支持字符串，提供便利性
- 性能损失：仅在使用字符串 API 时有额外转换开销

#### 6.2 字符串哈希优化

**实现方式：**
```c
typedef struct {
    uint32_t topic_hash;            // 字符串哈希值
    const char* topic_name;          // 字符串名称
    // ...
} topic_entry_t;

// 查找时先比较哈希，再比较字符串
static topic_entry_t* __find_topic(topic_bus_t* bus, const char* name) {
    uint32_t hash = topic_hash(name);
    for (size_t i = 0; i < bus->max_topics; ++i) {
        if (bus->topics[i].topic_hash == hash) {
            // 哈希匹配，再比较字符串（避免哈希冲突）
            if (strcmp(bus->topics[i].topic_name, name) == 0) {
                return &bus->topics[i];
            }
        }
    }
    return NULL;
}
```

**性能提升：**
- 大部分情况下哈希不匹配，直接跳过
- 只有哈希匹配时才进行字符串比较
- **性能提升：约 5-10 倍（但仍不如整数 ID）**

#### 6.3 字符串池优化

**实现方式：**
```c
// 预定义所有 Topic 名称
static const char* TOPIC_NAME_POOL[] = {
    "sensor_data",
    "motor_control",
    // ...
};

// Topic 条目存储索引
typedef struct {
    uint16_t topic_name_idx;        // 字符串池索引
    // ...
} topic_entry_t;

// 查找时比较索引（O(1)）
static topic_entry_t* __find_topic(topic_bus_t* bus, const char* name) {
    uint16_t idx = topic_name_to_index(name);  // 需要维护映射表
    for (size_t i = 0; i < bus->max_topics; ++i) {
        if (bus->topics[i].topic_name_idx == idx) {
            return &bus->topics[i];
        }
    }
    return NULL;
}
```

**优点：**
- 查找性能接近整数 ID
- 内存占用较小（只需存储索引）

**缺点：**
- 需要预先定义所有 Topic
- 灵活性降低

### 7. 结论与建议

#### 7.1 性能损耗总结

**如果完全采用字符串形式：**
- **查找性能：下降约 20 倍**
- **内存占用：增加约 12 倍**
- **事件发布延迟：增加约 115.8 us**
- **缓存性能：下降约 15%**

#### 7.2 推荐方案

**方案 1：保持整数 ID（最佳性能）**
- 优点：性能最优，内存占用最小
- 缺点：需要维护 ID 到名称的映射（外部维护）

**方案 2：混合方案（推荐）**
- 内部使用整数 ID，保持高性能
- 外部 API 支持字符串，提供便利性
- 性能损失：仅在使用字符串 API 时有转换开销

**方案 3：字符串哈希（折中）**
- 如果必须使用字符串，使用哈希优化
- 性能提升约 5-10 倍，但仍不如整数 ID

#### 7.3 适用场景

**适合使用整数 ID：**
- 资源受限的 MCU
- 高频事件发布场景
- 实时性要求高的系统
- ISR 路径需要支持

**可以接受字符串：**
- 资源充足的系统
- 低频操作场景
- 需要动态 Topic 名称
- 不需要 ISR 支持

### 8. 性能测试建议

建议在实际硬件上测试以下场景：

1. **查找性能测试**
   - 测试不同 Topic 数量下的查找时间
   - 对比整数 ID 和字符串查找

2. **事件发布性能测试**
   - 测试高频事件发布的延迟
   - 验证是否满足实时性要求

3. **内存占用测试**
   - 测量实际内存占用
   - 验证是否在资源限制内

4. **缓存性能测试**
   - 使用性能分析工具测量缓存命中率
   - 评估缓存对性能的影响

## 参考资料

- [topic_bus 模块文档](./topic_bus.md)
- [topic_bus 实现代码](../topic_bus/topic_bus.c)
- [ARM Cortex-M4 性能参考](https://developer.arm.com/documentation/ddi0439/b/)

