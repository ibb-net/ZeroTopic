#ifndef OBJ_DICT_MEMPOOL_H_
#define OBJ_DICT_MEMPOOL_H_

#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>
#include "obj_dict_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#if OBJ_DICT_MEMPOOL_ENABLE

/* 内存池句柄（前向声明） */
typedef struct obj_dict_mempool obj_dict_mempool_t;

/*
 * @brief 创建内存池
 * @param block_size 每个块的大小（字节）
 * @param block_count 块数量
 * @return 内存池句柄，失败返回NULL
 */
obj_dict_mempool_t* obj_dict_mempool_create(size_t block_size, size_t block_count);

/*
 * @brief 销毁内存池
 * @param pool 内存池句柄
 */
void obj_dict_mempool_destroy(obj_dict_mempool_t* pool);

/*
 * @brief 从内存池分配内存
 * @param pool 内存池句柄
 * @param size 需要的大小（如果大于块大小，返回NULL）
 * @return 分配的内存指针，失败返回NULL
 */
void* obj_dict_mempool_alloc(obj_dict_mempool_t* pool, size_t size);

/*
 * @brief 释放内存到内存池
 * @param pool 内存池句柄
 * @param ptr 内存指针
 */
void obj_dict_mempool_free(obj_dict_mempool_t* pool, void* ptr);

/*
 * @brief 获取内存池统计信息
 * @param pool 内存池句柄
 * @param total_blocks 输出总块数
 * @param free_blocks 输出空闲块数
 * @param used_blocks 输出已使用块数
 * @return 0成功，-1失败
 */
int obj_dict_mempool_get_stats(obj_dict_mempool_t* pool, 
                                size_t* total_blocks, 
                                size_t* free_blocks, 
                                size_t* used_blocks);

#endif /* OBJ_DICT_MEMPOOL_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* OBJ_DICT_MEMPOOL_H_ */

