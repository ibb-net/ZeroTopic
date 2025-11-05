<!-- 6cbc6254-8ada-4189-97d6-c195a1537690 805d68c1-f488-479c-acab-3d7c4076f77f -->
# VFB模块重构与功能扩展实现计划

## 模块架构设计（对齐microROS移植与对象字典统一数据模型）

### 核心模块划分

1. **环形队列库** (`ring_buffer/`) - MCU优化的高效环形队列
2. **对象字典/事件字典库** (`obj_dict/`) - KEY-Value数据管理，支持RAM/Flash
3. **Topic事件框架** (`topic_bus/`) - 基于回调的事件发布订阅框架
4. **性能测试框架** (`perf_test/`) - 统一的性能测试接口
5. **配置系统** (`config/`) - 基于配置文件的模块参数管理

### 目录结构

```
OpenIBBOs/Middlewares/
├── ring_buffer/          # 环形队列库
│   ├── ring_buffer.h
│   ├── ring_buffer.c
│   ├── ring_buffer_config.h
│   └── perf_test_ring_buffer.c
├── obj_dict/            # 对象字典库
│   ├── obj_dict.h
│   ├── obj_dict.c
│   ├── obj_dict_ram.h
│   ├── obj_dict_ram.c
│   ├── obj_dict_flash.h
│   ├── obj_dict_flash.c
│   ├── obj_dict_config.h
│   └── perf_test_obj_dict.c
├── topic_bus/            # Topic事件框架
│   ├── topic_bus.h
│   ├── topic_bus.c
│   ├── topic_bus_config.h
│   ├── topic_rule.h      # Topic规则定义
│   ├── topic_rule.c
│   └── perf_test_topic_bus.c
└── vfb_new/             # 新VFB主模块（可选，用于逐步迁移）
    ├── vfb_new.h
    └── vfb_new.c
```

## C11原子操作优化策略 ⭐新增⭐

### 优化背景
基于 `-std=gnu11` 编译器标准，项目已具备C11原子操作支持。当前使用`volatile`关键字和信号量保护在多核/高频场景下存在性能瓶颈。

### 优化目标
1. **无锁高性能**：使用C11原子操作替代volatile，消除关键路径锁开销
2. **内存序保证**：使用memory_order规范保证多线程/ISR安全性
3. **零拷贝优化**：结合原子索引实现最高效数据传递
4. **平台无关性**：C11原子操作提供统一接口，适配多种ARM架构

### 优化方案对比

#### 方案对比表
| 特性 | volatile | FreeRTOS原子(关中断) | **C11原子操作** ⭐ |
|------|----------|---------------------|-----------------|
| 编译器支持 | C99+ | FreeRTOS | C11+ |
| 性能 | 基准 | 高开销(关中断) | **优秀** |
| 多核支持 | 弱 | 支持 | **强** |
| ISR安全 | 弱 | 支持 | **支持** |
| 内存序 | 无保证 | 全局顺序 | **精确控制** |
| 可移植性 | 通用 | FreeRTOS绑定 | **通用** |
| 推荐场景 | 单线程 | RTOS单核 | **MCU优化** ⭐ |

#### 内存序选择策略

```c
/* 针对不同场景选择最合适的memory_order */

// 1. SPSC(单生产者单消费者) - 最优化性能
atomic_load_explicit(&rb->read_idx, memory_order_relaxed)  // 读索引
atomic_store_explicit(&rb->write_idx, new_val, memory_order_release)  // 写索引

// 2. MPMC(多生产者多消费者) - 保证安全性
atomic_load_explicit(&rb->read_idx, memory_order_acquire)  // 读索引
atomic_store_explicit(&rb->write_idx, new_val, memory_order_release)  // 写索引

// 3. ISR路径 - 中断上下文优化
atomic_load_explicit(&rb->write_idx, memory_order_relaxed)  // 只读不写
atomic_store_explicit(&rb->write_idx, new_val, memory_order_release)  // 发布写入
```

### 实施方案

#### 1. ring_buffer原子优化

**核心改进**：
- 将`volatile size_t write_idx/read_idx`替换为`atomic_size_t`
- 使用`memory_order_release/acquire`保证SPSC场景下的正确性
- 保持与现有FreeRTOS阻塞接口兼容

**性能预期**：
- 非阻塞操作：**~50%延迟降低** (ns级优化)
- 阻塞操作：保持相同，但减少锁竞争
- ISR操作：**~30%延迟降低** (无锁路径)

#### 2. obj_dict原子优化

**核心改进**：
- version字段使用原子递增保证线程安全
- timestamp使用原子操作保证数据一致性
- 支持无锁读路径(lockless read path)

**性能预期**：
- 无锁读：**~70%延迟降低** (高频查询场景)
- 写操作：通过atomic increment优化版本管理

#### 3. topic_bus原子优化

**核心改进**：
- 订阅列表使用无锁链表
- 事件计数使用原子操作
- 规则匹配掩码使用原子位操作

**性能预期**：
- 事件发布：**~40%延迟降低** (高频事件场景)
- 规则匹配：减少锁等待时间

## 详细实现计划（更新：对象字典统一数据缓冲；microROS快速移植接口预置；C11原子优化）

### 阶段1: 环形队列库 (ring_buffer) - ⭐C11原子优化⭐

**文件**: `ring_buffer/ring_buffer.h`, `ring_buffer.c`

**核心功能**:

- MCU优化的无锁环形队列 (**C11原子操作**)
- 支持单生产者单消费者(SPSC)和多生产者多消费者(MPMC)
- 零拷贝支持（可选）
- 内存对齐优化

**关键结构**:

```c
#include <stdatomic.h>

typedef struct {
    uint8_t* buffer;                    // 数据缓冲区
    size_t capacity;                     // 容量（必须是2的幂）
    atomic_size_t write_idx;            // ⭐原子写索引⭐
    atomic_size_t read_idx;             // ⭐原子读索引⭐
    size_t item_size;                    // 元素大小
#if RING_BUFFER_ENABLE_BLOCKING
    OsSemaphore_t* write_sem;           // 写信号量（可选）
    OsSemaphore_t* read_sem;            // 读信号量（可选）
#endif
    int lock_mode;                       // 锁模式：无锁/信号量/互斥锁
} ring_buffer_t;
```

**接口设计**:

- `ring_buffer_create()` - 创建环形队列
- `ring_buffer_write()` - 写入数据（支持阻塞/非阻塞）- ⭐原子优化⭐
- `ring_buffer_read()` - 读取数据（支持阻塞/非阻塞）- ⭐原子优化⭐
- `ring_buffer_isr_write()` - ISR安全写入 - ⭐原子优化⭐
- `ring_buffer_peek()` - 查看数据不移动指针
- `ring_buffer_get_count()` - 获取队列元素数量 - ⭐原子优化⭐
- `ring_buffer_reset()` - 重置队列

**性能测试**:

- 写入吞吐量测试
- 读取吞吐量测试
- 延迟测试
- 内存使用测试
- ⭐原子操作 vs volatile性能对比⭐

### 阶段2: 对象字典库 (obj_dict) - ⭐C11原子优化⭐

**文件**: `obj_dict/obj_dict.h`, `obj_dict.c`

**核心功能**:

- KEY(与event枚举一致)到数据条目的映射
- 固定长度数据的RAM存储
- 时间戳记录
- 数据版本管理 (**C11原子递增**)

**关键结构**:

```c
#include <stdatomic.h>

typedef uint16_t obj_dict_key_t;  // 与vfb_event_t一致

typedef struct {
    obj_dict_key_t key;                    // KEY == event id
    void*          value;                  // 数据缓冲区首地址
    size_t         value_len;              // 数据长度(字节)
    uint64_t       timestamp_us;           // 时间戳(微秒)
    atomic_uint_fast32_t version;          // ⭐原子版本号⭐
    uint8_t        flags;                  // 标志位（持久化/只读等）
} obj_dict_entry_t;

typedef struct {
    obj_dict_entry_t* entries;             // 条目数组
    size_t max_keys;                       // 最大KEY数量
    OsSemaphore_t* lock;                   // 线程安全锁(可选的写锁)
    void* ram_storage;                     // RAM存储区域(可选池)
    size_t ram_size;                       // RAM大小
} obj_dict_t;
```

**接口设计**:

- `obj_dict_init()` - 初始化对象字典
- `obj_dict_set()` - 设置KEY的值（RAM） - ⭐版本原子递增⭐
- `obj_dict_get()` - 获取KEY的值 - ⭐支持无锁读⭐
- `obj_dict_get_timestamp()` - 获取时间戳
- `obj_dict_get_version()` - 获取版本号 - ⭐原子读取⭐
- `obj_dict_iterate()` - 遍历所有KEY

**Flash持久化**:

- `obj_dict_flash_save()` - 保存到Flash
- `obj_dict_flash_load()` - 从Flash加载
- `obj_dict_flash_erase()` - 擦除Flash区域

### 阶段3: 数据管理库抽象层

**文件**: `obj_dict/obj_dict_storage.h`, `obj_dict_storage.c`

**核心功能**:

- 统一的存储接口（RAM/Flash/多内存块）
- 存储后端可插拔
- 调用RTE接口实现

**接口抽象**:

```c
typedef struct {
    int (*init)(void* storage, size_t size);
    int (*read)(obj_dict_key_t key, void* data, size_t size);
    int (*write)(obj_dict_key_t key, const void* data, size_t size);
    int (*erase)(obj_dict_key_t key);
} obj_dict_storage_ops_t;
```

### 阶段4: Topic机制

**文件**: `topic_bus/topic_bus.h`, `topic_bus.c`, `topic_rule.h`, `topic_rule.c`

**核心功能**:

- Topic定义（单一event或组合event规则组）
- 规则类型：OR（任一触发）、AND（全部触发）、手动发布
- 回调订阅机制

**关键结构**:

```c
typedef enum {
    TOPIC_RULE_OR,      // 任一event触发
    TOPIC_RULE_AND,     // 全部event触发
    TOPIC_RULE_MANUAL   // 手动触发
} topic_rule_type_t;

typedef struct {
    topic_rule_type_t type;
    obj_dict_key_t* events;      // 事件列表
    size_t event_count;
    uint32_t trigger_mask;       // 触发掩码（用于AND规则）
} topic_rule_t;

typedef struct {
    uint16_t topic_id;
    topic_rule_t rule;
    void (*callback)(uint16_t topic_id, const void* data);
    void* user_data;
} topic_subscription_t;
```

**接口设计**:

- `topic_bus_init()` - 初始化Topic总线
- `topic_rule_create()` - 创建Topic规则
- `topic_subscribe()` - 订阅Topic（回调函数）
- `topic_publish_event()` - 发布事件（自动触发相关Topic）
- `topic_publish_manual()` - 手动发布Topic
- `topic_unsubscribe()` - 取消订阅

### 阶段5: 新的发布订阅框架

**文件**: `topic_bus/topic_bus.c`

**核心功能**:

- 替换原有的队列订阅机制为回调订阅
- 线程安全和ISR安全支持
- 与对象字典集成

**接口设计**:

- `topic_bus_subscribe()` - 注册回调订阅
- `topic_bus_unsubscribe()` - 取消订阅
- `topic_bus_publish()` - 发布事件（任务模式）
- `topic_bus_publish_isr()` - 发布事件（ISR模式）

### 阶段6: 线程安全强化

**关键点**:

- ISR路径使用无锁数据结构或中断安全API
- 使用RTE层的`os_semaphore_give_isr()`
- 关键路径使用临界区保护
- 避免死锁设计（锁顺序固定）

**实现**:

- ISR路径：环形队列 + 中断安全信号量 - ⭐C11原子优化⭐
- 任务路径：信号量/互斥锁保护
- 双重检查锁定优化

### 阶段7: 性能测试框架

**文件**: `perf_test/perf_test.h`, `perf_test.c`

**核心功能**:

- 统一的性能测试接口
- 配置驱动的测试用例
- 指标收集：吞吐量、延迟、CPU使用率
- ⭐原子操作性能对比⭐

**接口设计**:

- `perf_test_init()` - 初始化测试框架
- `perf_test_register()` - 注册测试用例
- `perf_test_run()` - 运行测试
- `perf_test_report()` - 生成测试报告

**配置文件**: `perf_test_config.h`

- 测试用例开关
- 测试参数配置
- 性能阈值定义

### 阶段8: 配置文件系统

**文件**: `config/module_config.h`

**配置内容**:

- 环形队列配置（容量、模式等）
- 对象字典配置（最大KEY数、存储方式等）
- Topic配置（最大Topic数、规则数等）
- 性能测试配置（测试开关、参数等）
- ⭐C11原子操作开关⭐

## 实现优先级

1. **P0 (核心功能)**:

   - 环形队列库 - ⭐C11原子优化⭐
   - 对象字典基础功能（RAM） - ⭐C11原子优化⭐
   - Topic基础机制
   - 回调订阅机制

2. **P1 (重要功能)**:

   - Flash持久化
   - Topic规则（OR/AND/手动）
   - 线程安全强化 - ⭐C11原子优化⭐
   - 性能测试框架 - ⭐原子对比测试⭐

3. **P2 (增强功能)**:

   - 多内存块管理
   - 高级性能测试
   - 配置文件系统优化

## 关键设计决策

1. **KEY与Event统一**: `obj_dict_key_t` 与 `vfb_event_t` 使用相同类型（`uint16_t`）
2. **回调优先**: 替换队列机制，使用回调函数订阅
3. **模块独立**: 各库独立实现，可单独使用
4. **MCU优化**: 避免动态内存分配（配置时分配），零拷贝支持
5. **向后兼容**: 新模块独立，不影响现有VFB代码（后续可逐步迁移）
6. ⭐**C11原子优先**: 性能关键路径使用C11原子操作，替代volatile和锁⭐

## 测试策略

1. 单元测试：每个库独立测试
2. 集成测试：模块间协作测试
3. 性能测试：使用性能测试框架 - ⭐原子vs锁性能对比⭐
4. ISR测试：中断上下文功能验证
5. 压力测试：高负载场景验证

## Topic Bus 测试用例详细设计 ⭐新增⭐

### 1. 基础功能测试用例

#### 1.1 Topic初始化与规则创建
- **测试目标**: 验证Topic Bus初始化、规则创建功能
- **测试步骤**:
  1. 初始化obj_dict和topic_bus
  2. 创建OR规则Topic（3个事件）
  3. 创建AND规则Topic（2个事件）
  4. 创建MANUAL规则Topic
- **预期结果**: 所有规则创建成功，无错误返回

#### 1.2 订阅与取消订阅
- **测试目标**: 验证Topic订阅机制
- **测试步骤**:
  1. 订阅OR规则Topic，注册回调函数
  2. 订阅AND规则Topic，注册多个回调函数
  3. 取消订阅，验证回调不再触发
- **预期结果**: 订阅成功，回调正确触发，取消订阅后不再触发

#### 1.3 OR规则触发测试
- **测试目标**: 验证OR规则（任一事件触发）
- **测试步骤**:
  1. 创建OR规则Topic（事件10, 20, 30）
  2. 发布事件10，验证Topic触发
  3. 发布事件20，验证Topic触发
  4. 发布事件30，验证Topic触发
  5. 发布不相关事件40，验证Topic不触发
- **预期结果**: 匹配事件均能触发Topic，不匹配事件不触发

#### 1.4 AND规则触发测试
- **测试目标**: 验证AND规则（全部事件触发）
- **测试步骤**:
  1. 创建AND规则Topic（事件40, 50）
  2. 发布事件40，验证Topic不触发
  3. 发布事件50，验证Topic触发
  4. 重置掩码，再次发布40和50，验证触发
- **预期结果**: 仅当所有事件都发布后才触发

#### 1.5 手动发布测试
- **测试目标**: 验证MANUAL规则手动触发
- **测试步骤**:
  1. 创建MANUAL规则Topic
  2. 调用topic_publish_manual()，验证Topic触发
  3. 发布相关事件，验证Topic不自动触发
- **预期结果**: 仅手动触发时回调执行

### 2. 时效性测试用例

#### 2.1 默认时效性测试
- **测试目标**: 验证默认超时时间（3000ms）生效
- **测试步骤**:
  1. 创建OR规则Topic，使用默认时效性
  2. 发布事件并立即发布Topic，验证触发
  3. 等待3100ms后发布Topic，验证不触发（超时）
- **预期结果**: 3秒内事件有效，超过3秒无效

#### 2.2 自定义时效性测试
- **测试目标**: 验证每个event_key独立的时效性配置
- **测试步骤**:
  1. 创建AND规则Topic（事件A超时1000ms，事件B超时5000ms）
  2. 发布事件A，等待500ms后发布事件B，验证触发（均在时效内）
  3. 重置，发布事件A，等待1500ms后发布事件B，验证不触发（A超时）
  4. 重置，发布事件A，等待500ms后发布事件B，等待6000ms后触发，验证不触发（B超时）
- **预期结果**: 每个事件的时效性独立判断

#### 2.3 最大延迟宏测试
- **测试目标**: 验证TOPIC_BUS_EVENT_TIMEOUT_MAX表示不判断时效性
- **测试步骤**:
  1. 创建规则，设置event_timeouts_ms为TOPIC_BUS_EVENT_TIMEOUT_MAX
  2. 发布事件，等待任意长时间后发布Topic
  3. 验证Topic始终触发
- **预期结果**: 设置为最大值时，时效性检查被跳过

#### 2.4 时效性边界测试
- **测试目标**: 验证时效性边界条件
- **测试步骤**:
  1. 设置超时为1000ms
  2. 发布事件，等待999ms后触发，验证成功
  3. 发布事件，等待1001ms后触发，验证失败
- **预期结果**: 边界值处理正确

### 3. 多Topic共享Event Key测试

#### 3.1 多个Topic共享同一Event Key
- **测试目标**: 验证不同Topic规则可以包含相同的event_key
- **测试步骤**:
  1. 创建Topic1（OR规则：事件10, 20）
  2. 创建Topic2（OR规则：事件20, 30）
  3. 发布事件20，验证Topic1和Topic2均触发
- **预期结果**: 同一事件可以触发多个Topic

#### 3.2 复杂规则组合测试
- **测试目标**: 验证复杂场景下的规则匹配
- **测试步骤**:
  1. 创建Topic1（OR规则：事件10, 20, 30）
  2. 创建Topic2（AND规则：事件20, 30）
  3. 创建Topic3（OR规则：事件30, 40）
  4. 发布事件10，验证Topic1触发，Topic2和Topic3不触发
  5. 发布事件20，验证Topic1触发，Topic2不触发（AND未完成）
  6. 发布事件30，验证Topic1、Topic2、Topic3均触发
- **预期结果**: 所有Topic按各自规则正确触发

### 4. Topic Server测试用例

#### 4.1 Server周期性运行测试
- **测试目标**: 验证Topic Server周期性处理ISR队列
- **测试步骤**:
  1. 初始化Topic Server，设置周期100ms
  2. 在ISR中发布事件到ISR队列
  3. 启动Server，验证事件被处理
  4. 停止Server，验证不再处理
- **预期结果**: Server按周期运行，处理ISR队列事件

#### 4.2 Server性能统计测试
- **测试目标**: 验证Topic Server性能统计功能
- **测试步骤**:
  1. 运行Server处理多个事件
  2. 调用topic_server_get_stats()获取统计信息
  3. 验证总处理数和平均耗时
- **预期结果**: 统计数据准确

#### 4.3 Server任务模式测试
- **测试目标**: 验证Topic Server创建独立任务运行
- **测试步骤**:
  1. 调用topic_server_start()创建任务
  2. 验证任务周期性运行
  3. 调用topic_server_stop()停止任务
- **预期结果**: 任务创建成功，周期性运行，停止后不再运行

### 5. Topic Router测试用例

#### 5.1 VFB Router测试
- **测试目标**: 验证Topic触发时通过VFB广播
- **测试步骤**:
  1. 初始化Topic Router，添加VFB Router（Topic1 → VFB事件100）
  2. 设置Router到Topic Bus
  3. 创建Topic1并触发
  4. 验证VFB事件100被发送
- **预期结果**: Topic触发时，VFB事件正确广播

#### 5.2 自定义Router测试
- **测试目标**: 验证自定义Router回调处理
- **测试步骤**:
  1. 定义自定义回调函数
  2. 添加自定义Router到Topic Router
  3. 触发Topic，验证回调被调用
  4. 验证回调参数正确
- **预期结果**: 自定义回调正确执行

#### 5.3 多Router测试
- **测试目标**: 验证一个Topic可以配置多个Router
- **测试步骤**:
  1. 为Topic1添加VFB Router和自定义Router
  2. 触发Topic1
  3. 验证VFB事件发送和自定义回调均执行
- **预期结果**: 所有Router均被调用

#### 5.4 Router移除测试
- **测试目标**: 验证Router移除功能
- **测试步骤**:
  1. 添加Router并触发Topic，验证执行
  2. 移除Router
  3. 再次触发Topic，验证Router不再执行
- **预期结果**: Router移除后不再执行

### 6. ISR路径测试用例

#### 6.1 ISR发布测试
- **测试目标**: 验证ISR上下文安全发布
- **测试步骤**:
  1. 在模拟ISR中调用topic_publish_isr()
  2. 验证事件进入ISR队列
  3. 在任务上下文中调用topic_bus_process_isr_queue()
  4. 验证事件被处理，Topic触发
- **预期结果**: ISR发布安全，任务处理正常

#### 6.2 ISR队列满测试
- **测试目标**: 验证ISR队列满时的行为
- **测试步骤**:
  1. 快速发布大量事件到ISR队列
  2. 验证队列满时返回失败
  3. 处理队列后，验证可以继续发布
- **预期结果**: 队列满时正确处理，不阻塞

### 7. 性能测试用例

#### 7.1 事件发布延迟测试
- **测试目标**: 测量事件发布延迟
- **测试步骤**:
  1. 创建多个Topic（5个OR规则Topic）
  2. 预热1000次
  3. 测量100000次发布的平均延迟
  4. 对比原子操作和锁版本性能
- **预期结果**: 延迟在预期范围内，原子版本性能更优

#### 7.2 规则匹配性能测试
- **测试目标**: 测量规则匹配性能
- **测试步骤**:
  1. 创建10个Topic（OR规则）
  2. 测量OR规则匹配延迟
  3. 创建AND规则Topic（3个事件）
  4. 测量AND规则匹配延迟
- **预期结果**: 匹配延迟满足实时性要求

#### 7.3 时效性检查性能测试
- **测试目标**: 测量时效性检查开销
- **测试步骤**:
  1. 创建带时效性配置的规则
  2. 测量时效性检查的额外开销
  3. 对比启用和禁用时效性检查的性能差异
- **预期结果**: 时效性检查开销可接受

#### 7.4 高并发测试
- **测试目标**: 验证高并发场景下的性能
- **测试步骤**:
  1. 创建大量Topic（64个）
  2. 创建多个订阅者（每个Topic 8个订阅者）
  3. 并发发布事件，测量吞吐量
  4. 验证无数据竞争和死锁
- **预期结果**: 高并发下性能稳定，无竞争

### 8. 集成测试用例

#### 8.1 Topic Bus + Obj Dict集成测试
- **测试目标**: 验证Topic Bus与对象字典的集成
- **测试步骤**:
  1. 在对象字典中设置事件数据
  2. 发布事件，触发Topic
  3. 验证回调中获取的数据正确
- **预期结果**: 数据正确传递

#### 8.2 Topic Bus + Router + VFB集成测试
- **测试目标**: 验证完整链路：Topic触发 → Router → VFB广播
- **测试步骤**:
  1. 配置Topic Router（VFB类型）
  2. 触发Topic
  3. 验证VFB事件被正确广播
  4. 验证VFB订阅者收到数据
- **预期结果**: 完整链路正常工作

#### 8.3 Topic Server + ISR + Router集成测试
- **测试目标**: 验证完整场景：ISR发布 → Server处理 → Router转发
- **测试步骤**:
  1. 在ISR中发布事件
  2. Topic Server周期性处理
  3. Topic触发，Router转发
  4. 验证端到端数据流
- **预期结果**: 完整场景正常工作

### 9. 压力测试用例

#### 9.1 长时间运行测试
- **测试目标**: 验证长时间运行稳定性
- **测试步骤**:
  1. 运行24小时，持续发布事件
  2. 监控内存使用和泄漏
  3. 验证统计数据无溢出
- **预期结果**: 长时间运行稳定，无内存泄漏

#### 9.2 高频事件测试
- **测试目标**: 验证高频事件场景
- **测试步骤**:
  1. 以1MHz频率发布事件
  2. 验证系统不崩溃
  3. 验证事件不丢失（关键事件）
- **预期结果**: 高频场景下系统稳定

#### 9.3 大量Topic测试
- **测试目标**: 验证大量Topic场景
- **测试步骤**:
  1. 创建最大数量Topic（64个）
  2. 每个Topic配置多个订阅者
  3. 并发发布事件，验证性能
- **预期结果**: 大量Topic下性能可接受

### 10. 错误处理测试用例

#### 10.1 无效参数测试
- **测试目标**: 验证参数校验
- **测试步骤**:
  1. 传入NULL指针
  2. 传入无效Topic ID
  3. 传入无效规则配置
- **预期结果**: 返回错误，不崩溃

#### 10.2 资源耗尽测试
- **测试目标**: 验证资源耗尽时的处理
- **测试步骤**:
  1. 创建最大数量Topic
  2. 尝试创建更多Topic，验证失败
  3. 添加最大数量订阅者
  4. 尝试添加更多订阅者，验证失败
- **预期结果**: 资源耗尽时优雅降级

### 测试用例执行计划

#### 阶段1：基础功能测试（P0）
- 1.1 ~ 1.5 基础功能测试用例
- 2.1 ~ 2.4 时效性测试用例
- 3.1 ~ 3.2 多Topic共享测试用例

#### 阶段2：新功能测试（P1）
- 4.1 ~ 4.3 Topic Server测试用例
- 5.1 ~ 5.4 Topic Router测试用例
- 6.1 ~ 6.2 ISR路径测试用例

#### 阶段3：性能与集成测试（P1）
- 7.1 ~ 7.4 性能测试用例
- 8.1 ~ 8.3 集成测试用例

#### 阶段4：压力与稳定性测试（P2）
- 9.1 ~ 9.3 压力测试用例
- 10.1 ~ 10.2 错误处理测试用例

### 测试工具与环境

- **测试框架**: 使用现有的perf_test_topic_bus.c框架
- **测试环境**: Linux POSIX环境 + 嵌入式MCU环境
- **性能基准**: 原子操作版本 vs 锁版本对比
- **自动化**: 所有测试用例可自动化执行

## 性能优化收益预期 ⭐新增⭐

### ring_buffer
- **延迟优化**: 非阻塞操作 ~50%降低 (volatile → atomic_relaxed)
- **吞吐优化**: 高频场景 ~30%提升 (减少锁竞争)
- **ISR优化**: 中断延迟 ~30%降低 (无锁路径)

### obj_dict
- **读路径**: 无锁读 ~70%延迟降低 (高频查询场景)
- **版本管理**: 原子递增避免锁开销
- **并发性能**: 多读者场景显著提升

### topic_bus
- **事件发布**: ~40%延迟降低 (高频事件)
- **规则匹配**: 减少锁等待时间
- **整体吞吐**: ~25%提升

### To-dos

- [ ] 实现环形队列库(ring_buffer)：**支持C11原子操作**、支持SPSC/MPMC模式、ISR安全、零拷贝、性能测试
- [ ] 实现对象字典核心(obj_dict)：**支持C11原子版本管理**、KEY-Value映射、RAM存储、时间戳记录
- [ ] 实现存储抽象层：RAM后端、Flash后端接口、多内存块管理支持
- [ ] 实现Topic规则引擎：OR/AND/手动触发规则、规则匹配算法
- [ ] 实现Topic总线核心：回调订阅机制、事件发布、Topic触发
- [ ] 实现ISR安全发布订阅：中断上下文支持、**C11原子无锁优化**、线程安全强化
- [ ] 实现性能测试框架：统一测试接口、配置驱动、指标收集与报告、**原子性能对比**
- [ ] 实现配置文件系统：各模块配置统一管理、条件编译支持、**原子操作开关**
- [ ] 集成测试：模块间协作验证、压力测试、ISR场景测试

<!-- ZeroTopic: 按优先级新增任务（简短条目） -->
- [ ] [P0] 数据生命周期管理：引用计数/RAII，确保订阅者数据指针有效
- [ ] [P0] 并发安全增强：多订阅者并发访问的数据同步与冲突规避
- [ ] [P1] 内存池优化：对象字典采用内存池/分级池，减少动态分配
- [ ] [P1] 泄漏防护：对象字典数据清理与生命周期钩子，防止内存泄漏
- [ ] [P2] 规则缓存优化：对高频规则匹配进行缓存与快速路径加速
- [ ] [P2] 监控与诊断：运行时性能统计、热点追踪与调试开关
- [ ] [P2] 调试优化：指针传递路径的可观测性与错误信息增强


## microROS 对接与RTE适配 ⭐新增⭐

### 目标
- 与 microROS 话题/服务语义对齐：事件→Topic、对象字典→参数/状态源
- 通过 RTE 适配层屏蔽 OS/硬件差异，保证可移植性

### 目录与文件
- `rte/`：抽象 OS 同步原语、时间、内存、日志
  - `rte_os.h/.c`：`os_mutex/os_semaphore/os_time_us/os_enter_critical` 等
  - `rte_mem.h/.c`：静态/池化内存分配接口
  - `rte_log.h/.c`：分级日志（ISR安全版本）
- `uros_bridge/`：microROS 桥接
  - `uros_bridge.h/.c`：Topic 映射、QoS、序列化/反序列化

### 接口映射
- `topic_bus_publish()` ↔ `rcl_publish()`
- `topic_subscribe()` ↔ `rcl_subscription` 回调适配
- QoS 映射：`BEST_EFFORT/RELIABLE` → 发布路径的阻塞/丢弃策略
- 类型适配：通过对象字典的 `value_len` 与IDL类型长度校验

### 快速移植步骤
1. 启用 `rte/` 层并完成 OS 绑定（FreeRTOS/NuttX/POSIX）
2. 编译通过 `uORB/microXRCE-DDS` 所需类型（可选）
3. 在 `uros_bridge` 中注册 Topic <→ event 映射表
4. 打通最小闭环：一个事件发布→一个 ROS 订阅者接收


## API 使用示例 ⭐新增⭐

### ring_buffer 最小示例
```c
static uint8_t storage[1024];
ring_buffer_t rb;

ring_buffer_create(&rb, storage, sizeof(storage), sizeof(uint32_t));

uint32_t v = 1234;
ring_buffer_write(&rb, &v, 1, 0 /* non-block */);

uint32_t out = 0;
size_t n = ring_buffer_read(&rb, &out, 1, 0);
```

### obj_dict 读写示例
```c
obj_dict_t dict;
obj_dict_init(&dict, entries_buf, max_keys, ram_pool, ram_size);

foo_t foo = {0};
obj_dict_set(&dict, VFB_EVENT_FOO, &foo, sizeof(foo));

foo_t foo_out;
obj_dict_get(&dict, VFB_EVENT_FOO, &foo_out, sizeof(foo_out));
uint32_t ver = obj_dict_get_version(&dict, VFB_EVENT_FOO);
```

### topic_bus 发布/订阅示例
```c
static void on_topic(uint16_t topic_id, const void* data) {
    (void)topic_id; (void)data;
}

topic_bus_init(&bus);
topic_subscribe(&bus, TOPIC_ID_FOO, on_topic, NULL);
topic_publish_event(&bus, VFB_EVENT_FOO, &payload, sizeof(payload));
```


## 内存与并发模型 ⭐新增⭐

### 内存布局
- 配置期静态分配：环形队列缓冲、对象字典 RAM 池、订阅表
- 对齐策略：按 `sizeof(max_native_word)` 对齐；Topic 载荷按 4/8 字节对齐
- 零拷贝：发布路径优先传递指针与长度，读端按需拷贝

### 并发模型
- SPSC：`memory_order_relaxed/release`，极致性能
- MPMC：`acquire/release` + 可选写锁降低冲突
- ISR：只走无锁快路径，避免阻塞；必要时仅使用 `os_semaphore_give_isr`

### 死锁避免
- 统一锁顺序：`obj_dict` → `topic_bus` → 后端存储
- ISR 禁止拿互斥锁；仅允许信号量 give/轻量原子操作


## 迁移与兼容策略 ⭐新增⭐

### 分阶段迁移
1. 引入 `vfb_new/` 与 `topic_bus` 并并存旧实现
2. 用回调订阅替换旧队列消费者，保留旧生产者
3. 将事件源逐一迁移到 `topic_publish_*` 接口
4. 删除旧队列实现与胶水层

### 兼容层
- 旧 API → 新 API 适配薄层：保持函数签名不变，内部转发到新框架
- 日志告警：当走到兼容层路径时输出 `DEPRECATED` 级别日志


## 风险与回滚计划 ⭐新增⭐

### 主要风险
- 原子内存序误用导致偶现一致性问题
- ISR 路径拷贝越界或指针悬挂
- microROS 桥接的类型不一致

### 缓解措施
- 关键路径单元测试 + 模拟并发竞态测试（随机调度/故障注入）
- 在 `-O2 -flto` 优化下做一致性回归
- 引入静态分析：clang-tidy/Cppcheck，开启 `-fsanitize=address/undefined` 的宿主侧仿真

### 回滚
- 模块化开关：`CONFIG_VFB_NEW`, `CONFIG_ATOMICS` 独立可关
- 可按子模块回滚：仅关闭 ring_buffer 原子实现，保留其余


## 里程碑与时间表 ⭐新增⭐

### M1 (1-2 周)
- 完成 ring_buffer 原子版 + 基准测试
- 完成 obj_dict 原子版（RAM 后端）

### M2 (2-3 周)
- 完成 topic_bus 回调版与基本规则（OR/AND）
- 打通最小 microROS 桥接链路

### M3 (2 周)
- 完成性能测试框架与原子对比报告
- 完成闪存后端基本读写 + 擦除

### M4 (1 周)
- 压力与ISR场景回归、修复与收尾
- 文档与迁移指引发布


## 代码规范与质量保障 ⭐新增⭐
- C11；启用 `-std=gnu11 -Wall -Wextra -Werror`（可阶段性放宽）
- 禁止隐式转换与未使用变量/参数
- 原子访问必须成对（acquire/release）并附带注释
- 引入 `clang-format` 统一风格；`clang-tidy` 静态检查
- 关键函数添加最小可复现示例（doctest/示例用例）


## 日志与调试 ⭐新增⭐
- `rte_log`：分级（ERROR/WARN/INFO/DEBUG/TRACE），可编译时裁剪
- ISR 安全日志：环形缓冲收集，任务态异步下刷
- 性能埋点：`PERF_MARK(name)` 与 `perf_test_report()` 关联

