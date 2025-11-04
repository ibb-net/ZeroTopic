# ring_buffer 模块说明

## 概述
- MCU 友好的高效环形队列，支持非阻塞/阻塞/ISR 写路径；可作为 Topic/对象字典缓冲的基础设施。
- 采用固定元素大小（创建时指定），支持 2 的幂容量优化（可配置）。

## 目录结构
- `ring_buffer.h/.c`：核心实现
- `ring_buffer_config.h`：编译期配置
- `perf_test_ring_buffer.c`：功能与性能测试

## 配置
- `RING_BUFFER_ENABLE_BLOCKING`：1 启用阻塞 API
- `RING_BUFFER_ENABLE_ISR`：1 启用 `ring_buffer_write_isr`
- `RING_BUFFER_REQUIRE_POWER_OF_TWO`：1 要求容量为 2 的幂
- `RING_BUFFER_ENABLE_ZEROCOPY`：预留零拷贝能力开关

## 核心 API
```c
ring_buffer_t* ring_buffer_create(size_t capacity, size_t item_size);
void ring_buffer_destroy(ring_buffer_t* rb);
ssize_t ring_buffer_write(ring_buffer_t* rb, const void* item);
ssize_t ring_buffer_read(ring_buffer_t* rb, void* item_out);
ssize_t ring_buffer_write_blocking(ring_buffer_t* rb,const void* item,uint32_t timeout_ms);
ssize_t ring_buffer_read_blocking(ring_buffer_t* rb,void* item_out,uint32_t timeout_ms);
ssize_t ring_buffer_write_isr(ring_buffer_t* rb, const void* item);
size_t  ring_buffer_get_count(const ring_buffer_t* rb);
size_t  ring_buffer_get_space(const ring_buffer_t* rb);
void    ring_buffer_reset(ring_buffer_t* rb);
```

## 使用示例
```c
ring_buffer_t* rb = ring_buffer_create(1024, sizeof(uint32_t));
uint32_t v = 123;
ring_buffer_write(rb, &v);
ring_buffer_read(rb, &v);
ring_buffer_destroy(rb);
```

## 测试
- 在 `apps/linux_demo/config.h` 设置 `#define ENABLE_RING_BUFFER_TEST 1`
- 运行 linux_demo，输出包含：
  - 基础功能测试（满/空、顺序、阻塞超时）
  - 性能测试（多 item 大小与变长头场景）

## 与 microROS 的关系
- 可作为 microROS 传输适配或节点内部缓冲队列的实现基础。


