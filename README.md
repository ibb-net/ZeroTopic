# ZeroTopic

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![GitHub stars](https://img.shields.io/github/stars/yourusername/ZeroTopic?style=social)](https://github.com/yourusername/ZeroTopic)

> **Zero-Copy Communication Framework with Topic Bus, Rules Engine, and Service/Action Architecture**

ZeroTopic 是一个专为嵌入式实时系统设计的零拷贝通信框架，基于对象字典的数据存储、Topic 总线的发布订阅机制和规则引擎，提供高效的事件驱动通信能力。

## ✨ 核心特性

### 🚀 零拷贝数据传递
- **对象字典统一存储**：所有数据集中存储在对象字典中，多订阅者共享同一份数据
- **指针传递**：订阅者回调直接接收数据指针，无需数据拷贝
- **内存高效**：单份数据，多订阅者共享，大幅降低内存占用

### 📡 Topic 总线架构
- **发布订阅模式**：基于 Topic ID 的松耦合通信
- **规则引擎**：支持 OR/AND/MANUAL 三种规则类型
- **时效性检查**：可配置事件超时时间，确保数据时效性
- **ISR 安全**：支持中断上下文发布事件

### 🔧 规则处理机制
- **OR 规则**：任一事件触发即可发布 Topic
- **AND 规则**：全部事件触发后才发布 Topic
- **MANUAL 规则**：手动触发 Topic 发布
- **超时控制**：支持事件时效性检查

### 🛠️ 服务化架构（规划中）
- **Service/Action 支持**：将传统 API 调用改为服务/动作调用
- **多协议支持**：MQTT、microROS 等通信方式
- **统一接口**：提供统一的通信接口抽象

## 📋 目录结构

```
ZeroTopic/
├── docs/                    # 文档目录
│   ├── architecture.md     # 架构设计文档
│   ├── api_reference.md     # API 参考文档
│   └── examples/            # 示例代码
├── src/                     # 源代码
│   ├── obj_dict/            # 对象字典模块
│   ├── topic_bus/           # Topic 总线模块
│   ├── topic_rule/          # 规则引擎模块
│   └── topic_router/        # 路由模块
├── tests/                   # 测试代码
├── examples/                # 示例项目
├── LICENSE                  # MIT 许可证
├── README.md                # 项目说明
├── CONTRIBUTING.md          # 贡献指南
└── CHANGELOG.md             # 变更日志
```

## 🚀 快速开始

### 1. 初始化对象字典

```c
#include "obj_dict.h"

#define MAX_KEYS 128
obj_dict_entry_t dict_entries[MAX_KEYS];
obj_dict_t dict;

obj_dict_init(&dict, dict_entries, MAX_KEYS);
```

### 2. 初始化 Topic 总线

```c
#include "topic_bus.h"

#define MAX_TOPICS 16
topic_entry_t topic_entries[MAX_TOPICS];
topic_bus_t bus;

topic_bus_init(&bus, topic_entries, MAX_TOPICS, &dict);
```

### 3. 创建规则并订阅

```c
// 创建 OR 规则：任一事件触发
obj_dict_key_t events[] = {10, 20, 30};
topic_rule_t rule = {
    .type = TOPIC_RULE_OR,
    .events = events,
    .event_count = 3,
};
topic_rule_create(&bus, 1, &rule);

// 订阅 Topic
void my_callback(uint16_t topic_id, const void* data, size_t len, void* user) {
    printf("Topic %d triggered, data_len=%zu\n", topic_id, len);
}
topic_subscribe(&bus, 1, my_callback, NULL);
```

### 4. 发布事件

```c
// 更新数据到对象字典
my_data_t data = {.value = 123};
obj_dict_set(&dict, 10, &data, sizeof(data), 0);

// 发布事件触发 Topic
topic_publish_event(&bus, 10);  // 触发回调
```

## 📖 核心概念

### 对象字典 (Object Dictionary)

对象字典是 ZeroTopic 的核心数据存储机制，提供：
- **统一数据管理**：所有数据通过 Key 统一管理
- **版本号机制**：原子版本号保证数据一致性
- **时间戳追踪**：自动记录数据更新时间
- **零拷贝访问**：直接通过指针访问数据

### Topic 总线 (Topic Bus)

Topic 总线提供发布订阅机制：
- **Topic ID**：每个 Topic 有唯一 ID
- **规则匹配**：基于规则引擎自动匹配事件
- **多订阅者**：支持多个订阅者同时订阅同一 Topic
- **回调机制**：事件触发时调用订阅者回调

### 规则引擎 (Rules Engine)

规则引擎支持多种触发模式：
- **OR 规则**：任一事件触发即发布
- **AND 规则**：全部事件触发才发布
- **MANUAL 规则**：手动触发发布
- **时效性检查**：可配置事件超时时间

## 🎯 使用场景

### 嵌入式实时系统
- **传感器数据采集**：多传感器数据通过 Topic 总线分发
- **控制信号处理**：基于规则引擎的复杂控制逻辑
- **事件驱动架构**：松耦合的模块间通信

### 分布式系统
- **多节点通信**：通过 MQTT/microROS 实现跨节点通信
- **服务化架构**：将功能模块封装为 Service/Action
- **统一接口**：提供统一的通信接口抽象

## 📊 性能指标

- **事件发布延迟**：< 10μs（非阻塞模式）
- **规则匹配**：< 5μs（OR 规则），< 8μs（AND 规则）
- **ISR 路径**：< 3μs（入队）
- **回调执行**：框架开销 < 2μs

## 🔗 相关资源

- [架构设计文档](docs/architecture.md)
- [API 参考文档](docs/api_reference.md)
- [示例代码](examples/)
- [贡献指南](CONTRIBUTING.md)
- [变更日志](CHANGELOG.md)

## 🤝 贡献

欢迎贡献代码、报告问题或提出建议！请查看 [CONTRIBUTING.md](CONTRIBUTING.md) 了解如何参与。

## 📄 许可证

本项目采用 [MIT 许可证](LICENSE)。

## 👥 作者

- 开发团队

## 🙏 致谢

感谢所有为项目做出贡献的开发者！

---

**ZeroTopic** - 零拷贝通信框架，让嵌入式系统通信更高效！

