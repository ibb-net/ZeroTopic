# 多个MCU运行microROS通信方案

## 概述

当多个MCU都运行microROS时，它们可以通过多种方式进行通信。本文档介绍常见的通信架构和实现方案。

## MCU内部线程间通信

### 问题：可以使用microROS做MCU内部各个thread间的通信吗？

**简短回答：技术上可以，但不推荐。**

### 详细分析

#### 1. 技术可行性

microROS **理论上**可以用于MCU内部线程间通信，但需要满足以下条件：

- **需要自定义传输层**：microROS默认需要通过UDP/TCP/串口连接到ROS2 Agent。要实现MCU内部通信，需要实现一个"内存传输层"或"虚拟传输层"，将ROS2消息直接传递给同一MCU内的其他线程。
- **需要Agent或自定义DDS实现**：ROS2使用DDS进行服务发现和消息路由。在MCU内部，要么：
  - 运行一个轻量级Agent（内存中）
  - 实现简化版的DDS发现机制
  - 使用静态配置绕过服务发现

#### 2. 为什么不推荐？

**性能问题：**
- **开销大**：microROS的消息序列化/反序列化、DDS协议栈、QoS管理等开销对于MCU内部通信来说过大
- **延迟高**：即使使用内存传输，ROS2的消息处理流程仍然比直接的消息队列慢得多
- **内存占用**：microROS需要较多内存（通常>50KB），在资源受限的MCU上可能造成问题

**复杂度问题：**
- **实现复杂**：需要实现自定义传输层，相当于重新实现microROS的核心部分
- **调试困难**：内部通信使用ROS2工具链调试不如直接的消息队列直观
- **维护成本高**：增加了系统复杂度，维护成本上升

**设计理念不符：**
- microROS设计目标是**跨设备通信**和与ROS2生态系统集成
- MCU内部线程通信属于**进程内通信**（IPC），更适合使用轻量级机制

#### 3. 推荐的替代方案

**方案A：使用现有的VFB系统（推荐）**

您的项目已经实现了VFB（Virtual Function Bus）系统，这是**专门为MCU内部线程通信设计的**：

```c
// VFB的优势：
// 1. 轻量级，开销小
// 2. 基于FreeRTOS队列，高效可靠
// 3. 支持事件驱动和发布订阅模式
// 4. 已经在项目中使用，成熟稳定

// 示例：线程A发布事件
vfb_send(SensorDataEvent, sensor_value, data_buffer, data_len);

// 线程B订阅并接收
void thread_b_callback(void *msg) {
    vfb_message_t msg = (vfb_message_t)msg;
    // 处理消息...
}
```

**方案B：使用FreeRTOS队列**

对于简单的点对点通信，直接使用FreeRTOS队列：

```c
// 创建队列
QueueHandle_t xQueue = xQueueCreate(10, sizeof(MyMessage));

// 线程A发送
MyMessage msg;
xQueueSend(xQueue, &msg, portMAX_DELAY);

// 线程B接收
MyMessage received_msg;
xQueueReceive(xQueue, &received_msg, portMAX_DELAY);
```

**方案C：混合架构（推荐用于需要外部通信的场景）**

- **MCU内部**：使用VFB或FreeRTOS队列（高效、低延迟）
- **跨MCU通信**：使用microROS（标准、可扩展）
- **桥接层**：在VFB和microROS之间建立桥接

```
┌─────────────────────────────────┐
│ MCU内部                          │
│  ┌──────────┐   ┌────────────┐  │
│  │  VFB     │◄─►│ microROS   │  │
│  │(线程间)  │   │(跨设备)    │  │
│  └──────────┘   └──────┬─────┘  │
└─────────────────────────┼────────┘
                          │
                    ROS2 Network
```

这种架构既保证了内部通信的高效性，又提供了标准的外部通信接口。

#### 4. 如果确实需要使用microROS内部通信

如果您的场景确实需要（例如：需要与ROS2生态系统完全兼容、需要动态服务发现等），可以考虑以下实现：

**实现思路：**

```c
// 1. 实现内存传输层
typedef struct {
    uint8_t* buffer;
    size_t size;
} memory_transport_t;

// 2. 创建内存中的Agent或简化DDS实现
// 3. 配置microROS使用内存传输层
rmw_uros_options_t options;
options.transport = RMW_UXRCE_TRANSPORT_MEMORY;  // 假设支持
options.memory_buffer = &mem_buffer;

// 4. 在单个MCU内运行多个microROS节点
rclc_node_init_default(&node1, "internal_node1", "", &context);
rclc_node_init_default(&node2, "internal_node2", "", &context);
```

**注意事项：**
- 需要修改microROS源码或实现自定义传输层
- 性能仍然不如直接的消息队列
- 可能需要更多内存和CPU资源

### 总结

| 通信场景 | 推荐方案 | 原因 |
|---------|---------|------|
| MCU内部线程通信 | **VFB系统**或**FreeRTOS队列** | 高效、低延迟、低开销 |
| 跨MCU通信 | **microROS** | 标准、可扩展、与ROS2兼容 |
| 混合场景 | **VFB + microROS桥接** | 兼顾内部效率和外部兼容性 |

**结论：** 对于MCU内部线程通信，优先使用VFB或FreeRTOS队列；microROS更适合跨设备通信场景。

## 通信架构

### 架构1：通过ROS2 Agent集中通信（推荐）

```
┌─────────┐         ┌─────────┐         ┌─────────┐
│ MCU1    │         │ MCU2    │         │ MCU3    │
│microROS │         │microROS │         │microROS │
└────┬────┘         └────┬────┘         └────┬────┘
     │                  │                  │
     │   UDP/TCP        │   UDP/TCP        │   UDP/TCP
     │   或串口          │   或串口          │   或串口
     └────────┬─────────┴────────┬─────────┘
              │                   │
              │   ROS2 DDS        │
              └────────┬──────────┘
                       │
              ┌─────────┴─────────┐
              │   ROS2 Agent      │
              │  (Linux/PC)       │
              └───────────────────┘
```

**工作原理：**
- 每个MCU通过microROS连接到同一个ROS2 Agent
- Agent负责DDS发现和消息路由
- 所有MCU可以互相发布/订阅Topic，调用服务

**优点：**
- 标准ROS2通信方式
- 支持多对多通信
- 易于调试和监控
- 可以与标准ROS2节点通信

**缺点：**
- 需要Agent作为中间件
- 增加网络延迟（如果使用网络传输）

### 架构2：点对点直接通信

```
┌─────────┐         ┌─────────┐
│ MCU1    │◄───────►│ MCU2    │
│microROS │  UART/  │microROS │
└─────────┘  CAN/   └─────────┘
              SPI
```

**工作原理：**
- MCU之间直接通过物理接口通信
- 需要自定义协议封装ROS2消息
- 或者使用topic_bus作为中间层

**优点：**
- 无需Agent
- 低延迟
- 适合固定拓扑

**缺点：**
- 需要自定义协议
- 扩展性差
- 调试困难

### 架构3：混合架构（topic_bus + microROS）

```
┌─────────────────────────────────┐
│ MCU1 (内部)                      │
│  ┌──────────┐   ┌────────────┐  │
│  │topic_bus │◄─►│ microROS   │  │
│  │          │   │  Bridge    │  │
│  └──────────┘   └──────┬─────┘  │
└────────────────────────┼────────┘
                          │
                    ROS2 Network
                          │
┌─────────────────────────┼────────┐
│ MCU2 (内部)             │        │
│  ┌──────────┐   ┌──────┴─────┐  │
│  │topic_bus │◄─►│ microROS   │  │
│  │          │   │  Bridge    │  │
│  └──────────┘   └────────────┘  │
└─────────────────────────────────┘
```

**工作原理：**
- MCU内部使用topic_bus进行事件驱动通信
- topic_bus与microROS通过桥接层连接
- 外部通信通过microROS和Agent

**优点：**
- 结合两者优势
- 内部通信高效（topic_bus）
- 外部通信标准（microROS）

## 实现方案

### 方案1：通过ROS2 Agent通信（推荐）

#### 1.1 配置每个MCU连接Agent

**MCU1配置：**
```c
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rmw_microxrcedds_c/rmw_microxrcedds_c.h>

// 配置传输层（UDP）
rmw_uros_options_t options;
options.transport = RMW_UXRCE_TRANSPORT_UDP;
options.agent_address = "192.168.1.100";  // Agent IP地址
options.agent_port = 8888;                 // Agent端口

// 初始化microROS
rcl_context_t context;
rcl_init(0, NULL, &context);

rcl_node_t node;
rclc_node_init_default(&node, "mcu1_node", "", &context);
```

**MCU2配置：**
```c
// 同样的配置，但节点名不同
rclc_node_init_default(&node, "mcu2_node", "", &context);
```

**MCU3配置：**
```c
rclc_node_init_default(&node, "mcu3_node", "", &context);
```

#### 1.2 发布/订阅Topic

**MCU1发布数据：**
```c
// MCU1中
rcl_publisher_t publisher;
std_msgs__msg__Int32 msg;

void mcu1_init(void) {
    const rosidl_message_type_support_t * ts = 
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32);
    rclc_publisher_init_default(&publisher, &node, ts, "mcu1_sensor_data");
    msg.data = 0;
}

void mcu1_publish(void) {
    msg.data++;
    rcl_publish(&publisher, &msg, NULL);
}
```

**MCU2订阅MCU1的数据：**
```c
// MCU2中
rcl_subscription_t subscription;
std_msgs__msg__Int32 received_msg;

void mcu2_subscription_callback(const void * msgin) {
    const std_msgs__msg__Int32 * msg = (const std_msgs__msg__Int32 *)msgin;
    printf("MCU2 received from MCU1: %d\n", msg->data);
}

void mcu2_init(void) {
    const rosidl_message_type_support_t * ts = 
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32);
    rclc_subscription_init_default(&subscription, &node, ts, "mcu1_sensor_data");
    
    rclc_executor_t executor;
    rclc_executor_init(&executor, &context, 1, NULL);
    rclc_executor_add_subscription(
        &executor, &subscription, &received_msg,
        &mcu2_subscription_callback, ON_NEW_DATA
    );
}
```

**MCU3订阅MCU1的数据：**
```c
// MCU3中（类似MCU2）
rclc_subscription_init_default(&subscription, &node, ts, "mcu1_sensor_data");
```

#### 1.3 启动ROS2 Agent

在Linux PC上启动Agent：
```bash
# 安装microROS Agent
sudo apt install ros-humble-micro-ros-agent

# 启动Agent（UDP模式）
ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888

# 或串口模式
ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0
```

### 方案2：点对点通信（UART/SPI/CAN）

#### 2.1 自定义协议封装

如果MCU之间直接通信，需要自定义协议：

```c
// 消息帧结构
typedef struct {
    uint8_t start_flag;      // 0xAA
    uint16_t topic_id;       // Topic ID
    uint16_t data_len;        // 数据长度
    uint8_t data[256];        // 数据内容
    uint8_t checksum;         // 校验和
} microros_frame_t;

// 发送函数
void send_to_peer_mcu(uint16_t topic_id, const void* data, size_t len) {
    microros_frame_t frame;
    frame.start_flag = 0xAA;
    frame.topic_id = topic_id;
    frame.data_len = len;
    memcpy(frame.data, data, len);
    frame.checksum = calculate_checksum(&frame);
    
    // 通过UART发送
    uart_send((uint8_t*)&frame, sizeof(frame));
}
```

#### 2.2 接收和路由

```c
void receive_from_peer_mcu(void) {
    microros_frame_t frame;
    if (uart_receive((uint8_t*)&frame, sizeof(frame))) {
        // 验证帧
        if (frame.start_flag == 0xAA && 
            verify_checksum(&frame)) {
            // 路由到microROS
            route_to_microros(frame.topic_id, frame.data, frame.data_len);
        }
    }
}
```

### 方案3：topic_bus + microROS桥接

#### 3.1 桥接层实现

```c
// topic_bus_microros_bridge.c
#include "topic_bus.h"
#include "rcl/rcl.h"

// Topic ID到ROS Topic名称的映射
typedef struct {
    uint16_t topic_id;
    const char* ros_topic_name;
    const rosidl_message_type_support_t* type_support;
} topic_mapping_t;

static topic_mapping_t topic_map[] = {
    {1, "mcu1_sensor_data", ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32)},
    {2, "mcu2_sensor_data", ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32)},
    // ... 更多映射
};

// Topic Bus回调：发布到microROS
void topic_bus_to_microros_callback(uint16_t topic_id, const void* data, 
                                     size_t len, void* user_data) {
    // 查找映射
    for (size_t i = 0; i < sizeof(topic_map)/sizeof(topic_map[0]); i++) {
        if (topic_map[i].topic_id == topic_id) {
            // 发布到microROS
            rcl_publisher_t* pub = (rcl_publisher_t*)user_data;
            rcl_publish(pub, data, NULL);
            break;
        }
    }
}

// microROS回调：发布到Topic Bus
void microros_to_topic_bus_callback(const void* msgin) {
    // 从microROS接收消息
    // 转换为Topic Bus事件
    topic_publish_event(&bus, topic_id);
}
```

#### 3.2 初始化桥接

```c
void init_bridge(topic_bus_t* bus, rcl_node_t* node) {
    // 为每个Topic创建Publisher
    for (size_t i = 0; i < sizeof(topic_map)/sizeof(topic_map[0]); i++) {
        rcl_publisher_t publisher;
        rclc_publisher_init_default(
            &publisher, node, 
            topic_map[i].type_support,
            topic_map[i].ros_topic_name
        );
        
        // 订阅Topic Bus
        topic_subscribe(bus, topic_map[i].topic_id,
                       topic_bus_to_microros_callback,
                       &publisher);
    }
}
```

## 网络配置

### 使用UDP/IP通信

#### MCU端配置
```c
// 配置UDP传输
rmw_uros_options_t options;
options.transport = RMW_UXRCE_TRANSPORT_UDP;
options.agent_address = "192.168.1.100";  // Agent IP
options.agent_port = 8888;
```

#### Agent端启动
```bash
ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888
```

### 使用TCP/IP通信

#### MCU端配置
```c
options.transport = RMW_UXRCE_TRANSPORT_TCP;
options.agent_address = "192.168.1.100";
options.agent_port = 8888;
```

#### Agent端启动
```bash
ros2 run micro_ros_agent micro_ros_agent tcp4 --port 8888
```

### 使用串口通信

#### MCU端配置
```c
options.transport = RMW_UXRCE_TRANSPORT_SERIAL;
options.device = "/dev/ttyUSB0";
options.baudrate = 115200;
```

#### Agent端启动
```bash
ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 115200
```

## 多MCU通信示例

### 示例：MCU1发布，MCU2和MCU3订阅

**MCU1代码：**
```c
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <std_msgs/msg/int32.h>

rcl_node_t node;
rcl_publisher_t publisher;
std_msgs__msg__Int32 msg;
rcl_context_t context;

void mcu1_init(void) {
    // 初始化microROS
    rcl_init(0, NULL, &context);
    rclc_node_init_default(&node, "mcu1_node", "", &context);
    
    // 创建Publisher
    const rosidl_message_type_support_t * ts = 
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32);
    rclc_publisher_init_default(&publisher, &node, ts, "sensor_data");
    
    msg.data = 0;
}

void mcu1_loop(void) {
    // 发布数据
    msg.data++;
    rcl_publish(&publisher, &msg, NULL);
    rclc_sleep_ms(1000);  // 每秒发布一次
}
```

**MCU2代码：**
```c
rcl_node_t node;
rcl_subscription_t subscription;
std_msgs__msg__Int32 received_msg;
rclc_executor_t executor;
rcl_context_t context;

void mcu2_subscription_callback(const void * msgin) {
    const std_msgs__msg__Int32 * msg = (const std_msgs__msg__Int32 *)msgin;
    printf("MCU2: Received %d\n", msg->data);
    // 处理数据...
}

void mcu2_init(void) {
    rcl_init(0, NULL, &context);
    rclc_node_init_default(&node, "mcu2_node", "", &context);
    
    // 创建Subscription
    const rosidl_message_type_support_t * ts = 
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32);
    rclc_subscription_init_default(&subscription, &node, ts, "sensor_data");
    
    // 添加到执行器
    rclc_executor_init(&executor, &context, 1, NULL);
    rclc_executor_add_subscription(
        &executor, &subscription, &received_msg,
        &mcu2_subscription_callback, ON_NEW_DATA
    );
}

void mcu2_loop(void) {
    rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
}
```

**MCU3代码：**
```c
// 与MCU2类似，只需修改节点名
rclc_node_init_default(&node, "mcu3_node", "", &context);
```

## 调试和监控

### 查看连接状态

在Agent端运行：
```bash
# 查看所有连接的节点
ros2 node list

# 查看所有Topic
ros2 topic list

# 查看Topic数据
ros2 topic echo /sensor_data

# 查看Topic信息
ros2 topic info /sensor_data
```

### MCU端调试

```c
// 检查连接状态
rmw_ret_t ret = rmw_uros_ping_agent(1000);  // 1秒超时
if (ret != RMW_RET_OK) {
    printf("Agent not connected!\n");
}
```

## 性能优化

### 1. 减少消息大小
- 使用紧凑的消息类型
- 避免发送不必要的数据

### 2. 调整QoS
```c
rcl_publisher_options_t pub_options = rcl_publisher_get_default_options();
pub_options.qos.reliability = RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT;
pub_options.qos.durability = RMW_QOS_POLICY_DURABILITY_VOLATILE;
rclc_publisher_init(&publisher, &node, ts, "topic", &pub_options);
```

### 3. 批量处理
- 使用消息数组批量发送
- 减少网络开销

## 注意事项

1. **节点命名唯一性**：确保每个MCU的节点名称唯一
2. **Topic命名规范**：使用清晰的Topic命名，便于识别来源
3. **网络配置**：确保所有MCU和Agent在同一网络
4. **时间同步**：考虑使用NTP或ROS2时间同步
5. **错误处理**：实现连接重试和错误恢复机制
6. **资源限制**：注意MCU内存和计算资源限制

## 总结

### 推荐方案

1. **标准场景**：使用方案1（通过ROS2 Agent），简单可靠
2. **高性能场景**：使用方案3（topic_bus + microROS），内部高效，外部标准
3. **特殊场景**：使用方案2（点对点），适合固定拓扑

### 选择依据

- **有Agent**：使用方案1或3
- **无Agent但需要标准通信**：实现方案2的自定义协议
- **需要高性能内部通信**：使用方案3的topic_bus

## 参考资料

- [microROS官方文档](https://micro.ros.org/)
- [ROS2 DDS配置](https://docs.ros.org/en/humble/Concepts/About-DDS-and-ROS-2.html)
- [topic_bus模块文档](./topic_bus.md)

