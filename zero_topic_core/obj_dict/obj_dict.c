#include <string.h>
#include "obj_dict.h"
#include "obj_dict_mempool.h"
#include "../../Rte/inc/os_heap.h"

/* ============================================================
 * 内部函数声明 (Internal Functions Declaration)
 * ============================================================ */

static obj_dict_entry_t* __find_entry(obj_dict_t* dict, obj_dict_key_t key);
static obj_dict_entry_t* __find_free_slot(obj_dict_t* dict);

/* ============================================================
 * 函数实现 (Function Implementation)
 * ============================================================ */

/*
 * @brief 查找指定key的条目
 * @param dict 字典句柄
 * @param key  目标键
 * @return 找到返回条目指针，否则返回NULL
 */
static obj_dict_entry_t* __find_entry(obj_dict_t* dict, obj_dict_key_t key) {
    for (size_t i = 0; i < dict->max_keys; ++i) {
        if (dict->entries[i].value != NULL && dict->entries[i].key == key) {
            return &dict->entries[i];
        }
    }
    return NULL;
}

/*
 * @brief 查找空闲条目槽位
 * @param dict 字典句柄
 * @return 返回空槽的条目指针，若无空槽返回NULL
 */
static obj_dict_entry_t* __find_free_slot(obj_dict_t* dict) {
    for (size_t i = 0; i < dict->max_keys; ++i) {
        if (dict->entries[i].value == NULL) {
            return &dict->entries[i];
        }
    }
    return NULL;
}

/*
 * @brief 初始化对象字典
 * @param dict 字典对象
 * @param entry_array 外部提供的条目数组（作为存储池）
 * @param max_keys 条目数组容量
 * @return 0成功，-1失败
 */
int obj_dict_init(obj_dict_t* dict, obj_dict_entry_t* entry_array, size_t max_keys) {
    if (!dict || !entry_array || max_keys == 0) return -1;
    dict->entries = entry_array;
    dict->max_keys = max_keys;
    memset(entry_array, 0, sizeof(obj_dict_entry_t) * max_keys);
    /* 初始化所有条目的原子版本号和引用计数 */
    for (size_t i = 0; i < max_keys; ++i) {
        atomic_init(&entry_array[i].version, 0);
        atomic_init(&entry_array[i].ref_count, 0);
    }
#if OBJ_DICT_MEMPOOL_ENABLE
    dict->mempool = NULL;  /* 默认不使用内存池 */
#endif
    dict->lock = os_semaphore_create(1, "obj_dict_lock");
    return (dict->lock != NULL) ? 0 : -1;
}

/*
 * @brief 初始化对象字典（带内存池）
 * @param dict 字典对象
 * @param entry_array 外部提供的条目数组
 * @param max_keys 条目数组容量
 * @param mempool_block_size 内存池块大小
 * @param mempool_block_count 内存池块数量
 * @return 0成功，-1失败
 */
int obj_dict_init_with_mempool(obj_dict_t* dict, obj_dict_entry_t* entry_array, size_t max_keys,
                                size_t mempool_block_size, size_t mempool_block_count) {
    if (obj_dict_init(dict, entry_array, max_keys) != 0) return -1;

#if OBJ_DICT_MEMPOOL_ENABLE
    dict->mempool = obj_dict_mempool_create(mempool_block_size, mempool_block_count);
    if (!dict->mempool) {
        /* 内存池创建失败，但字典已初始化，可以继续使用系统堆 */
        return 0;  /* 允许回退到系统堆 */
    }
#endif

    return 0;
}

/*
 * @brief 设置指定key对应的数据（拷贝写入）
 * @param dict 字典对象
 * @param key  键值（与事件枚举一致）
 * @param data 待写入数据指针（len>0时不可为NULL）
 * @param len  数据长度（字节）
 * @param flags 标志位（持久化/只读等）
 * @return 0成功，-1失败
 */
int obj_dict_set(obj_dict_t* dict, obj_dict_key_t key, const void* data, size_t len, uint8_t flags) {
    if (!dict || (!data && len > 0)) return -1;
    if (os_semaphore_take(dict->lock, 100) < 0) return -1;

    obj_dict_entry_t* e = __find_entry(dict, key);
    if (!e) {
        e = __find_free_slot(dict);
        if (!e) {
            os_semaphore_give(dict->lock);
            return -1;
        }
        e->key = key;
        atomic_init(&e->version, 0);
        atomic_init(&e->ref_count, 0);
    }

    /* 重分配缓冲 */
    if (e->value && e->value_len != len) {
        /* 释放旧内存 */
#if OBJ_DICT_MEMPOOL_ENABLE
        if (dict->mempool) {
            obj_dict_mempool_free(dict->mempool, e->value);
        } else {
            os_free(e->value);
        }
#else
        os_free(e->value);
#endif
        e->value = NULL;
        e->value_len = 0;
    }
    if (len > 0) {
        if (!e->value) {
            /* 从内存池或系统堆分配 */
#if OBJ_DICT_MEMPOOL_ENABLE
            if (dict->mempool && len <= OBJ_DICT_MEMPOOL_BLOCK_SIZE) {
                e->value = obj_dict_mempool_alloc(dict->mempool, len);
                if (!e->value) {
                    /* 内存池分配失败，回退到系统堆 */
                    e->value = os_malloc(len);
                }
            } else {
                e->value = os_malloc(len);
            }
#else
            e->value = os_malloc(len);
#endif
            if (!e->value) {
                os_semaphore_give(dict->lock);
                return -1;
            }
        }
        memcpy(e->value, data, len);
        e->value_len = len;
    } else {
        /* 长度为0表示清空数据 */
        if (e->value) {
#if OBJ_DICT_MEMPOOL_ENABLE
            if (dict->mempool) {
                obj_dict_mempool_free(dict->mempool, e->value);
            } else {
                os_free(e->value);
            }
#else
            os_free(e->value);
#endif
            e->value = NULL;
        }
        e->value_len = 0;
    }

    e->flags = flags;
    e->timestamp_us = os_monotonic_time_get_microsecond();
    /* 使用原子递增版本号，保证线程安全 */
    atomic_fetch_add_explicit(&e->version, 1, memory_order_release);

    os_semaphore_give(dict->lock);
    return 0;
}

/*
 * @brief 获取指定key对应的数据（拷贝读取）
 * @param dict 字典对象
 * @param key  键值
 * @param out  输出缓冲区指针（可为NULL表示仅查询元数据）
 * @param out_cap 输出缓冲区大小
 * @param ts_us 若非NULL，返回微秒时间戳
 * @param version 若非NULL，返回版本号
 * @param flags 若非NULL，返回标志位
 * @return 成功返回写入到out的字节数，失败返回-1
 */
ssize_t obj_dict_get(obj_dict_t* dict, obj_dict_key_t key, void* out, size_t out_cap,
                     uint64_t* ts_us, uint32_t* version, uint8_t* flags) {
    if (!dict) return -1;
    if (os_semaphore_take(dict->lock, 100) < 0) return -1;

    obj_dict_entry_t* e = __find_entry(dict, key);
    if (!e) {
        os_semaphore_give(dict->lock);
        return -1;
    }
    size_t n = 0;
    if (out && out_cap > 0 && e->value && e->value_len > 0) {
        n = (e->value_len <= out_cap) ? e->value_len : out_cap;
        memcpy(out, e->value, n);
    }
    if (ts_us) *ts_us = e->timestamp_us;
    /* 使用原子读取版本号，保证一致性 */
    if (version) *version = atomic_load_explicit(&e->version, memory_order_acquire);
    if (flags) *flags = e->flags;

    os_semaphore_give(dict->lock);
    return (ssize_t)n;
}

/*
 * @brief 迭代非空条目
 * @param dict 字典对象
 * @param next_from 上一次返回的索引（首次传-1）
 * @return 返回下一非空条目的索引，若无更多返回-1
 */
int obj_dict_iterate(obj_dict_t* dict, int next_from) {
    if (!dict) return -1;
    int start = next_from + 1;
    if (start < 0) start = 0;
    for (int i = start; i < (int)dict->max_keys; ++i) {
        if (dict->entries[i].value != NULL) return i;
    }
    return -1;
}

/*
 * @brief 增加引用计数（生命周期保护）
 * @param dict 字典对象
 * @param key  键值
 * @return 0成功，-1失败（键不存在）
 */
int obj_dict_retain(obj_dict_t* dict, obj_dict_key_t key) {
    if (!dict) return -1;
    if (os_semaphore_take(dict->lock, 100) < 0) return -1;

    obj_dict_entry_t* e = __find_entry(dict, key);
    if (!e) {
        os_semaphore_give(dict->lock);
        return -1;
    }

    /* 原子递增引用计数 */
    atomic_fetch_add_explicit(&e->ref_count, 1, memory_order_acq_rel);

    os_semaphore_give(dict->lock);
    return 0;
}

/*
 * @brief 减少引用计数（释放引用）
 * @param dict 字典对象
 * @param key  键值
 * @return 0成功，-1失败（键不存在）
 */
int obj_dict_release(obj_dict_t* dict, obj_dict_key_t key) {
    if (!dict) return -1;
    if (os_semaphore_take(dict->lock, 100) < 0) return -1;

    obj_dict_entry_t* e = __find_entry(dict, key);
    if (!e) {
        os_semaphore_give(dict->lock);
        return -1;
    }

    /* 原子递减引用计数 */
    uint32_t old_count = atomic_fetch_sub_explicit(&e->ref_count, 1, memory_order_acq_rel);
    
    /* 如果引用计数减到0，可以考虑清理（当前版本保留数据，由外部管理清理） */
    /* 注意：这里不自动清理，避免在回调期间数据被意外删除 */
    (void)old_count;

    os_semaphore_give(dict->lock);
    return 0;
}

/*
 * @brief 获取引用计数（调试用）
 * @param dict 字典对象
 * @param key  键值
 * @return 引用计数值，失败返回-1
 */
int32_t obj_dict_get_ref_count(obj_dict_t* dict, obj_dict_key_t key) {
    if (!dict) return -1;
    if (os_semaphore_take(dict->lock, 100) < 0) return -1;

    obj_dict_entry_t* e = __find_entry(dict, key);
    int32_t count = -1;
    if (e) {
        count = (int32_t)atomic_load_explicit(&e->ref_count, memory_order_acquire);
    }

    os_semaphore_give(dict->lock);
    return count;
}

/*
 * @brief 清理未使用的数据（引用计数为0且超过指定时间未更新）
 * @param dict 字典对象
 * @param timeout_us 超时时间（微秒），如果数据超过此时间未更新且引用计数为0，则清理
 * @return 清理的条目数量，失败返回-1
 */
int obj_dict_cleanup_unused(obj_dict_t* dict, uint64_t timeout_us) {
    if (!dict) return -1;
    if (os_semaphore_take(dict->lock, 100) < 0) return -1;

    int cleaned_count = 0;
    uint64_t now_us = os_monotonic_time_get_microsecond();

    for (size_t i = 0; i < dict->max_keys; ++i) {
        obj_dict_entry_t* e = &dict->entries[i];
        if (!e->value) continue;  /* 空槽跳过 */

        /* 检查引用计数 */
        uint32_t ref_count = atomic_load_explicit(&e->ref_count, memory_order_acquire);
        if (ref_count > 0) continue;  /* 有引用，不清理 */

        /* 检查时间戳 */
        uint64_t elapsed_us = (now_us >= e->timestamp_us) ? (now_us - e->timestamp_us) : 0;
        if (elapsed_us < timeout_us) continue;  /* 未超时，不清理 */

        /* 清理数据 */
        if (e->value) {
#if OBJ_DICT_MEMPOOL_ENABLE
            if (dict->mempool) {
                obj_dict_mempool_free(dict->mempool, e->value);
            } else {
                os_free(e->value);
            }
#else
            os_free(e->value);
#endif
            e->value = NULL;
            e->value_len = 0;
            e->key = 0;
            atomic_store_explicit(&e->version, 0, memory_order_release);
            atomic_store_explicit(&e->ref_count, 0, memory_order_release);
            cleaned_count++;
        }
    }

    os_semaphore_give(dict->lock);
    return cleaned_count;
}

/*
 * @brief 获取所有条目的引用计数统计
 * @param dict 字典对象
 * @param ref_counts 输出数组，存储每个条目的引用计数
 * @param max_count 输出数组大小
 * @return 实际填充的数量，失败返回-1
 */
int obj_dict_get_all_ref_counts(obj_dict_t* dict, int32_t* ref_counts, size_t max_count) {
    if (!dict || !ref_counts) return -1;
    if (os_semaphore_take(dict->lock, 100) < 0) return -1;

    size_t count = (max_count < dict->max_keys) ? max_count : dict->max_keys;
    for (size_t i = 0; i < count; ++i) {
        if (dict->entries[i].value) {
            ref_counts[i] = (int32_t)atomic_load_explicit(&dict->entries[i].ref_count, memory_order_acquire);
        } else {
            ref_counts[i] = -1;  /* 空槽标记为-1 */
        }
    }

    os_semaphore_give(dict->lock);
    return (int)count;
}


