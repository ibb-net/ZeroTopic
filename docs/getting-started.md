# 快速开始

本指南将帮助您在 5 分钟内开始使用 OpenIBBOs。

## 前置要求

- C 编译器（GCC/Clang）
- CMake 3.10+
- 支持的嵌入式平台（FreeRTOS、RT-Thread 等）

## 安装

### 1. 克隆仓库

```bash
git clone https://github.com/yourusername/OpenIBBOs.git
cd OpenIBBOs
```

### 2. 集成到项目

将 `zero_topic_core` 目录复制到您的项目中，或使用 Git 子模块：

```bash
git submodule add https://github.com/yourusername/OpenIBBOs.git
```

### 3. 配置 CMake

在您的 `CMakeLists.txt` 中添加：

```cmake
add_subdirectory(OpenIBBOs/zero_topic_core)
target_link_libraries(your_target PRIVATE zero_topic_core)
```

## 第一个示例

### 1. 初始化对象字典

```c
#include "obj_dict.h"
#include "topic_bus.h"

// 定义数据对象
typedef struct {
    float temperature;
    uint32_t timestamp;
} sensor_data_t;

sensor_data_t sensor_data = {0};

// 注册到对象字典
obj_dict_entry_t entry;
entry.index = 0x1000;
entry.type = OBJ_DICT_TYPE_FLOAT;
entry.data_ptr = &sensor_data.temperature;
obj_dict_register(&entry);
```

### 2. 创建 Topic

```c
// 定义 Topic
topic_id_t sensor_topic = TOPIC_ID_SENSOR_DATA;

// 创建 Topic
topic_bus_create_topic(sensor_topic, 0x1000, sizeof(sensor_data_t));
```

### 3. 订阅 Topic

```c
void sensor_callback(topic_id_t topic, void* data) {
    sensor_data_t* sensor = (sensor_data_t*)data;
    printf("Temperature: %.2f\n", sensor->temperature);
}

// 订阅 Topic
topic_bus_subscribe(sensor_topic, sensor_callback);
```

### 4. 发布事件

```c
// 更新数据
sensor_data.temperature = 25.5f;
sensor_data.timestamp = get_timestamp();

// 发布事件（零拷贝）
topic_bus_publish_event(sensor_topic, 0x1000);
```

## 下一步

- 阅读 [Topic 总线文档](zero_topic_core/topic_bus/topic_bus.md) 了解详细机制
- 查看 [对象字典文档](zero_topic_core/obj_dict/obj_dict.md) 学习数据管理
- 参考示例代码了解更高级用法

## 获取帮助

- 查看 [问题反馈](https://github.com/yourusername/OpenIBBOs/issues)
- 阅读 [开发文档](../README.md)
