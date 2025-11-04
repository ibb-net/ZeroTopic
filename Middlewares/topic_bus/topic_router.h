#ifndef TOPIC_ROUTER_H_
#define TOPIC_ROUTER_H_

#include <stdint.h>
#include <stddef.h>
#include "topic_bus_config.h"
#include "../obj_dict/obj_dict.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Router类型 */
typedef enum {
    TOPIC_ROUTER_TYPE_VFB,      /* VFB消息队列广播 */
    TOPIC_ROUTER_TYPE_CUSTOM,   /* 自定义服务处理 */
} topic_router_type_t;

/* Router回调函数类型 */
typedef int (*topic_router_callback_t)(uint16_t topic_id, const void* data, size_t data_len, void* user_data);

/* Router条目结构 */
typedef struct {
    topic_router_type_t type;           /* Router类型 */
    union {
        obj_dict_key_t vfb_event_key;   /* VFB事件键（用于VFB类型） */
        topic_router_callback_t custom_cb;  /* 自定义回调（用于自定义类型） */
    } u;
    void* user_data;                    /* 用户数据（用于匹配topic_id和回调参数） */
    void* callback_user_data;           /* 回调函数的用户数据（仅用于自定义类型） */
} topic_router_entry_t;

/* Topic Router结构 */
struct topic_router {
    topic_router_entry_t* routers;      /* Router条目数组 */
    size_t max_routers;                 /* 最大Router数量 */
    size_t router_count;                /* 当前Router数量 */
};
typedef struct topic_router topic_router_t;

/*
 * @brief 初始化Topic Router
 * @param router Topic Router指针
 * @param routers Router条目数组
 * @param max_routers 最大Router数量
 * @return 0成功，-1失败
 */
int topic_router_init(topic_router_t* router, topic_router_entry_t* routers, size_t max_routers);

/*
 * @brief 添加VFB Router
 * @param router Topic Router指针
 * @param topic_id Topic ID
 * @param vfb_event_key VFB事件键
 * @return 0成功，-1失败
 */
int topic_router_add_vfb(topic_router_t* router, uint16_t topic_id, obj_dict_key_t vfb_event_key);

/*
 * @brief 添加自定义Router
 * @param router Topic Router指针
 * @param topic_id Topic ID（用于索引，当前版本暂不支持）
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return 0成功，-1失败
 */
int topic_router_add_custom(topic_router_t* router, uint16_t topic_id,
                             topic_router_callback_t callback, void* user_data);

/*
 * @brief 处理Topic触发（当Topic触发时调用）
 * @param router Topic Router指针
 * @param topic_id Topic ID
 * @param data 数据指针
 * @param data_len 数据长度
 * @return 0成功，-1失败
 */
int topic_router_route(topic_router_t* router, uint16_t topic_id,
                        const void* data, size_t data_len);

/*
 * @brief 移除Router
 * @param router Topic Router指针
 * @param topic_id Topic ID
 * @param type Router类型
 * @return 0成功，-1失败
 */
int topic_router_remove(topic_router_t* router, uint16_t topic_id, topic_router_type_t type);

#ifdef __cplusplus
}
#endif

#endif /* TOPIC_ROUTER_H_ */

