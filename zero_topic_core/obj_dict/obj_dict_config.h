#ifndef OBJ_DICT_CONFIG_H_
#define OBJ_DICT_CONFIG_H_

/* 是否启用内存池优化 */
#ifndef OBJ_DICT_MEMPOOL_ENABLE
#define OBJ_DICT_MEMPOOL_ENABLE 1
#endif

/* 内存池默认块大小（字节） */
#ifndef OBJ_DICT_MEMPOOL_BLOCK_SIZE
#define OBJ_DICT_MEMPOOL_BLOCK_SIZE 256
#endif

/* 内存池默认块数量 */
#ifndef OBJ_DICT_MEMPOOL_BLOCK_COUNT
#define OBJ_DICT_MEMPOOL_BLOCK_COUNT 32
#endif

/* 是否启用内存池统计信息 */
#ifndef OBJ_DICT_MEMPOOL_ENABLE_STATS
#define OBJ_DICT_MEMPOOL_ENABLE_STATS 1
#endif

/* 是否启用引用计数（生命周期管理） */
#ifndef OBJ_DICT_ENABLE_REF_COUNT
#define OBJ_DICT_ENABLE_REF_COUNT 1
#endif

/* 是否启用自动清理（定时清理未使用的数据） */
#ifndef OBJ_DICT_ENABLE_AUTO_CLEANUP
#define OBJ_DICT_ENABLE_AUTO_CLEANUP 0  /* 默认关闭，需要手动调用清理函数 */
#endif

/* 默认清理超时时间（微秒） */
#ifndef OBJ_DICT_DEFAULT_CLEANUP_TIMEOUT_US
#define OBJ_DICT_DEFAULT_CLEANUP_TIMEOUT_US (60 * 1000 * 1000)  /* 60秒 */
#endif

#endif /* OBJ_DICT_CONFIG_H_ */

