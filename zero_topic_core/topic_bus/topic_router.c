#include <string.h>
#include "topic_router.h"
#include "../../Rte/inc/os_heap.h"
#include "../../Rte/inc/os_printf.h"

/* 检查是否支持VFB */
#ifdef __VFB_SERVER_H__
#define TOPIC_ROUTER_VFB_AVAILABLE 1
#include "../vfb/vfb.h"
#else
#define TOPIC_ROUTER_VFB_AVAILABLE 0
#endif

/*
 * @brief 初始化Topic Router
 * @param router Topic Router指针
 * @param routers Router条目数组
 * @param max_routers 最大Router数量
 * @return 0成功，-1失败
 */
int topic_router_init(topic_router_t* router, topic_router_entry_t* routers, size_t max_routers) {
    if (!router || !routers || max_routers == 0) return -1;
    
    memset(router, 0, sizeof(topic_router_t));
    router->routers = routers;
    router->max_routers = max_routers;
    router->router_count = 0;
    
    /* 初始化Router数组 */
    memset(routers, 0, sizeof(topic_router_entry_t) * max_routers);
    
    return 0;
}

/*
 * @brief 添加VFB Router
 * @param router Topic Router指针
 * @param topic_id Topic ID
 * @param vfb_event_key VFB事件键
 * @return 0成功，-1失败
 */
int topic_router_add_vfb(topic_router_t* router, uint16_t topic_id, obj_dict_key_t vfb_event_key) {
    if (!router || router->router_count >= router->max_routers) return -1;
    
    /* 查找空闲槽位 */
    for (size_t i = 0; i < router->max_routers; ++i) {
        if (router->routers[i].type == 0) {  /* 0表示未使用 */
            router->routers[i].type = TOPIC_ROUTER_TYPE_VFB;
            router->routers[i].u.vfb_event_key = vfb_event_key;
            router->routers[i].user_data = (void*)(uintptr_t)topic_id;
            router->router_count++;
            return 0;
        }
    }
    
    return -1;
}

/*
 * @brief 添加自定义Router
 * @param router Topic Router指针
 * @param topic_id Topic ID（用于索引，当前版本暂不支持）
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return 0成功，-1失败
 */
int topic_router_add_custom(topic_router_t* router, uint16_t topic_id,
                             topic_router_callback_t callback, void* user_data) {
    if (!router || !callback || router->router_count >= router->max_routers) return -1;
    
    /* 查找空闲槽位 */
    for (size_t i = 0; i < router->max_routers; ++i) {
        if (router->routers[i].type == 0) {  /* 0表示未使用 */
            router->routers[i].type = TOPIC_ROUTER_TYPE_CUSTOM;
            router->routers[i].u.custom_cb = callback;
            router->routers[i].user_data = (void*)(uintptr_t)topic_id;  /* 用于匹配topic_id */
            router->routers[i].callback_user_data = user_data;  /* 用于回调函数的用户数据 */
            router->router_count++;
            return 0;
        }
    }
    
    return -1;
}

/*
 * @brief 处理Topic触发（当Topic触发时调用）
 * @param router Topic Router指针
 * @param topic_id Topic ID
 * @param data 数据指针
 * @param data_len 数据长度
 * @return 0成功，-1失败
 */
int topic_router_route(topic_router_t* router, uint16_t topic_id,
                        const void* data, size_t data_len) {
    if (!router || !data || !router->routers) return -1;
    
    int routed = 0;
    
    /* 遍历所有Router，查找匹配的Topic */
    for (size_t i = 0; i < router->max_routers; ++i) {
        topic_router_entry_t* entry = &router->routers[i];
        if (!entry || entry->type == 0) continue;  /* 跳过未使用的 */
        
        /* 检查是否匹配该Topic（通过user_data中的topic_id判断） */
        if (!entry->user_data) continue;  /* user_data为NULL，跳过 */
        uint16_t router_topic_id = (uint16_t)(uintptr_t)entry->user_data;
        if (router_topic_id != topic_id) continue;
        
        switch (entry->type) {
            case TOPIC_ROUTER_TYPE_VFB: {
#if TOPIC_ROUTER_VFB_AVAILABLE
                /* 通过VFB广播 */
                obj_dict_key_t vfb_key = entry->u.vfb_event_key;
                if (vfb_send(vfb_key, 0, data, (uint16_t)data_len) == 0) {
                    routed++;
                }
#else
                /* VFB未启用，跳过 */
                (void)entry;
#endif
                break;
            }
            
            case TOPIC_ROUTER_TYPE_CUSTOM: {
                /* 调用自定义回调 */
                if (entry->u.custom_cb) {
                    void* user_data = entry->callback_user_data;  /* 可能为NULL，但回调函数应该能处理 */
                    if (entry->u.custom_cb(topic_id, data, data_len, user_data) == 0) {
                        routed++;
                    }
                }
                break;
            }
            
            default:
                break;
        }
    }
    
    return (routed > 0) ? 0 : -1;
}

/*
 * @brief 移除Router
 * @param router Topic Router指针
 * @param topic_id Topic ID
 * @param type Router类型
 * @return 0成功，-1失败
 */
int topic_router_remove(topic_router_t* router, uint16_t topic_id, topic_router_type_t type) {
    if (!router) return -1;
    
    for (size_t i = 0; i < router->max_routers; ++i) {
        topic_router_entry_t* entry = &router->routers[i];
        if (entry->type == 0) continue;  /* 跳过未使用的 */
        
        uint16_t router_topic_id = (uint16_t)(uintptr_t)entry->user_data;
        
        if (entry->type == type && router_topic_id == topic_id) {
            memset(entry, 0, sizeof(topic_router_entry_t));
            router->router_count--;
            return 0;
        }
    }
    
    return -1;
}

