#include <string.h>

#include "ring_buffer.h"
#include "../../Rte/inc/os_heap.h"
#include "../../Rte/inc/os_semaphore.h"

/* ============================================================
 * 内部函数声明 (Internal Functions Declaration)
 * ============================================================ */

#if RING_BUFFER_REQUIRE_POWER_OF_TWO
static int __is_power_of_two(size_t x);
#endif
static size_t __rb_mask(const ring_buffer_t* rb, size_t v);
static int __rb_push(ring_buffer_t* rb, const void* item);
static int __rb_pop(ring_buffer_t* rb, void* item_out);

/* ============================================================
 * 函数实现 (Function Implementation)
 * ============================================================ */

/*
 * @brief 判断一个数是否为2的幂
 * @param x 待判断数值
 * @return 非0表示是2的幂，0表示否
 */
#if RING_BUFFER_REQUIRE_POWER_OF_TWO
static int __is_power_of_two(size_t x) {
    return x && ((x & (x - 1)) == 0);
}
#endif

/*
 * @brief 将线性索引映射为环形索引
 * @param rb 环形队列句柄
 * @param v  线性索引值
 * @return 环形下标（0..capacity-1）
 */
static size_t __rb_mask(const ring_buffer_t* rb, size_t v) {
#if RING_BUFFER_REQUIRE_POWER_OF_TWO
    return v & (rb->capacity - 1);
#else
    return v % rb->capacity;
#endif
}

/*
 * @brief 创建环形队列
 * @param capacity 元素容量（建议为2的幂以获得更佳性能）
 * @param item_size 单个元素大小（字节），为0则采用默认
 * @return 成功返回队列指针，失败返回NULL
 */
ring_buffer_t* ring_buffer_create(size_t capacity, size_t item_size) {
    if (item_size == 0) {
        item_size = RING_BUFFER_DEFAULT_ITEM_SIZE;
    }
#if RING_BUFFER_REQUIRE_POWER_OF_TWO
    if (!__is_power_of_two(capacity)) {
        return NULL;
    }
#endif
    ring_buffer_t* rb = (ring_buffer_t*)os_malloc(sizeof(ring_buffer_t));
    if (!rb) return NULL;
    memset(rb, 0, sizeof(*rb));

    rb->buffer = (uint8_t*)os_malloc(capacity * item_size);
    if (!rb->buffer) {
        os_free(rb);
        return NULL;
    }
    rb->capacity  = capacity;
    rb->item_size = item_size;
    atomic_init(&rb->write_idx, 0);
    atomic_init(&rb->read_idx, 0);

#if RING_BUFFER_ENABLE_BLOCKING
    rb->sem_items  = os_semaphore_create(0, "rb_items");
    rb->sem_spaces = os_semaphore_create(capacity, "rb_spaces");
    if (!rb->sem_items || !rb->sem_spaces) {
        if (rb->sem_items) {
            os_semaphore_destroy(rb->sem_items);
        }
        if (rb->sem_spaces) {
            os_semaphore_destroy(rb->sem_spaces);
        }
        os_free(rb->buffer);
        os_free(rb);
        return NULL;
    }
#endif
    return rb;
}

/*
 * @brief 销毁环形队列并释放资源
 * @param rb 环形队列句柄
 */
void ring_buffer_destroy(ring_buffer_t* rb) {
    if (!rb) return;
#if RING_BUFFER_ENABLE_BLOCKING
    if (rb->sem_items)  os_semaphore_destroy(rb->sem_items);
    if (rb->sem_spaces) os_semaphore_destroy(rb->sem_spaces);
#endif
    if (rb->buffer) os_free(rb->buffer);
    os_free(rb);
}

/*
 * @brief 获取当前已存元素个数
 * @param rb 环形队列句柄
 * @return 已存元素数量
 */
size_t ring_buffer_get_count(const ring_buffer_t* rb) {
    /* 使用memory_order_relaxed在SPSC场景下提供最佳性能 */
    size_t w = atomic_load_explicit(&rb->write_idx, memory_order_relaxed);
    size_t r = atomic_load_explicit(&rb->read_idx, memory_order_relaxed);
    return (w - r);
}

/*
 * @brief 获取当前可用空位个数
 * @param rb 环形队列句柄
 * @return 可写入的空位数量
 */
size_t ring_buffer_get_space(const ring_buffer_t* rb) {
    return rb->capacity - ring_buffer_get_count(rb);
}

/*
 * @brief 重置队列读写指针并重置信号量
 * @param rb 环形队列句柄
 */
void ring_buffer_reset(ring_buffer_t* rb) {
    atomic_store_explicit(&rb->write_idx, (size_t)0, memory_order_release);
    atomic_store_explicit(&rb->read_idx, (size_t)0, memory_order_release);
#if RING_BUFFER_ENABLE_BLOCKING
    /* 重置信号量：简化起见，销毁重建 */
    if (rb->sem_items)  os_semaphore_destroy(rb->sem_items);
    if (rb->sem_spaces) os_semaphore_destroy(rb->sem_spaces);
    rb->sem_items  = os_semaphore_create(0, "rb_items");
    rb->sem_spaces = os_semaphore_create(rb->capacity, "rb_spaces");
#endif
}

/*
 * @brief 非阻塞入队内部实现
 * @param rb 环形队列句柄
 * @param item 待写入元素指针
 * @return 0成功，-1队列满
 */
static int __rb_push(ring_buffer_t* rb, const void* item) {
    if (ring_buffer_get_space(rb) == 0) {
        return -1;
    }
    /* 使用memory_order_acquire读取，确保数据可见性 */
    size_t w = atomic_load_explicit(&rb->write_idx, memory_order_acquire);
    size_t pos = __rb_mask(rb, w);
    memcpy(rb->buffer + pos * rb->item_size, item, rb->item_size);
    /* 使用memory_order_release写入，确保数据写入后可见 */
    atomic_store_explicit(&rb->write_idx, w + 1, memory_order_release);
    return 0;
}

/*
 * @brief 非阻塞出队内部实现
 * @param rb 环形队列句柄
 * @param item_out 输出元素缓冲区指针
 * @return 0成功，-1队列空
 */
static int __rb_pop(ring_buffer_t* rb, void* item_out) {
    if (ring_buffer_get_count(rb) == 0) {
        return -1;
    }
    /* 使用memory_order_acquire读取，确保数据可见性 */
    size_t r = atomic_load_explicit(&rb->read_idx, memory_order_acquire);
    size_t pos = __rb_mask(rb, r);
    memcpy(item_out, rb->buffer + pos * rb->item_size, rb->item_size);
    /* 使用memory_order_release写入，确保数据写入后可见 */
    atomic_store_explicit(&rb->read_idx, r + 1, memory_order_release);
    return 0;
}

/*
 * @brief 非阻塞写入一个元素
 * @param rb 环形队列句柄
 * @param item 元素指针（大小等于创建时item_size）
 * @return 0成功，-1失败（队列满或参数错误）
 */
ssize_t ring_buffer_write(ring_buffer_t* rb, const void* item) {
    if (!rb || !item) return -1;
    return __rb_push(rb, item);
}

/*
 * @brief 非阻塞读取一个元素
 * @param rb 环形队列句柄
 * @param item_out 输出缓冲区指针（大小等于创建时item_size）
 * @return 0成功，-1失败（队列空或参数错误）
 */
ssize_t ring_buffer_read(ring_buffer_t* rb, void* item_out) {
    if (!rb || !item_out) return -1;
    return __rb_pop(rb, item_out);
}

#if RING_BUFFER_ENABLE_BLOCKING
/*
 * @brief 阻塞式写入一个元素
 * @param rb 环形队列句柄
 * @param item 元素指针
 * @param timeout_ms 超时毫秒（0立即返回，UINT32_MAX表示永久等待）
 * @return 0成功，-1超时或失败
 */
ssize_t ring_buffer_write_blocking(ring_buffer_t* rb, const void* item, uint32_t timeout_ms) {
    if (!rb || !item) return -1;
    if (os_semaphore_take(rb->sem_spaces, timeout_ms) < 0) {
        return -1;
    }
    int rc = __rb_push(rb, item);
    if (rc == 0) {
        (void)os_semaphore_give(rb->sem_items);
    } else {
        /* 写失败(理论上不应发生)，归还space */
        (void)os_semaphore_give(rb->sem_spaces);
    }
    return rc;
}

/*
 * @brief 阻塞式读取一个元素
 * @param rb 环形队列句柄
 * @param item_out 输出缓冲区
 * @param timeout_ms 超时毫秒（0立即返回，UINT32_MAX表示永久等待）
 * @return 0成功，-1超时或失败
 */
ssize_t ring_buffer_read_blocking(ring_buffer_t* rb, void* item_out, uint32_t timeout_ms) {
    if (!rb || !item_out) return -1;
    if (os_semaphore_take(rb->sem_items, timeout_ms) < 0) {
        return -1;
    }
    int rc = __rb_pop(rb, item_out);
    if (rc == 0) {
        (void)os_semaphore_give(rb->sem_spaces);
    } else {
        /* 读失败(理论上不应发生)，归还items */
        (void)os_semaphore_give(rb->sem_items);
    }
    return rc;
}
#endif

#if RING_BUFFER_ENABLE_ISR
/*
 * @brief ISR上下文写入一个元素
 * @param rb 环形队列句柄
 * @param item 元素指针
 * @return 0成功，-1失败（队列满或参数错误）
 */
ssize_t ring_buffer_write_isr(ring_buffer_t* rb, const void* item) {
    if (!rb || !item) return -1;
    if (ring_buffer_get_space(rb) == 0) return -1;
    /* ISR路径使用relaxed模式，无需acquire因为ISR不会被抢占 */
    size_t w = atomic_load_explicit(&rb->write_idx, memory_order_relaxed);
    size_t pos = __rb_mask(rb, w);
    memcpy(rb->buffer + pos * rb->item_size, item, rb->item_size);
    /* 使用release确保ISR写入对其他线程可见 */
    atomic_store_explicit(&rb->write_idx, w + 1, memory_order_release);
#if RING_BUFFER_ENABLE_BLOCKING
    (void)os_semaphore_give_isr(rb->sem_items);
#endif
    return 0;
}
#endif


