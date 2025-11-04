#include <string.h>
#include "obj_dict_storage.h"
#include "../../Rte/inc/os_heap.h"

/* ============================================================
 * 内部函数声明 (Internal Functions Declaration)
 * ============================================================ */

static int __ram_init(void* storage, size_t size);
static ssize_t __ram_read(obj_dict_key_t key, void* data, size_t size);
static int __ram_write(obj_dict_key_t key, const void* data, size_t size);
static int __ram_erase(obj_dict_key_t key);
static int __flash_init(void* storage, size_t size);
static ssize_t __flash_read(obj_dict_key_t key, void* data, size_t size);
static int __flash_write(obj_dict_key_t key, const void* data, size_t size);
static int __flash_erase(obj_dict_key_t key);

/* ============================================================
 * 函数实现 (Function Implementation)
 * ============================================================ */

/* ---------------- RAM 后端实现 ---------------- */

static int __ram_init(void* storage, size_t size) {
    /*
     * @brief 初始化RAM后端
     * @param storage obj_dict_ram_storage_t*
     * @param size    预期容量（可忽略，按storage->capacity生效）
     */
    obj_dict_ram_storage_t* ram = (obj_dict_ram_storage_t*)storage;
    if (!ram) return -1;
    if (ram->base && ram->capacity > 0) {
        memset(ram->base, 0, ram->capacity);
    }
    (void)size;
    return 0;
}

static ssize_t __ram_read(obj_dict_key_t key, void* data, size_t size) {
    /*
     * @brief 简化参考：按key大小直接返回失败；
     * 实际项目可做key→offset映射，这里仅提供占位接口（建议结合具体布局实现）。
     */
    (void)key; (void)data; (void)size;
    return -1;
}

static int __ram_write(obj_dict_key_t key, const void* data, size_t size) {
    (void)key; (void)data; (void)size;
    return -1;
}

static int __ram_erase(obj_dict_key_t key) {
    (void)key;
    return -1;
}

static const obj_dict_storage_ops_t g_ram_ops = {
    .init  = __ram_init,
    .read  = __ram_read,
    .write = __ram_write,
    .erase = __ram_erase,
};

const obj_dict_storage_ops_t* obj_dict_ram_storage_ops(void) {
    return &g_ram_ops;
}

/* ---------------- Flash 后端占位 ---------------- */

static int __flash_init(void* storage, size_t size) {
    (void)storage; (void)size;
    return 0;
}

static ssize_t __flash_read(obj_dict_key_t key, void* data, size_t size) {
    (void)key; (void)data; (void)size;
    return -1;
}

static int __flash_write(obj_dict_key_t key, const void* data, size_t size) {
    (void)key; (void)data; (void)size;
    return -1;
}

static int __flash_erase(obj_dict_key_t key) {
    (void)key;
    return -1;
}

static const obj_dict_storage_ops_t g_flash_ops = {
    .init  = __flash_init,
    .read  = __flash_read,
    .write = __flash_write,
    .erase = __flash_erase,
};

const obj_dict_storage_ops_t* obj_dict_flash_storage_ops(void) {
    return &g_flash_ops;
}


