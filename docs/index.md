# OpenIBBOs Wiki

欢迎来到 OpenIBBOs 文档中心！

## 🚀 关于 OpenIBBOs

OpenIBBOs 是一个专为嵌入式实时系统设计的**零拷贝通信框架**，基于对象字典的数据存储、Topic 总线的发布订阅机制和规则引擎，提供高效的事件驱动通信能力。

## ✨ 核心特性

### 🎯 零拷贝数据传递
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

## 📚 文档导航

### 快速开始
- [快速开始指南](getting-started.md) - 5分钟快速上手
- [安装指南](installation.md) - 详细的安装和配置步骤

### 核心概念
- [Topic 总线](zero_topic_core/topic_bus/topic_bus.md) - 发布订阅通信机制
- [对象字典](zero_topic_core/obj_dict/obj_dict.md) - 统一数据存储
- [环形缓冲区](zero_topic_core/ring_buffer/ring_buffer.md) - 高效缓冲区实现

### 架构设计
- [硬件线程 vs 直接API分析](zero_topic_core/topic_bus/hardware_thread_vs_direct_api_analysis.md)
- [MCU硬件优化分析](zero_topic_core/topic_bus/mcu_hardware_optimization_analysis.md)
- [多MCU microROS通信](zero_topic_core/topic_bus/multi_mcu_microros_communication.md)

## 🔗 相关资源

- [GitHub 仓库](https://github.com/yourusername/OpenIBBOs)
- [问题反馈](https://github.com/yourusername/OpenIBBOs/issues)
- [贡献指南](CONTRIBUTING.md)

## 📄 许可证

本项目采用 [MIT License](https://github.com/yourusername/OpenIBBOs/blob/main/LICENSE)。

---

**最后更新**: {{ git_revision_date_localized }}
