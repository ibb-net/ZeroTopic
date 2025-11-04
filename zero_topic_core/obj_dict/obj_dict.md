# obj_dict 模块说明

## 概述
- 对象/事件字典：以 `key (uint16_t)` 统一标识，保存值缓冲、长度、时间戳、版本与标志。
- 设计用于与事件总线/Topic 紧耦合，支持按 KEY 快速读写与时间戳追踪。

## 数据模型
```c
typedef uint16_t obj_dict_key_t; // 与 vfb_event_t 一致

typedef struct {
    obj_dict_key_t key;
    void*          value;
    size_t         value_len;
    uint64_t       timestamp_us;
    uint32_t       version;
    uint8_t        flags;
} obj_dict_entry_t;
```

## 核心 API
```c
int obj_dict_init(obj_dict_t* dict, obj_dict_entry_t* entry_array, size_t max_keys);
int obj_dict_set(obj_dict_t* dict, obj_dict_key_t key, const void* data, size_t len, uint8_t flags);
ssize_t obj_dict_get(obj_dict_t* dict, obj_dict_key_t key, void* out, size_t out_cap,
                     uint64_t* ts_us, uint32_t* version, uint8_t* flags);
int obj_dict_iterate(obj_dict_t* dict, int next_from); // -1 开始
```

## 使用示例
```c
#define MAX_KEYS 128
static obj_dict_entry_t g_entries[MAX_KEYS];
static obj_dict_t g_dict;

obj_dict_init(&g_dict, g_entries, MAX_KEYS);
const char msg[] = "hello";
obj_dict_set(&g_dict, 1, msg, sizeof(msg), 0);
char buf[16]; uint64_t ts; uint32_t ver; uint8_t fl;
ssize_t n = obj_dict_get(&g_dict, 1, buf, sizeof(buf), &ts, &ver, &fl);
```

## 版本号的作用

版本号（version）是一个单调递增的原子计数器，每次写入数据时自动加1。它的主要用途包括：

### 1. **检测数据更新**
多线程/多任务场景下，用版本号判断数据是否被更新：
```c
uint32_t last_version = 0;
while (running) {
    uint32_t current_version;
    obj_dict_get(&dict, KEY_SENSOR_DATA, &data, sizeof(data), NULL, &current_version, NULL);
    
    if (current_version != last_version) {
        // 数据已更新，处理新数据
        process_data(&data);
        last_version = current_version;
    } else {
        // 数据未变化，可跳过处理
    }
}
```

### 2. **实现乐观锁（Optimistic Locking）**
CAS 类并发控制：
```c
uint32_t expected_version = 10;
obj_dict_get(&dict, KEY_CONFIG, &config, sizeof(config), NULL, &expected_version, NULL);

// 在另一个线程修改数据...
config.value += 1;

// 检查版本是否变化，避免覆盖
uint32_t current_version;
obj_dict_get(&dict, KEY_CONFIG, &check_data, sizeof(check_data), NULL, &current_version, NULL);
if (current_version == expected_version) {
    obj_dict_set(&dict, KEY_CONFIG, &config, sizeof(config), 0);  // 安全写入
} else {
    // 版本已被其他线程修改，需要重新读取
}
```

### 3. **状态变化追踪**
监控指定数据的变化频率：
```c
uint32_t old_version = 0;
while (monitoring) {
    uint32_t new_version;
    obj_dict_get(&dict, KEY_STATUS, &status, sizeof(status), NULL, &new_version, NULL);
    
    if (new_version > old_version) {
        uint32_t change_count = new_version - old_version;
        printf("状态变化了 %u 次\n", change_count);
        old_version = new_version;
    }
}
```

### 4. **实现 Last-Value Cache（ROS2概念）**
作为 ROS Topic 的最新值缓存：
```c
// Publisher侧：每发布一次topic，更新dict
void on_topic_update(const ros_msg_t* msg) {
    obj_dict_set(&dict, KEY_MOTOR_SPEED, &msg->speed, sizeof(msg->speed), 0);
}

// Subscriber侧：检查是否是新数据
void check_topic_update(void) {
    static uint32_t last_version = 0;
    uint32_t current_version;
    float speed;
    
    obj_dict_get(&dict, KEY_MOTOR_SPEED, &speed, sizeof(speed), NULL, &current_version, NULL);
    
    if (current_version > last_version) {
        on_new_motor_speed(speed);
        last_version = current_version;
    }
}
```

### 5. **调试和诊断**
快速定位异常更新：
```c
uint32_t previous_version = 0;
while (true) {
    uint32_t current_version;
    obj_dict_get(&dict, KEY_TEMP, &temp, sizeof(temp), NULL, &current_version, NULL);
    
    if (current_version - previous_version > 1) {
        // 检测到版本跳跃，可能有多线程竞争或数据丢失
        os_printf("[WARN] 版本跳跃: %u -> %u\n", previous_version, current_version);
    }
    previous_version = current_version;
}
```

### 性能特点
- **原子操作**：使用C11 `atomic_uint_fast32_t`，无锁读写
- **低开销**：原子自增约 1-2 CPU 周期
- **内存序**：`memory_order_release/acquire` 保证跨线程一致性
- **永不溢出**：32位最大约 42 亿次，满足长期运行

## 持久化与多内存块（预留）
- `obj_dict_storage_ops_t` 作为后端抽象（RAM/Flash/多块），后续文件 `obj_dict_storage.*` 对接。

## 与 microROS 的关系
- 可作为 ROS2 Topic 本地"最后值缓存"(last-value cache)，并用于桥接消息/事件键值化访问。

## 性能与并发测试

### 测试功能
详细测试实现位于 `perf_test_obj_dict.c`，包括：

1. **基础功能测试**
   - set/get 数据完整性校验
   - 版本号递增检查
   - 遍历功能验证

2. **性能测试**
   - 吞吐量测试：单线程写入/读取/混合操作
   - 数据大小测试：4B~256B不同大小的性能对比
   - 平均延迟：每个操作的纳秒级延迟

3. **并发测试**
   - 多线程纯写入测试：4线程并发写入
   - 多线程读写混合测试：4线程并发读写
   - 线程安全性验证：无数据竞争

4. **版本一致性测试**
   - 原子版本号递增验证
   - 连续写入版本号检查

### 测试运行
```bash
cd OpenIBBOs/apps/linux_demo/build
make
./bin/OpenIBBOs_Linux_Demo
```

### 性能数据示例
测试环境：Linux x86_64, GCC优化

- **单线程吞吐**：150-160 ns/op
- **多线程并发**：480-560 ns/op (4线程)
- **版本号管理**：基于C11原子操作，严格递增
- **线程安全**：使用信号量保证数据一致性


