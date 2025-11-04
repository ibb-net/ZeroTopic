#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

#include "ring_buffer_config.h"
#include "../../Rte/inc/os_semaphore.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t*            buffer;        /* 数据缓冲区起始地址 */
    size_t              capacity;      /* 元素容量(个) */
    atomic_size_t       write_idx;     /* 写索引（C11原子操作） */
    atomic_size_t       read_idx;      /* 读索引（C11原子操作） */
    size_t              item_size;     /* 单个元素大小(字节) */
#if RING_BUFFER_ENABLE_BLOCKING
    OsSemaphore_t*      sem_items;     /* 计数：可读项 */
    OsSemaphore_t*      sem_spaces;    /* 计数：可写空位 */
#endif
} ring_buffer_t;

/* 创建/销毁 */
ring_buffer_t* ring_buffer_create(size_t capacity, size_t item_size);
void ring_buffer_destroy(ring_buffer_t* rb);

/* 非阻塞API：成功返回0，失败返回-1 */
ssize_t ring_buffer_write(ring_buffer_t* rb, const void* item);
ssize_t ring_buffer_read(ring_buffer_t* rb, void* item_out);

/* 阻塞API：等待毫秒，UINT32_MAX表示无限等待。返回0成功，-1失败/超时 */
#if RING_BUFFER_ENABLE_BLOCKING
ssize_t ring_buffer_write_blocking(ring_buffer_t* rb, const void* item, uint32_t timeout_ms);
ssize_t ring_buffer_read_blocking(ring_buffer_t* rb, void* item_out, uint32_t timeout_ms);
#endif

/* ISR写入：仅在启用时可用 */
#if RING_BUFFER_ENABLE_ISR
ssize_t ring_buffer_write_isr(ring_buffer_t* rb, const void* item);
#endif

/* 查询/维护 */
size_t ring_buffer_get_count(const ring_buffer_t* rb);
size_t ring_buffer_get_space(const ring_buffer_t* rb);
void   ring_buffer_reset(ring_buffer_t* rb);

/* 性能测试入口（可选编译）*/
int ring_buffer_perf_test_main(void);

#ifdef __cplusplus
}
#endif

#endif /* RING_BUFFER_H_ */

