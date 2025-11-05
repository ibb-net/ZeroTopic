#include "obj_dict_mempool.h"
#include "../../Rte/inc/os_heap.h"
#include "../../Rte/inc/os_semaphore.h"
#include <string.h>
#include <stdatomic.h>

#if OBJ_DICT_MEMPOOL_ENABLE

/* 内存池块结构 */
typedef struct {
    void* ptr;          /* 内存块指针 */
    uint8_t in_use;     /* 是否使用中（简化实现，原子操作） */
} mempool_block_t;

/* 内存池结构 */
struct obj_dict_mempool {
    mempool_block_t* blocks;    /* 块数组 */
    size_t block_size;          /* 每个块大小 */
    size_t block_count;         /* 块数量 */
    void* buffer;               /* 预分配的内存缓冲区 */
    OsSemaphore_t* lock;        /* 线程安全锁 */
#if OBJ_DICT_MEMPOOL_ENABLE_STATS
    atomic_size_t used_count;   /* 已使用块数（原子操作） */
#endif
};

/*
 * @brief 创建内存池
 */
obj_dict_mempool_t* obj_dict_mempool_create(size_t block_size, size_t block_count) {
    if (block_size == 0 || block_count == 0) return NULL;

    obj_dict_mempool_t* pool = (obj_dict_mempool_t*)os_malloc(sizeof(obj_dict_mempool_t));
    if (!pool) return NULL;
    memset(pool, 0, sizeof(obj_dict_mempool_t));

    pool->block_size = block_size;
    pool->block_count = block_count;

    /* 分配块数组 */
    pool->blocks = (mempool_block_t*)os_malloc(sizeof(mempool_block_t) * block_count);
    if (!pool->blocks) {
        os_free(pool);
        return NULL;
    }
    memset(pool->blocks, 0, sizeof(mempool_block_t) * block_count);

    /* 预分配内存缓冲区 */
    pool->buffer = os_malloc(block_size * block_count);
    if (!pool->buffer) {
        os_free(pool->blocks);
        os_free(pool);
        return NULL;
    }
    memset(pool->buffer, 0, block_size * block_count);

    /* 初始化块数组，将内存分段分配给各个块 */
    uint8_t* buf_ptr = (uint8_t*)pool->buffer;
    for (size_t i = 0; i < block_count; ++i) {
        pool->blocks[i].ptr = buf_ptr + i * block_size;
        pool->blocks[i].in_use = 0;
    }

#if OBJ_DICT_MEMPOOL_ENABLE_STATS
    atomic_init(&pool->used_count, 0);
#endif

    /* 创建锁（互斥锁初始值为1） */
    pool->lock = os_semaphore_create(1, "obj_dict_mempool_lock");
    if (!pool->lock) {
        os_free(pool->buffer);
        os_free(pool->blocks);
        os_free(pool);
        return NULL;
    }
    /* 初始化互斥锁为可用状态 */
    os_semaphore_give(pool->lock);

    return pool;
}

/*
 * @brief 销毁内存池
 */
void obj_dict_mempool_destroy(obj_dict_mempool_t* pool) {
    if (!pool) return;

    if (pool->lock) {
        os_semaphore_destroy(pool->lock);
    }
    if (pool->buffer) {
        os_free(pool->buffer);
    }
    if (pool->blocks) {
        os_free(pool->blocks);
    }
    os_free(pool);
}

/*
 * @brief 从内存池分配内存
 */
void* obj_dict_mempool_alloc(obj_dict_mempool_t* pool, size_t size) {
    if (!pool || size == 0 || size > pool->block_size) return NULL;
    if (os_semaphore_take(pool->lock, 100) < 0) return NULL;

    /* 查找空闲块 */
    for (size_t i = 0; i < pool->block_count; ++i) {
        if (!pool->blocks[i].in_use) {
            pool->blocks[i].in_use = 1;
#if OBJ_DICT_MEMPOOL_ENABLE_STATS
            atomic_fetch_add_explicit(&pool->used_count, 1, memory_order_relaxed);
#endif
            os_semaphore_give(pool->lock);
            return pool->blocks[i].ptr;
        }
    }

    os_semaphore_give(pool->lock);
    return NULL;  /* 内存池已满 */
}

/*
 * @brief 释放内存到内存池
 */
void obj_dict_mempool_free(obj_dict_mempool_t* pool, void* ptr) {
    if (!pool || !ptr) return;
    if (os_semaphore_take(pool->lock, 100) < 0) return;

    /* 查找对应的块 */
    for (size_t i = 0; i < pool->block_count; ++i) {
        if (pool->blocks[i].ptr == ptr) {
            if (pool->blocks[i].in_use) {
                pool->blocks[i].in_use = 0;
                /* 清零内存（可选，用于调试） */
                memset(ptr, 0, pool->block_size);
#if OBJ_DICT_MEMPOOL_ENABLE_STATS
                atomic_fetch_sub_explicit(&pool->used_count, 1, memory_order_relaxed);
#endif
            }
            break;
        }
    }

    os_semaphore_give(pool->lock);
}

/*
 * @brief 获取内存池统计信息
 */
int obj_dict_mempool_get_stats(obj_dict_mempool_t* pool, 
                                size_t* total_blocks, 
                                size_t* free_blocks, 
                                size_t* used_blocks) {
    if (!pool) return -1;
    if (os_semaphore_take(pool->lock, 100) < 0) return -1;

    if (total_blocks) *total_blocks = pool->block_count;

    size_t used = 0;
    for (size_t i = 0; i < pool->block_count; ++i) {
        if (pool->blocks[i].in_use) {
            used++;
        }
    }

    if (free_blocks) *free_blocks = pool->block_count - used;
    if (used_blocks) *used_blocks = used;

    os_semaphore_give(pool->lock);
    return 0;
}

#endif /* OBJ_DICT_MEMPOOL_ENABLE */

