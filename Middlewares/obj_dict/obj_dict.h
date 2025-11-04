#ifndef OBJ_DICT_H_
#define OBJ_DICT_H_

#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

#include "../../Rte/inc/os_semaphore.h"
#include "../../Rte/inc/os_timestamp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t obj_dict_key_t; /* 与vfb_event_t一致 */

typedef struct {
    obj_dict_key_t key;          /* 键(ID) */
    void*          value;        /* 数据缓冲 */
    size_t         value_len;    /* 数据长度 */
    uint64_t       timestamp_us; /* 时间戳(微秒) */
    atomic_uint_fast32_t version; /* 版本号（C11原子操作） */
    uint8_t        flags;        /* 标志 */
} obj_dict_entry_t;

typedef struct {
    obj_dict_entry_t* entries;   /* 条目数组 */
    size_t            max_keys;  /* 最大键数量 */
    OsSemaphore_t*    lock;      /* 线程安全 */
} obj_dict_t;

/* 初始化对象字典：由调用者提供条目数组及容量 */
int obj_dict_init(obj_dict_t* dict, obj_dict_entry_t* entry_array, size_t max_keys);

/* 设置键的值(拷贝写入)：若键不存在则占用空槽；返回0成功 */
int obj_dict_set(obj_dict_t* dict, obj_dict_key_t key, const void* data, size_t len, uint8_t flags);

/* 获取键的值(拷贝读取)：返回写入字节数，<0失败；可返回时间戳与版本 */
ssize_t obj_dict_get(obj_dict_t* dict, obj_dict_key_t key, void* out, size_t out_cap,
                     uint64_t* ts_us, uint32_t* version, uint8_t* flags);

/* 简易遍历：返回第一个非空条目索引，之后传入next_from继续 */
int obj_dict_iterate(obj_dict_t* dict, int next_from /* -1开始 */);

#ifdef __cplusplus
}
#endif

#endif /* OBJ_DICT_H_ */

