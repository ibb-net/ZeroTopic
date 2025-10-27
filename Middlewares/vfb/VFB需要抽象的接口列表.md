# VFB 需要抽象的接口列表

## 概述
基于对 `vfb.c` 文件的分析，整理出VFB模块需要抽象的系统接口，以便完全脱离FreeRTOS依赖。

## 当前状态分析

### ✅ 已完成抽象的接口
| 接口类型 | 原始接口 | 已替换为 | 状态 |
|---------|---------|---------|------|
| 内存管理 | `pvPortMalloc()` | `os_malloc()` | ✅ 完成 |
| 内存管理 | `vPortFree()` | `os_free()` | ✅ 完成 |
| 信号量 | `xSemaphoreCreateBinary()` | `os_semaphore_create()` | ✅ 完成 |
| 信号量 | `xSemaphoreTake()` | `os_semaphore_take()` | ✅ 完成 |
| 信号量 | `xSemaphoreGive()` | `os_semaphore_give()` | ✅ 完成 |
| 信号量 | `xSemaphoreGiveFromISR()` | `os_semaphore_give_isr()` | ✅ 完成 |
| 任务 | `xTaskGetCurrentTaskHandle()` | `os_thread_get_current()` | ✅ 完成 |
| 日志 | `VFB_E/W/I/D()` | `os_printf()` | ✅ 完成 |

### ❌ 仍需抽象的接口

## 1. 队列管理接口

### 当前使用的FreeRTOS接口
```c
// 队列创建
QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize);

// 队列发送（任务模式）
BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait);

// 队列发送（中断模式）
BaseType_t xQueueSendFromISR(QueueHandle_t xQueue, const void *pvItemToQueue, BaseType_t *pxHigherPriorityTaskWoken);

// 队列接收
BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait);
```

### 需要抽象的接口
```c
// 建议在Rte层添加的队列接口
typedef void OsQueue_t;

// 队列创建
OsQueue_t* os_queue_create(size_t queue_length, size_t item_size, const char* pName);

// 队列销毁
ssize_t os_queue_destroy(OsQueue_t* pQueue);

// 队列发送（任务模式）
ssize_t os_queue_send(OsQueue_t* pQueue, const void* pItem, size_t timeout_ms);

// 队列发送（中断模式）
ssize_t os_queue_send_isr(OsQueue_t* pQueue, const void* pItem);

// 队列接收
ssize_t os_queue_receive(OsQueue_t* pQueue, void* pItem, size_t timeout_ms);

// 队列状态查询
ssize_t os_queue_get_count(OsQueue_t* pQueue);
ssize_t os_queue_get_space(OsQueue_t* pQueue);
```

### 使用位置
- `vfb_subscribe()` - 创建任务消息队列
- `__vfb_send_queue()` - 发送消息到队列
- `VFB_MsgReceive()` - 接收队列消息

## 2. 列表管理接口

### 当前使用的FreeRTOS接口
```c
// 列表初始化
void vListInitialise(List_t * const pxList);
void vListInitialiseItem(ListItem_t * const pxItem);

// 列表操作
void vListInsertEnd(List_t * const pxList, ListItem_t * const pxNewListItem);

// 列表查询
ListItem_t * listGET_HEAD_ENTRY(const List_t * const pxList);
ListItem_t * listGET_END_MARKER(const List_t * const pxList);
ListItem_t * listGET_NEXT(const ListItem_t * const pxListItem);
UBaseType_t listCURRENT_LIST_LENGTH(const List_t * const pxList);

// 列表项操作
TickType_t listGET_LIST_ITEM_VALUE(const ListItem_t * const pxListItem);
void listSET_LIST_ITEM_VALUE(ListItem_t * const pxListItem, TickType_t xItemValue);
```

### 需要抽象的接口
```c
// 建议在Rte层添加的列表接口
typedef void OsList_t;
typedef void OsListItem_t;

// 列表初始化
void os_list_initialise(OsList_t* pList);
void os_list_item_initialise(OsListItem_t* pItem);

// 列表操作
void os_list_insert_end(OsList_t* pList, OsListItem_t* pItem);
void os_list_insert_head(OsList_t* pList, OsListItem_t* pItem);
void os_list_remove_item(OsList_t* pList, OsListItem_t* pItem);

// 列表查询
OsListItem_t* os_list_get_head_entry(OsList_t* pList);
OsListItem_t* os_list_get_end_marker(OsList_t* pList);
OsListItem_t* os_list_get_next(OsListItem_t* pItem);
size_t os_list_get_length(OsList_t* pList);

// 列表项操作
uintptr_t os_list_item_get_value(OsListItem_t* pItem);
void os_list_item_set_value(OsListItem_t* pItem, uintptr_t value);
```

### 使用位置
- `__vfb_list_get_head()` - 获取事件列表头
- `__vfb_list_add_queue()` - 添加队列到列表
- `__vfb_list_find_queue()` - 查找队列
- `__vfb_send_core()` - 遍历事件订阅队列

## 3. 时间管理接口

### 当前使用的FreeRTOS接口
```c
// 时间转换
TickType_t pdMS_TO_TICKS(uint32_t xTimeInMs);

// 返回值定义
#define pdPASS    ( ( BaseType_t ) 1 )
#define pdTRUE    ( ( BaseType_t ) 1 )
#define pdFALSE   ( ( BaseType_t ) 0 )
```

### 需要抽象的接口
```c
// 建议在Rte层添加的时间接口
uint32_t os_ms_to_ticks(uint32_t ms);
uint32_t os_ticks_to_ms(uint32_t ticks);

// 返回值定义
#define OS_PASS    ( 1 )
#define OS_FAIL    ( 0 )
#define OS_TRUE    ( 1 )
#define OS_FALSE   ( 0 )
```

### 使用位置
- `__vfb_send_queue()` - 超时时间转换
- `VFB_MsgReceive()` - 超时时间转换

## 4. 任务管理接口扩展

### 当前使用的FreeRTOS接口
```c
// 获取任务名称
char *pcTaskGetName(TaskHandle_t xTaskToQuery);
```

### 需要抽象的接口
```c
// 建议在Rte层添加的任务接口
ssize_t os_thread_get_name(OsThread_t* pThread, char* pName, size_t name_length);
OsThread_t* os_thread_get_current(void);
ssize_t os_thread_get_priority(OsThread_t* pThread);
```

### 使用位置
- `vfb_subscribe()` - 获取当前任务名称

## 5. 数据类型抽象

### 当前使用的FreeRTOS类型
```c
typedef void * QueueHandle_t;
typedef void * List_t;
typedef void * ListItem_t;
typedef uint32_t TickType_t;
typedef long BaseType_t;
```

### 需要抽象的类型
```c
// 建议在Rte层定义的类型
typedef void* OsQueue_t;
typedef void* OsList_t;
typedef void* OsListItem_t;
typedef uint32_t OsTick_t;
typedef int OsResult_t;
```

## 6. 中断控制接口

### 当前使用的FreeRTOS接口
```c
// 中断上下文标志
BaseType_t xHigherPriorityTaskWoken;
```

### 需要抽象的接口
```c
// 建议在Rte层添加的中断接口
typedef struct {
    int higher_priority_task_woken;
} OsIsrContext_t;

void os_isr_context_init(OsIsrContext_t* pContext);
int os_isr_context_get_woken(OsIsrContext_t* pContext);
```

## 优先级排序

### 高优先级（必须实现）
1. **队列管理接口** - VFB核心功能依赖
2. **时间管理接口** - 超时控制必需
3. **数据类型抽象** - 类型安全必需

### 中优先级（建议实现）
4. **列表管理接口** - 事件管理功能
5. **任务管理接口扩展** - 调试和监控

### 低优先级（可选实现）
6. **中断控制接口** - 高级功能

## 实现建议

### 阶段一：核心接口实现
1. 实现队列管理接口
2. 实现时间管理接口
3. 定义统一的数据类型

### 阶段二：扩展接口实现
4. 实现列表管理接口
5. 扩展任务管理接口

### 阶段三：优化和测试
6. 实现中断控制接口
7. 全面测试和优化

## 总结

VFB模块需要抽象的主要接口包括：
- **队列管理**：4个核心接口
- **列表管理**：10个操作接口
- **时间管理**：2个转换接口
- **任务管理**：3个扩展接口
- **数据类型**：5个类型定义
- **中断控制**：3个控制接口

总计需要抽象约27个接口，其中队列管理接口是最高优先级，必须首先实现。

---

## POSIX 对应关系与建议实现

为在 Linux/Posix 环境（如 `apps/linux_demo`）下运行，建议在 Rte 层提供面向 POSIX 的实现。本节给出各抽象接口到 POSIX 的参考映射与注意事项。

### 1) 队列管理（os_queue_*）

- 建议优先使用 `mqueue.h`（POSIX 消息队列），如系统/权限不便，可回退到 `pthread_mutex` + `pthread_cond` + 环形缓冲实现。

参考类型与API（mq 优先）：
```c
#include <mqueue.h>

typedef struct {
    mqd_t mq;                // POSIX message queue descriptor
    size_t item_size;        // 固定消息大小
    char name[64];           // 队列名
} OsQueue_t;

OsQueue_t* os_queue_create(size_t queue_length, size_t item_size, const char* pName);
ssize_t    os_queue_destroy(OsQueue_t* pQueue);
ssize_t    os_queue_send(OsQueue_t* pQueue, const void* pItem, size_t timeout_ms);
ssize_t    os_queue_send_isr(OsQueue_t* pQueue, const void* pItem); // 在POSIX中等价于非阻塞发送
ssize_t    os_queue_receive(OsQueue_t* pQueue, void* pItem, size_t timeout_ms);
ssize_t    os_queue_get_count(OsQueue_t* pQueue); // mq_getattr().mq_curmsgs
ssize_t    os_queue_get_space(OsQueue_t* pQueue); // mq_attr.mq_maxmsg - mq_curmsgs
```

实现要点：
- `os_queue_create` 使用 `mq_open()` + `mq_setattr()`，注意 `O_CREAT|O_RDWR`，以及 `mq_msgsize=item_size`，`mq_maxmsg=queue_length`。
- 带超时的发送/接收通过 `mq_timedsend` / `mq_timedreceive` 实现，使用 `clock_gettime(CLOCK_REALTIME, &ts)` 计算绝对超时。
- 不想依赖系统队列命名空间时，使用 `pthread` + `cond` 的环形队列实现，语义保持一致，`timeout_ms` 用 `pthread_cond_timedwait`。

### 2) 列表管理（os_list_*）

- 可选：使用 `<sys/queue.h>` 提供的 `TAILQ`/`LIST` 宏；或自定义轻量双向链表。

参考类型与API（以自定义双向链表为例）：
```c
typedef struct OsListItem {
    struct OsListItem* prev;
    struct OsListItem* next;
    uintptr_t value; // 存放指针/句柄
} OsListItem_t;

typedef struct OsList {
    OsListItem_t* head;
    OsListItem_t* tail;
    size_t length;
} OsList_t;

void       os_list_initialise(OsList_t* pList);
void       os_list_item_initialise(OsListItem_t* pItem);
void       os_list_insert_end(OsList_t* pList, OsListItem_t* pItem);
void       os_list_insert_head(OsList_t* pList, OsListItem_t* pItem);
void       os_list_remove_item(OsList_t* pList, OsListItem_t* pItem);
OsListItem_t* os_list_get_head_entry(OsList_t* pList);
OsListItem_t* os_list_get_end_marker(OsList_t* pList); // 可返回NULL或虚尾标记
OsListItem_t* os_list_get_next(OsListItem_t* pItem);
size_t     os_list_get_length(OsList_t* pList);
uintptr_t  os_list_item_get_value(OsListItem_t* pItem);
void       os_list_item_set_value(OsListItem_t* pItem, uintptr_t value);
```

实现要点：
- 与 FreeRTOS List 语义保持一致，仅抽象最小必要语义（head、end、next、length、value）。

### 3) 时间管理（os_ms_to_ticks / os_ticks_to_ms）

```c
#include <time.h>

uint32_t os_ms_to_ticks(uint32_t ms);   // 在POSIX下可直接返回ms，或基于统一时基转换
uint32_t os_ticks_to_ms(uint32_t ticks);
```

实现要点：
- 若统一采用毫秒为“tick”，两函数可为恒等映射。
- 若需要系统节拍，定义 `CONFIG_TICKS_PER_SECOND`，按 \( ticks = ms * TPS / 1000 \) 转换。
- 基于 `clock_gettime(CLOCK_MONOTONIC, ...)` 实现毫秒时间源。

### 4) 任务管理（os_thread_*）

```c
#include <pthread.h>

typedef pthread_t OsThread_t;

OsThread_t* os_thread_get_current(void); // 返回线程自身指针或封装对象
ssize_t     os_thread_get_name(OsThread_t* pThread, char* pName, size_t name_length); // pthread_getname_np
ssize_t     os_thread_get_priority(OsThread_t* pThread); // pthread_getschedparam
```

注意：
- `pthread_getname_np` 在部分平台不可用时，需降级为空名或自维护名称表。

### 5) 数据类型抽象

```c
typedef void*     OsQueueHandle;  // 若使用mq，等价于 OsQueue_t*
typedef void*     OsListHandle;   // 等价于 OsList_t*
typedef void*     OsListItemHandle; // 等价于 OsListItem_t*
typedef uint32_t  OsTick_t;
typedef int       OsResult_t;
```

### 6) 中断控制接口（os_isr_context_*）

POSIX 无真实中断上下文，建议采用占位实现：
```c
typedef struct {
    int higher_priority_task_woken; // 固定为0，或基于事件触发语义设置
} OsIsrContext_t;

void os_isr_context_init(OsIsrContext_t* pContext);   // 清零
int  os_isr_context_get_woken(OsIsrContext_t* pContext); // 直接返回字段
```

---

## 在 vfb.c 中的迁移建议

- 将直接使用 `pdMS_TO_TICKS(x)` 的位置，替换为统一封装 `vfb_ms_to_ticks(x)`，由 Rte 的 `os_ms_to_ticks(x)` 提供，POSIX 下可恒等为毫秒。
- 将 `xQueueCreate/Send/Receive/FromISR` 等替换为 `os_queue_*`，并在 Rte 的 POSIX 端用 `mqueue` 或 `pthread` 实现。
- 列表相关 `List_t/ListItem_t` 建议切换到 `os_list_*`，保持 `value` 存放 `QueueHandle`（或 `OsQueueHandle`）句柄的约定。

这样，`vfb.c` 不再直接依赖 FreeRTOS 头文件，可在 FreeRTOS 与 POSIX 两端通过 Rte 选择后端实现。
