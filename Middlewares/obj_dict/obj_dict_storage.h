#ifndef OBJ_DICT_STORAGE_H_
#define OBJ_DICT_STORAGE_H_

#include <stddef.h>
#include <stdint.h>
#include "obj_dict.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 存储后端操作集：RAM/Flash/多块可插拔 */
typedef struct {
    /* 初始化存储后端
     * @param storage 后端上下文指针
     * @param size    可用容量/字节（按实现语义）
     * @return 0成功，-1失败
     */
    int (*init)(void* storage, size_t size);

    /* 读取键对应的数据
     * @param key   键
     * @param data  输出缓冲区
     * @param size  输出缓冲区大小
     * @return 写入data的字节数，<0失败
     */
    ssize_t (*read)(obj_dict_key_t key, void* data, size_t size);

    /* 写入键对应的数据
     * @param key   键
     * @param data  数据指针
     * @param size  数据长度
     * @return 0成功，-1失败
     */
    int (*write)(obj_dict_key_t key, const void* data, size_t size);

    /* 擦除键对应的数据
     * @param key 键
     * @return 0成功，-1失败
     */
    int (*erase)(obj_dict_key_t key);
} obj_dict_storage_ops_t;

/* ---------------- RAM 后端（参考实现，可直接使用） ---------------- */

typedef struct {
    uint8_t* base;      /* 连续RAM缓冲区（可选），若NULL按key分配块 */
    size_t   capacity;  /* 总容量（字节） */
} obj_dict_ram_storage_t;

/* 获取RAM后端操作集 */
const obj_dict_storage_ops_t* obj_dict_ram_storage_ops(void);

/* ---------------- Flash 后端（占位，待对接具体驱动） ---------------- */

typedef struct {
    uintptr_t flash_base;  /* 物理地址/句柄 */
    size_t    sector_size; /* 扇区大小 */
    size_t    total_size;  /* 总大小 */
} obj_dict_flash_storage_t;

/* 获取Flash后端操作集（当前返回占位实现，写入/擦除返回-1） */
const obj_dict_storage_ops_t* obj_dict_flash_storage_ops(void);

#ifdef __cplusplus
}
#endif

#endif /* OBJ_DICT_STORAGE_H_ */



