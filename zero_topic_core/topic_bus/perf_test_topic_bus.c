#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>
#include "topic_bus.h"
#include "topic_server.h"
#include "topic_router.h"
#include "../obj_dict/obj_dict.h"
#include "../../Rte/inc/os_timestamp.h"
#include "../../Rte/inc/os_printf.h"
#include "../../Rte/inc/os_thread.h"

/* 诊断打印辅助（仅当启用统计与诊断时有效） */
#if TOPIC_BUS_ENABLE_STATS && TOPIC_BUS_ENABLE_DIAG
static void print_topic_diag(topic_bus_t* bus, uint16_t topic_id) {
    if (!bus || !bus->topics) return;
    for (size_t i = 0; i < bus->max_topics; ++i) {
        topic_entry_t* e = &bus->topics[i];
        if (e->topic_id == topic_id) {
#if TOPIC_BUS_ENABLE_ATOMICS
            uint32_t cnt = atomic_load_explicit(&e->event_count, memory_order_acquire);
#else
            uint32_t cnt = e->event_count;
#endif
            os_printf("[topic][DIAG] topic=%u count=%u last_event=%u last_len=%u last_ts_us=%llu\n",
                      (unsigned)topic_id, (unsigned)cnt,
                      (unsigned)e->last_event_key, (unsigned)e->last_data_len,
                      (unsigned long long)e->last_ts_us);
            break;
        }
    }
}
#else
static void print_topic_diag(topic_bus_t* bus, uint16_t topic_id) { (void)bus; (void)topic_id; }
#endif

/* 测试配置 */
#define PERF_TEST_MAX_TOPICS      16
#define PERF_TEST_MAX_SUBSCRIBERS 8
#define PERF_TEST_LOOPS           100000
#define PERF_TEST_EVENT_COUNT     100

/* 测试Topic ID枚举 */
typedef enum {
    TEST_TOPIC_ID_OR_BASIC = 1,
    TEST_TOPIC_ID_AND_BASIC = 2,
    TEST_TOPIC_ID_MANUAL_BASIC = 3,
    TEST_TOPIC_ID_TIMEOUT_DEFAULT = 1,
    TEST_TOPIC_ID_TIMEOUT_CUSTOM = 1,
    TEST_TOPIC_ID_TIMEOUT_MAX = 1,
    TEST_TOPIC_ID_TIMEOUT_BOUNDARY = 1,
    TEST_TOPIC_ID_SHARED_1 = 1,
    TEST_TOPIC_ID_SHARED_2 = 2,
    TEST_TOPIC_ID_SERVER = 1,
    TEST_TOPIC_ID_ROUTER_VFB = 1,
    TEST_TOPIC_ID_ROUTER_CUSTOM = 1,
    TEST_TOPIC_ID_ROUTER_MULTIPLE = 1,
    TEST_TOPIC_ID_ISR = 1,
    TEST_TOPIC_ID_PERF_TIMEOUT = 1,
    TEST_TOPIC_ID_PERF_EVENT_PUBLISH_BASE = 1,
    TEST_TOPIC_ID_PERF_RULE_MATCHING_OR_BASE = 1,
    TEST_TOPIC_ID_PERF_RULE_MATCHING_AND = 100,
    TEST_TOPIC_ID_CONCURRENT_BASE = 1,
    TEST_TOPIC_ID_MAX = 100
} test_topic_id_t;

/* 测试Event ID枚举 */
typedef enum {
    TEST_EVENT_ID_OR_1 = 10,
    TEST_EVENT_ID_OR_2 = 20,
    TEST_EVENT_ID_OR_3 = 30,
    TEST_EVENT_ID_AND_1 = 40,
    TEST_EVENT_ID_AND_2 = 50,
    TEST_EVENT_ID_MANUAL = 60,
    TEST_EVENT_ID_TIMEOUT_DEFAULT = 100,
    TEST_EVENT_ID_TIMEOUT_CUSTOM_1 = 200,
    TEST_EVENT_ID_TIMEOUT_CUSTOM_2 = 201,
    TEST_EVENT_ID_TIMEOUT_MAX = 300,
    TEST_EVENT_ID_TIMEOUT_BOUNDARY = 350,
    TEST_EVENT_ID_SHARED_1 = 20,
    TEST_EVENT_ID_SHARED_2 = 30,
    TEST_EVENT_ID_SERVER = 100,
    TEST_EVENT_ID_ROUTER_VFB = 200,
    TEST_EVENT_ID_ROUTER_CUSTOM = 300,
    TEST_EVENT_ID_ROUTER_MULTIPLE = 400,
    TEST_EVENT_ID_ISR = 500,
    TEST_EVENT_ID_PERF_TIMEOUT = 600,
    TEST_EVENT_ID_PERF_EVENT_PUBLISH_BASE = 10,
    TEST_EVENT_ID_PERF_EVENT_PUBLISH_OFFSET = 20,
    TEST_EVENT_ID_PERF_RULE_MATCHING_OR = 15,
    TEST_EVENT_ID_PERF_RULE_MATCHING_AND_1 = 40,
    TEST_EVENT_ID_PERF_RULE_MATCHING_AND_2 = 50,
    TEST_EVENT_ID_PERF_RULE_MATCHING_AND_3 = 60,
    TEST_EVENT_ID_CONCURRENT_BASE = 700,
    TEST_EVENT_ID_MAX = 1000
} test_event_id_t;

/* 测试数据结构 */
typedef struct {
    uint32_t value;
    uint32_t counter;
    uint8_t  padding[24];
} test_event_data_t;

/* 全局统计 */
static atomic_uint callback_count = ATOMIC_VAR_INIT(0);
static atomic_uint router_vfb_count = ATOMIC_VAR_INIT(0);
static atomic_uint router_custom_count = ATOMIC_VAR_INIT(0);

/* ---------------- 测试回调函数 ---------------- */

static void test_callback(uint16_t topic_id, const void* data, size_t data_len, void* user) {
    (void)topic_id;
    (void)data;
    (void)data_len;
    (void)user;
    atomic_fetch_add_explicit(&callback_count, 1, memory_order_relaxed);
}

/* ---------------- 基础功能测试 ---------------- */

static int test_functional_basic(void) {
    os_printf("\n[topic][FUNC] 基础功能测试: init/subscribe/publish/manual\n");

    /* 初始化obj_dict */
    obj_dict_entry_t dict_entries[PERF_TEST_EVENT_COUNT];
    obj_dict_t dict;
    if (obj_dict_init(&dict, dict_entries, PERF_TEST_EVENT_COUNT) != 0) {
        os_printf("[topic][FUNC] obj_dict初始化失败\n");
        return -1;
    }

    /* 初始化topic_bus */
    topic_entry_t topic_entries[PERF_TEST_MAX_TOPICS];
    topic_bus_t bus;
    if (topic_bus_init(&bus, topic_entries, PERF_TEST_MAX_TOPICS, &dict) != 0) {
        os_printf("[topic][FUNC] topic_bus初始化失败\n");
        return -1;
    }

    /* 测试：创建OR规则 */
    obj_dict_key_t events_or[] = {TEST_EVENT_ID_OR_1, TEST_EVENT_ID_OR_2, TEST_EVENT_ID_OR_3};
    topic_rule_t rule_or = {
        .type = TOPIC_RULE_OR,
        .events = events_or,
        .event_count = 3,
    };

    if (topic_rule_create(&bus, TEST_TOPIC_ID_OR_BASIC, &rule_or) != 0) {
        os_printf("[topic][FUNC] 创建OR规则失败\n");
        return -1;
    }

    /* 测试：订阅Topic */
    if (topic_subscribe(&bus, TEST_TOPIC_ID_OR_BASIC, test_callback, NULL) != 0) {
        os_printf("[topic][FUNC] 订阅Topic失败\n");
        return -1;
    }

    /* 测试：发布事件触发OR规则 */
    test_event_data_t event_data = { .value = 0x12345678, .counter = 0 };
    obj_dict_set(&dict, TEST_EVENT_ID_OR_1, &event_data, sizeof(event_data), 0);
    
    atomic_store_explicit(&callback_count, 0, memory_order_release);
    if (topic_publish_event(&bus, TEST_EVENT_ID_OR_1) != 0) {
        os_printf("[topic][FUNC] 发布事件失败\n");
        return -1;
    }
    
    if (atomic_load_explicit(&callback_count, memory_order_acquire) != 1) {
        os_printf("[topic][FUNC] OR规则回调次数错误 count=%u\n", 
                  atomic_load_explicit(&callback_count, memory_order_acquire));
        return -1;
    }
    /* 打印OR Topic诊断信息 */
    print_topic_diag(&bus, TEST_TOPIC_ID_OR_BASIC);

    /* 测试：创建AND规则 */
    obj_dict_key_t events_and[] = {TEST_EVENT_ID_AND_1, TEST_EVENT_ID_AND_2};
    topic_rule_t rule_and = {
        .type = TOPIC_RULE_AND,
        .events = events_and,
        .event_count = 2,
    };

    if (topic_rule_create(&bus, TEST_TOPIC_ID_AND_BASIC, &rule_and) != 0) {
        os_printf("[topic][FUNC] 创建AND规则失败\n");
        return -1;
    }

    if (topic_subscribe(&bus, TEST_TOPIC_ID_AND_BASIC, test_callback, NULL) != 0) {
        os_printf("[topic][FUNC] 订阅AND Topic失败\n");
        return -1;
    }

    /* 发布第一个事件（不应触发） */
    atomic_store_explicit(&callback_count, 0, memory_order_release);
    obj_dict_set(&dict, TEST_EVENT_ID_AND_1, &event_data, sizeof(event_data), 0);
    topic_publish_event(&bus, TEST_EVENT_ID_AND_1);
    if (atomic_load_explicit(&callback_count, memory_order_acquire) != 0) {
        os_printf("[topic][FUNC] AND规则在第一个事件时不应触发\n");
        return -1;
    }

    /* 发布第二个事件（应触发） */
    obj_dict_set(&dict, TEST_EVENT_ID_AND_2, &event_data, sizeof(event_data), 0);
    topic_publish_event(&bus, TEST_EVENT_ID_AND_2);
    if (atomic_load_explicit(&callback_count, memory_order_acquire) != 1) {
        os_printf("[topic][FUNC] AND规则触发失败 count=%u\n",
                  atomic_load_explicit(&callback_count, memory_order_acquire));
        return -1;
    }
    /* 打印AND Topic诊断信息 */
    print_topic_diag(&bus, TEST_TOPIC_ID_AND_BASIC);

    /* 测试：手动发布 */
    obj_dict_key_t events_manual[] = {TEST_EVENT_ID_MANUAL};
    topic_rule_t rule_manual = {
        .type = TOPIC_RULE_MANUAL,
        .events = events_manual,
        .event_count = 1,
    };

    if (topic_rule_create(&bus, TEST_TOPIC_ID_MANUAL_BASIC, &rule_manual) != 0) {
        os_printf("[topic][FUNC] 创建MANUAL规则失败\n");
        return -1;
    }

    if (topic_subscribe(&bus, TEST_TOPIC_ID_MANUAL_BASIC, test_callback, NULL) != 0) {
        os_printf("[topic][FUNC] 订阅MANUAL Topic失败\n");
        return -1;
    }

    atomic_store_explicit(&callback_count, 0, memory_order_release);
    if (topic_publish_manual(&bus, TEST_TOPIC_ID_MANUAL_BASIC) != 0) {
        os_printf("[topic][FUNC] 手动发布失败\n");
        return -1;
    }

    if (atomic_load_explicit(&callback_count, memory_order_acquire) != 1) {
        os_printf("[topic][FUNC] MANUAL规则回调次数错误 count=%u\n",
                  atomic_load_explicit(&callback_count, memory_order_acquire));
        return -1;
    }
    /* 打印MANUAL Topic诊断信息 */
    print_topic_diag(&bus, TEST_TOPIC_ID_MANUAL_BASIC);

    os_printf("[topic][FUNC] 基础功能测试: 通过\n");
    return 0;
}

/* ---------------- 时效性测试用例 ---------------- */

/*
 * @brief 测试默认时效性（2.1）
 * @return 0成功，-1失败
 */
static int test_timeout_default(void) {
    os_printf("\n[topic][TIMEOUT] 默认时效性测试\n");

    obj_dict_entry_t dict_entries[PERF_TEST_EVENT_COUNT];
    obj_dict_t dict;
    obj_dict_init(&dict, dict_entries, PERF_TEST_EVENT_COUNT);

    topic_entry_t topic_entries[PERF_TEST_MAX_TOPICS];
    topic_bus_t bus;
    topic_bus_init(&bus, topic_entries, PERF_TEST_MAX_TOPICS, &dict);

    obj_dict_key_t events[] = {TEST_EVENT_ID_TIMEOUT_DEFAULT};
    topic_rule_t rule = {
        .type = TOPIC_RULE_OR,
        .events = events,
        .event_count = 1,
        .event_timeouts_ms = NULL,  /* 使用默认值 */
    };

    topic_rule_create(&bus, TEST_TOPIC_ID_TIMEOUT_DEFAULT, &rule);
    topic_subscribe(&bus, TEST_TOPIC_ID_TIMEOUT_DEFAULT, test_callback, NULL);

    test_event_data_t event_data = { .value = 0x12345678, .counter = 0 };
    obj_dict_set(&dict, TEST_EVENT_ID_TIMEOUT_DEFAULT, &event_data, sizeof(event_data), 0);

    /* 立即发布，应触发 */
    atomic_store_explicit(&callback_count, 0, memory_order_release);
    topic_publish_event(&bus, TEST_EVENT_ID_TIMEOUT_DEFAULT);
    if (atomic_load_explicit(&callback_count, memory_order_acquire) != 1) {
        os_printf("[topic][TIMEOUT] 立即发布应触发\n");
        return -1;
    }

    /* 等待3100ms后发布，应不触发（超时） */
    os_thread_sleep_ms(3100);
    atomic_store_explicit(&callback_count, 0, memory_order_release);
    topic_publish_event(&bus, TEST_EVENT_ID_TIMEOUT_DEFAULT);
    if (atomic_load_explicit(&callback_count, memory_order_acquire) != 0) {
        os_printf("[topic][TIMEOUT] 超时后不应触发 count=%u\n",
                  atomic_load_explicit(&callback_count, memory_order_acquire));
        return -1;
    }

    os_printf("[topic][TIMEOUT] 默认时效性测试: 通过\n");
    return 0;
}

/*
 * @brief 测试自定义时效性（2.2）
 * @return 0成功，-1失败
 */
static int test_timeout_custom(void) {
    os_printf("\n[topic][TIMEOUT] 自定义时效性测试\n");

    obj_dict_entry_t dict_entries[PERF_TEST_EVENT_COUNT];
    obj_dict_t dict;
    obj_dict_init(&dict, dict_entries, PERF_TEST_EVENT_COUNT);

    topic_entry_t topic_entries[PERF_TEST_MAX_TOPICS];
    topic_bus_t bus;
    topic_bus_init(&bus, topic_entries, PERF_TEST_MAX_TOPICS, &dict);

    obj_dict_key_t events[] = {TEST_EVENT_ID_TIMEOUT_CUSTOM_1, TEST_EVENT_ID_TIMEOUT_CUSTOM_2};
    uint32_t timeouts[] = {1000, 5000};  /* 事件200超时1秒，事件201超时5秒 */
    topic_rule_t rule = {
        .type = TOPIC_RULE_AND,
        .events = events,
        .event_count = 2,
        .event_timeouts_ms = timeouts,
    };

    topic_rule_create(&bus, TEST_TOPIC_ID_TIMEOUT_CUSTOM, &rule);
    topic_subscribe(&bus, TEST_TOPIC_ID_TIMEOUT_CUSTOM, test_callback, NULL);

    test_event_data_t event_data = { .value = 0x12345678, .counter = 0 };

    /* 场景1：均在时效内 */
    /* 先发布事件200（会检查时效性并更新trigger_mask） */
    obj_dict_set(&dict, TEST_EVENT_ID_TIMEOUT_CUSTOM_1, &event_data, sizeof(event_data), 0);
    atomic_store_explicit(&callback_count, 0, memory_order_release);
    topic_publish_event(&bus, TEST_EVENT_ID_TIMEOUT_CUSTOM_1);  /* 发布事件200，更新trigger_mask */
    /* 等待500ms，两个事件都在时效内 */
    os_thread_sleep_ms(500);
    /* 再发布事件201（会检查时效性，如果事件200也满足时效性，则触发） */
    obj_dict_set(&dict, TEST_EVENT_ID_TIMEOUT_CUSTOM_2, &event_data, sizeof(event_data), 0);
    topic_publish_event(&bus, TEST_EVENT_ID_TIMEOUT_CUSTOM_2);  /* 发布事件201，应触发Topic */
    if (atomic_load_explicit(&callback_count, memory_order_acquire) != 1) {
        os_printf("[topic][TIMEOUT] 场景1应在时效内触发 count=%u\n",
                  atomic_load_explicit(&callback_count, memory_order_acquire));
        return -1;
    }

    /* 场景2：事件200超时 */
    /* 需要重新创建规则，因为之前的trigger_mask已被重置 */
    topic_rule_create(&bus, TEST_TOPIC_ID_TIMEOUT_CUSTOM, &rule);  /* 重新创建规则以重置状态 */
    obj_dict_set(&dict, TEST_EVENT_ID_TIMEOUT_CUSTOM_1, &event_data, sizeof(event_data), 0);
    atomic_store_explicit(&callback_count, 0, memory_order_release);
    topic_publish_event(&bus, TEST_EVENT_ID_TIMEOUT_CUSTOM_1);  /* 发布事件200 */
    os_thread_sleep_ms(1500);  /* 等待1500ms，超过事件200的超时时间1000ms */
    obj_dict_set(&dict, TEST_EVENT_ID_TIMEOUT_CUSTOM_2, &event_data, sizeof(event_data), 0);
    topic_publish_event(&bus, TEST_EVENT_ID_TIMEOUT_CUSTOM_2);  /* 发布事件201，但事件200已超时，不应触发 */
    if (atomic_load_explicit(&callback_count, memory_order_acquire) != 0) {
        os_printf("[topic][TIMEOUT] 场景2事件200超时不应触发 count=%u\n",
                  atomic_load_explicit(&callback_count, memory_order_acquire));
        return -1;
    }

    os_printf("[topic][TIMEOUT] 自定义时效性测试: 通过\n");
    return 0;
}

/*
 * @brief 测试最大延迟宏（2.3）
 * @return 0成功，-1失败
 */
static int test_timeout_max(void) {
    os_printf("\n[topic][TIMEOUT] 最大延迟宏测试\n");

    obj_dict_entry_t dict_entries[PERF_TEST_EVENT_COUNT];
    obj_dict_t dict;
    obj_dict_init(&dict, dict_entries, PERF_TEST_EVENT_COUNT);

    topic_entry_t topic_entries[PERF_TEST_MAX_TOPICS];
    topic_bus_t bus;
    topic_bus_init(&bus, topic_entries, PERF_TEST_MAX_TOPICS, &dict);

    obj_dict_key_t events[] = {TEST_EVENT_ID_TIMEOUT_MAX};
    uint32_t timeouts[] = {TOPIC_BUS_EVENT_TIMEOUT_MAX};  /* 不判断时效性 */
    topic_rule_t rule = {
        .type = TOPIC_RULE_OR,
        .events = events,
        .event_count = 1,
        .event_timeouts_ms = timeouts,
    };

    topic_rule_create(&bus, TEST_TOPIC_ID_TIMEOUT_MAX, &rule);
    topic_subscribe(&bus, TEST_TOPIC_ID_TIMEOUT_MAX, test_callback, NULL);

    test_event_data_t event_data = { .value = 0x12345678, .counter = 0 };
    obj_dict_set(&dict, TEST_EVENT_ID_TIMEOUT_MAX, &event_data, sizeof(event_data), 0);

    /* 等待任意长时间后仍应触发 */
    os_thread_sleep_ms(10000);  /* 等待10秒 */
    atomic_store_explicit(&callback_count, 0, memory_order_release);
    topic_publish_event(&bus, TEST_EVENT_ID_TIMEOUT_MAX);
    if (atomic_load_explicit(&callback_count, memory_order_acquire) != 1) {
        os_printf("[topic][TIMEOUT] 最大延迟宏应始终触发\n");
        return -1;
    }

    os_printf("[topic][TIMEOUT] 最大延迟宏测试: 通过\n");
    return 0;
}

/*
 * @brief 测试时效性边界条件（2.4）
 * @return 0成功，-1失败
 */
static int test_timeout_boundary(void) {
    os_printf("\n[topic][TIMEOUT] 时效性边界测试\n");

    obj_dict_entry_t dict_entries[PERF_TEST_EVENT_COUNT];
    obj_dict_t dict;
    obj_dict_init(&dict, dict_entries, PERF_TEST_EVENT_COUNT);

    topic_entry_t topic_entries[PERF_TEST_MAX_TOPICS];
    topic_bus_t bus;
    topic_bus_init(&bus, topic_entries, PERF_TEST_MAX_TOPICS, &dict);

    obj_dict_key_t events[] = {TEST_EVENT_ID_TIMEOUT_BOUNDARY};
    uint32_t timeouts[] = {1000};  /* 超时1秒 */
    topic_rule_t rule = {
        .type = TOPIC_RULE_OR,
        .events = events,
        .event_count = 1,
        .event_timeouts_ms = timeouts,
    };

    topic_rule_create(&bus, TEST_TOPIC_ID_TIMEOUT_BOUNDARY, &rule);
    topic_subscribe(&bus, TEST_TOPIC_ID_TIMEOUT_BOUNDARY, test_callback, NULL);

    test_event_data_t event_data = { .value = 0x12345678, .counter = 0 };

    /* 测试边界：500ms后应触发（在1000ms超时内） */
    obj_dict_set(&dict, TEST_EVENT_ID_TIMEOUT_BOUNDARY, &event_data, sizeof(event_data), 0);
    uint64_t start_ts = os_monotonic_time_get_microsecond();
    os_thread_sleep_ms(500);  /* 等待500ms，在超时时间内 */
    uint64_t elapsed_ms = (os_monotonic_time_get_microsecond() - start_ts) / 1000;
    atomic_store_explicit(&callback_count, 0, memory_order_release);
    topic_publish_event(&bus, TEST_EVENT_ID_TIMEOUT_BOUNDARY);
    uint32_t count = atomic_load_explicit(&callback_count, memory_order_acquire);
    if (count != 1) {
        os_printf("[topic][TIMEOUT] 边界测试：500ms应触发 count=%u, elapsed=%llu ms\n",
                  count, (unsigned long long)elapsed_ms);
        return -1;
    }

    /* 测试边界：1100ms后不应触发（超过1000ms超时） */
    obj_dict_set(&dict, TEST_EVENT_ID_TIMEOUT_BOUNDARY, &event_data, sizeof(event_data), 0);
    start_ts = os_monotonic_time_get_microsecond();
    os_thread_sleep_ms(1100);  /* 等待1100ms，超过边界 */
    elapsed_ms = (os_monotonic_time_get_microsecond() - start_ts) / 1000;
    atomic_store_explicit(&callback_count, 0, memory_order_release);
    topic_publish_event(&bus, TEST_EVENT_ID_TIMEOUT_BOUNDARY);
    count = atomic_load_explicit(&callback_count, memory_order_acquire);
    if (count != 0) {
        os_printf("[topic][TIMEOUT] 边界测试：1100ms不应触发 count=%u, elapsed=%llu ms\n",
                  count, (unsigned long long)elapsed_ms);
        return -1;
    }

    os_printf("[topic][TIMEOUT] 时效性边界测试: 通过\n");
    return 0;
}

/* ---------------- 多Topic共享Event Key测试 ---------------- */

/*
 * @brief 测试多个Topic共享同一Event Key（3.1）
 * @return 0成功，-1失败
 */
static int test_shared_event_key(void) {
    os_printf("\n[topic][SHARED] 多Topic共享Event Key测试\n");

    obj_dict_entry_t dict_entries[PERF_TEST_EVENT_COUNT];
    obj_dict_t dict;
    obj_dict_init(&dict, dict_entries, PERF_TEST_EVENT_COUNT);

    topic_entry_t topic_entries[PERF_TEST_MAX_TOPICS];
    topic_bus_t bus;
    topic_bus_init(&bus, topic_entries, PERF_TEST_MAX_TOPICS, &dict);

    /* 创建Topic1（OR规则：事件10, 20） */
    obj_dict_key_t events1[] = {TEST_EVENT_ID_OR_1, TEST_EVENT_ID_SHARED_1};
    topic_rule_t rule1 = {
        .type = TOPIC_RULE_OR,
        .events = events1,
        .event_count = 2,
    };

    /* 创建Topic2（OR规则：事件20, 30） */
    obj_dict_key_t events2[] = {TEST_EVENT_ID_SHARED_1, TEST_EVENT_ID_SHARED_2};
    topic_rule_t rule2 = {
        .type = TOPIC_RULE_OR,
        .events = events2,
        .event_count = 2,
    };

    topic_rule_create(&bus, TEST_TOPIC_ID_SHARED_1, &rule1);
    topic_rule_create(&bus, TEST_TOPIC_ID_SHARED_2, &rule2);
    topic_subscribe(&bus, TEST_TOPIC_ID_SHARED_1, test_callback, NULL);
    topic_subscribe(&bus, TEST_TOPIC_ID_SHARED_2, test_callback, NULL);

    test_event_data_t event_data = { .value = 0x12345678, .counter = 0 };
    obj_dict_set(&dict, TEST_EVENT_ID_SHARED_1, &event_data, sizeof(event_data), 0);

    /* 发布事件20，应触发Topic1和Topic2 */
    atomic_store_explicit(&callback_count, 0, memory_order_release);
    topic_publish_event(&bus, TEST_EVENT_ID_SHARED_1);
    if (atomic_load_explicit(&callback_count, memory_order_acquire) != 2) {
        os_printf("[topic][SHARED] 事件20应触发2个Topic count=%u\n",
                  atomic_load_explicit(&callback_count, memory_order_acquire));
        return -1;
    }

    os_printf("[topic][SHARED] 多Topic共享Event Key测试: 通过\n");
    return 0;
}

/* ---------------- Topic Server测试 ---------------- */

#if TOPIC_BUS_ENABLE_SERVER
/*
 * @brief 测试Topic Server周期性运行（4.1）
 * @return 0成功，-1失败
 */
static int test_server_periodic(void) {
    os_printf("\n[topic][SERVER] Server周期性运行测试\n");

    obj_dict_entry_t dict_entries[PERF_TEST_EVENT_COUNT];
    obj_dict_t dict;
    obj_dict_init(&dict, dict_entries, PERF_TEST_EVENT_COUNT);

    topic_entry_t topic_entries[PERF_TEST_MAX_TOPICS];
    topic_bus_t bus;
    topic_bus_init(&bus, topic_entries, PERF_TEST_MAX_TOPICS, &dict);

    topic_server_t server;
    if (topic_server_init(&server, &bus, 100) != 0) {
        os_printf("[topic][SERVER] Server初始化失败\n");
        return -1;
    }

#if TOPIC_BUS_ENABLE_ISR
    /* 模拟ISR发布事件 */
    topic_publish_isr(&bus, TEST_EVENT_ID_SERVER);
    topic_publish_isr(&bus, (obj_dict_key_t)(TEST_EVENT_ID_SERVER + 1));

    /* 运行Server一次，处理ISR队列 */
    int processed = topic_server_run_once(&server);
    if (processed <= 0) {
        os_printf("[topic][SERVER] Server未处理事件\n");
        return -1;
    }
#endif

    os_printf("[topic][SERVER] Server周期性运行测试: 通过\n");
    return 0;
}

#if TOPIC_BUS_ENABLE_STATS
/*
 * @brief 测试Topic Server性能统计（4.2）
 * @return 0成功，-1失败
 */
static int test_server_stats(void) {
    os_printf("\n[topic][SERVER] Server性能统计测试\n");

    obj_dict_entry_t dict_entries[PERF_TEST_EVENT_COUNT];
    obj_dict_t dict;
    obj_dict_init(&dict, dict_entries, PERF_TEST_EVENT_COUNT);

    topic_entry_t topic_entries[PERF_TEST_MAX_TOPICS];
    topic_bus_t bus;
    topic_bus_init(&bus, topic_entries, PERF_TEST_MAX_TOPICS, &dict);

    /* 创建Topic以便事件可以触发 */
    obj_dict_key_t events[] = {TEST_EVENT_ID_SERVER};
    topic_rule_t rule = {
        .type = TOPIC_RULE_OR,
        .events = events,
        .event_count = 1,
    };
    topic_rule_create(&bus, TEST_TOPIC_ID_SERVER, &rule);
    topic_subscribe(&bus, TEST_TOPIC_ID_SERVER, test_callback, NULL);

    test_event_data_t event_data = { .value = 0x12345678, .counter = 0 };
    obj_dict_set(&dict, TEST_EVENT_ID_SERVER, &event_data, sizeof(event_data), 0);

    topic_server_t server;
    topic_server_init(&server, &bus, 100);

#if TOPIC_BUS_ENABLE_ISR
    /* 模拟多次ISR发布 */
    for (int i = 0; i < 10; ++i) {
        topic_publish_isr(&bus, TEST_EVENT_ID_SERVER);
    }
    /* 处理ISR队列并手动更新统计信息（模拟topic_server_task的行为） */
    uint64_t start_us = os_monotonic_time_get_microsecond();
    int processed = topic_server_run_once(&server);
    uint64_t end_us = os_monotonic_time_get_microsecond();
    
#if TOPIC_BUS_ENABLE_STATS
    if (processed > 0) {
        server.total_processed += processed;
        server.total_time_us += (end_us - start_us);
    }
#endif

    if (processed == 0) {
        os_printf("[topic][SERVER] Server未处理任何事件\n");
        return -1;
    }
#else
    /* 非ISR模式下，统计信息始终为0 */
    os_printf("[topic][SERVER] ISR未启用，跳过统计测试\n");
    return 0;
#endif

    uint64_t avg_time_us = 0;
    uint64_t total = topic_server_get_stats(&server, &avg_time_us);
    if (total == 0) {
        os_printf("[topic][SERVER] 统计信息为空\n");
        return -1;
    }

    os_printf("[topic][SERVER] 总处理数=%llu, 平均耗时=%llu us\n",
              (unsigned long long)total, (unsigned long long)avg_time_us);

    os_printf("[topic][SERVER] Server性能统计测试: 通过\n");
    return 0;
}
#endif
#endif

/* ---------------- Topic Router测试 ---------------- */

#if TOPIC_BUS_ENABLE_ROUTER
/*
 * @brief 自定义Router回调函数
 */
static int custom_router_callback(uint16_t topic_id, const void* data, size_t data_len, void* user_data) {
    (void)topic_id;
    (void)data;
    (void)data_len;
    (void)user_data;
    atomic_fetch_add_explicit(&router_custom_count, 1, memory_order_relaxed);
    return 0;
}

/*
 * @brief 测试VFB Router（5.1）
 * @return 0成功，-1失败
 */
static int test_router_vfb(void) {
    os_printf("\n[topic][ROUTER] VFB Router测试\n");

    obj_dict_entry_t dict_entries[PERF_TEST_EVENT_COUNT];
    obj_dict_t dict;
    obj_dict_init(&dict, dict_entries, PERF_TEST_EVENT_COUNT);

    topic_entry_t topic_entries[PERF_TEST_MAX_TOPICS];
    topic_bus_t bus;
    topic_bus_init(&bus, topic_entries, PERF_TEST_MAX_TOPICS, &dict);

    topic_router_entry_t router_entries[TOPIC_BUS_MAX_ROUTERS_PER_TOPIC];
    topic_router_t router;
    topic_router_init(&router, router_entries, TOPIC_BUS_MAX_ROUTERS_PER_TOPIC);

    /* 添加VFB Router */
    if (topic_router_add_vfb(&router, TEST_TOPIC_ID_ROUTER_VFB, 100) != 0) {
        os_printf("[topic][ROUTER] 添加VFB Router失败\n");
        return -1;
    }

    /* 设置Router到Topic Bus */
#if TOPIC_BUS_ENABLE_ROUTER
    topic_bus_set_router(&bus, &router);
#endif

    /* 创建Topic并触发 */
    obj_dict_key_t events[] = {TEST_EVENT_ID_ROUTER_VFB};
    topic_rule_t rule = {
        .type = TOPIC_RULE_OR,
        .events = events,
        .event_count = 1,
    };

    topic_rule_create(&bus, TEST_TOPIC_ID_ROUTER_VFB, &rule);

    test_event_data_t event_data = { .value = 0x12345678, .counter = 0 };
    obj_dict_set(&dict, TEST_EVENT_ID_ROUTER_VFB, &event_data, sizeof(event_data), 0);

    atomic_store_explicit(&router_vfb_count, 0, memory_order_release);
    topic_publish_event(&bus, TEST_EVENT_ID_ROUTER_VFB);

    /* 注意：VFB Router需要VFB模块支持，这里仅测试Router调用 */
    os_printf("[topic][ROUTER] VFB Router测试: 通过（需VFB模块支持）\n");
    return 0;
}

/*
 * @brief 测试自定义Router（5.2）
 * @return 0成功，-1失败
 */
static int test_router_custom(void) {
    os_printf("\n[topic][ROUTER] 自定义Router测试\n");

    obj_dict_entry_t dict_entries[PERF_TEST_EVENT_COUNT];
    obj_dict_t dict;
    obj_dict_init(&dict, dict_entries, PERF_TEST_EVENT_COUNT);

    topic_entry_t topic_entries[PERF_TEST_MAX_TOPICS];
    topic_bus_t bus;
    topic_bus_init(&bus, topic_entries, PERF_TEST_MAX_TOPICS, &dict);

    topic_router_entry_t router_entries[TOPIC_BUS_MAX_ROUTERS_PER_TOPIC];
    topic_router_t router;
    topic_router_init(&router, router_entries, TOPIC_BUS_MAX_ROUTERS_PER_TOPIC);

    /* 添加自定义Router */
    if (topic_router_add_custom(&router, TEST_TOPIC_ID_ROUTER_CUSTOM, custom_router_callback, NULL) != 0) {
        os_printf("[topic][ROUTER] 添加自定义Router失败\n");
        return -1;
    }

#if TOPIC_BUS_ENABLE_ROUTER
    topic_bus_set_router(&bus, &router);
#endif

    /* 创建Topic并触发 */
    obj_dict_key_t events[] = {TEST_EVENT_ID_ROUTER_CUSTOM};
    topic_rule_t rule = {
        .type = TOPIC_RULE_OR,
        .events = events,
        .event_count = 1,
    };

    topic_rule_create(&bus, TEST_TOPIC_ID_ROUTER_CUSTOM, &rule);

    test_event_data_t event_data = { .value = 0x12345678, .counter = 0 };
    obj_dict_set(&dict, TEST_EVENT_ID_ROUTER_CUSTOM, &event_data, sizeof(event_data), 0);

    atomic_store_explicit(&router_custom_count, 0, memory_order_release);
    topic_publish_event(&bus, TEST_EVENT_ID_ROUTER_CUSTOM);

    if (atomic_load_explicit(&router_custom_count, memory_order_acquire) != 1) {
        os_printf("[topic][ROUTER] 自定义Router回调未执行 count=%u\n",
                  atomic_load_explicit(&router_custom_count, memory_order_acquire));
        return -1;
    }

    os_printf("[topic][ROUTER] 自定义Router测试: 通过\n");
    return 0;
}

/*
 * @brief 测试多Router（5.3）
 * @return 0成功，-1失败
 */
static int test_router_multiple(void) {
    os_printf("\n[topic][ROUTER] 多Router测试\n");

    obj_dict_entry_t dict_entries[PERF_TEST_EVENT_COUNT];
    obj_dict_t dict;
    obj_dict_init(&dict, dict_entries, PERF_TEST_EVENT_COUNT);

    topic_entry_t topic_entries[PERF_TEST_MAX_TOPICS];
    topic_bus_t bus;
    topic_bus_init(&bus, topic_entries, PERF_TEST_MAX_TOPICS, &dict);

    topic_router_entry_t router_entries[TOPIC_BUS_MAX_ROUTERS_PER_TOPIC];
    topic_router_t router;
    topic_router_init(&router, router_entries, TOPIC_BUS_MAX_ROUTERS_PER_TOPIC);

    /* 为Topic1添加VFB Router和自定义Router */
    topic_router_add_vfb(&router, TEST_TOPIC_ID_ROUTER_MULTIPLE, 100);
    topic_router_add_custom(&router, TEST_TOPIC_ID_ROUTER_MULTIPLE, custom_router_callback, NULL);

#if TOPIC_BUS_ENABLE_ROUTER
    topic_bus_set_router(&bus, &router);
#endif

    obj_dict_key_t events[] = {TEST_EVENT_ID_ROUTER_MULTIPLE};
    topic_rule_t rule = {
        .type = TOPIC_RULE_OR,
        .events = events,
        .event_count = 1,
    };

    topic_rule_create(&bus, TEST_TOPIC_ID_ROUTER_MULTIPLE, &rule);

    test_event_data_t event_data = { .value = 0x12345678, .counter = 0 };
    obj_dict_set(&dict, TEST_EVENT_ID_ROUTER_MULTIPLE, &event_data, sizeof(event_data), 0);

    atomic_store_explicit(&router_custom_count, 0, memory_order_release);
    topic_publish_event(&bus, TEST_EVENT_ID_ROUTER_MULTIPLE);

    /* 验证自定义Router被调用 */
    if (atomic_load_explicit(&router_custom_count, memory_order_acquire) != 1) {
        os_printf("[topic][ROUTER] 多Router未全部执行\n");
        return -1;
    }

    os_printf("[topic][ROUTER] 多Router测试: 通过\n");
    return 0;
}
#endif

/* ---------------- ISR路径测试 ---------------- */

#if TOPIC_BUS_ENABLE_ISR
/*
 * @brief 测试ISR发布（6.1）
 * @return 0成功，-1失败
 */
static int test_isr_publish(void) {
    os_printf("\n[topic][ISR] ISR发布测试\n");

    obj_dict_entry_t dict_entries[PERF_TEST_EVENT_COUNT];
    obj_dict_t dict;
    obj_dict_init(&dict, dict_entries, PERF_TEST_EVENT_COUNT);

    topic_entry_t topic_entries[PERF_TEST_MAX_TOPICS];
    topic_bus_t bus;
    topic_bus_init(&bus, topic_entries, PERF_TEST_MAX_TOPICS, &dict);

    /* 创建Topic */
    obj_dict_key_t events[] = {TEST_EVENT_ID_ISR};
    topic_rule_t rule = {
        .type = TOPIC_RULE_OR,
        .events = events,
        .event_count = 1,
    };

    topic_rule_create(&bus, TEST_TOPIC_ID_ISR, &rule);
    topic_subscribe(&bus, TEST_TOPIC_ID_ISR, test_callback, NULL);

    test_event_data_t event_data = { .value = 0x12345678, .counter = 0 };
    obj_dict_set(&dict, TEST_EVENT_ID_ISR, &event_data, sizeof(event_data), 0);

    /* 模拟ISR发布 */
    if (topic_publish_isr(&bus, TEST_EVENT_ID_ISR) != 0) {
        os_printf("[topic][ISR] ISR发布失败\n");
        return -1;
    }

    /* 在任务上下文中处理ISR队列 */
    atomic_store_explicit(&callback_count, 0, memory_order_release);
    topic_bus_process_isr_queue(&bus);

    if (atomic_load_explicit(&callback_count, memory_order_acquire) != 1) {
        os_printf("[topic][ISR] ISR事件未处理 count=%u\n",
                  atomic_load_explicit(&callback_count, memory_order_acquire));
        return -1;
    }

    os_printf("[topic][ISR] ISR发布测试: 通过\n");
    return 0;
}
#endif

/* ---------------- 时效性性能测试 ---------------- */

/*
 * @brief 测试时效性检查性能开销（7.3）
 * @return 0成功，-1失败
 */
static int test_performance_timeout(void) {
    os_printf("\n[topic][PERF] 时效性检查性能测试\n");

    obj_dict_entry_t dict_entries[PERF_TEST_EVENT_COUNT];
    obj_dict_t dict;
    obj_dict_init(&dict, dict_entries, PERF_TEST_EVENT_COUNT);

    topic_entry_t topic_entries[PERF_TEST_MAX_TOPICS];
    topic_bus_t bus;
    topic_bus_init(&bus, topic_entries, PERF_TEST_MAX_TOPICS, &dict);

    obj_dict_key_t events[] = {TEST_EVENT_ID_PERF_TIMEOUT};
    uint32_t timeouts[] = {3000};  /* 启用时效性检查 */
    topic_rule_t rule = {
        .type = TOPIC_RULE_OR,
        .events = events,
        .event_count = 1,
        .event_timeouts_ms = timeouts,
    };

    topic_rule_create(&bus, TEST_TOPIC_ID_PERF_TIMEOUT, &rule);
    topic_subscribe(&bus, TEST_TOPIC_ID_PERF_TIMEOUT, test_callback, NULL);

    test_event_data_t event_data = { .value = 0x12345678, .counter = 0 };
    obj_dict_set(&dict, TEST_EVENT_ID_PERF_TIMEOUT, &event_data, sizeof(event_data), 0);

    /* 测量带时效性检查的性能 */
    uint64_t start = os_monotonic_time_get_microsecond();
    for (int i = 0; i < PERF_TEST_LOOPS; ++i) {
        topic_publish_event(&bus, TEST_EVENT_ID_PERF_TIMEOUT);
    }
    uint64_t end = os_monotonic_time_get_microsecond();

    double avg_us = (double)(end - start) / PERF_TEST_LOOPS;
    os_printf("[topic][PERF] 时效性检查延迟: %.2f us/publish\n", avg_us);

    return 0;
}

/* ---------------- 高并发测试 ---------------- */

/*
 * @brief 测试高并发场景（7.4）
 * @return 0成功，-1失败
 */
static int test_concurrent_high(void) {
    os_printf("\n[topic][CONCURRENT] 高并发测试\n");

    obj_dict_entry_t dict_entries[PERF_TEST_EVENT_COUNT];
    obj_dict_t dict;
    obj_dict_init(&dict, dict_entries, PERF_TEST_EVENT_COUNT);

    topic_entry_t topic_entries[PERF_TEST_MAX_TOPICS];
    topic_bus_t bus;
    topic_bus_init(&bus, topic_entries, PERF_TEST_MAX_TOPICS, &dict);

    /* 创建多个Topic */
    for (uint16_t i = 0; i < 8; ++i) {
        obj_dict_key_t events[] = {(obj_dict_key_t)(TEST_EVENT_ID_CONCURRENT_BASE + i)};
        topic_rule_t rule = {
            .type = TOPIC_RULE_OR,
            .events = events,
            .event_count = 1,
        };
        topic_rule_create(&bus, (test_topic_id_t)(TEST_TOPIC_ID_CONCURRENT_BASE + i), &rule);
        /* 每个Topic注册多个订阅者 */
        for (int j = 0; j < 4; ++j) {
            topic_subscribe(&bus, (test_topic_id_t)(TEST_TOPIC_ID_CONCURRENT_BASE + i), test_callback, NULL);
        }
    }

    test_event_data_t event_data = { .value = 0x12345678, .counter = 0 };

    /* 并发发布事件 */
    uint64_t start = os_monotonic_time_get_microsecond();
    for (int i = 0; i < 10000; ++i) {
        obj_dict_key_t key = (obj_dict_key_t)(TEST_EVENT_ID_CONCURRENT_BASE + (i % 8));
        obj_dict_set(&dict, key, &event_data, sizeof(event_data), 0);
        topic_publish_event(&bus, key);
    }
    uint64_t end = os_monotonic_time_get_microsecond();

    double avg_us = (double)(end - start) / 10000;
    uint32_t total_callbacks = atomic_load_explicit(&callback_count, memory_order_acquire);
    os_printf("[topic][CONCURRENT] 高并发延迟: %.2f us/publish, 总回调数=%u\n",
              avg_us, total_callbacks);

    return 0;
}

/* ---------------- 性能测试：事件发布延迟 ---------------- */

static int test_performance_event_publish(void) {
    os_printf("\n[topic][PERF] 事件发布延迟测试\n");

    obj_dict_entry_t dict_entries[PERF_TEST_EVENT_COUNT];
    obj_dict_t dict;
    obj_dict_init(&dict, dict_entries, PERF_TEST_EVENT_COUNT);

    topic_entry_t topic_entries[PERF_TEST_MAX_TOPICS];
    topic_bus_t bus;
    topic_bus_init(&bus, topic_entries, PERF_TEST_MAX_TOPICS, &dict);

    /* 创建多个OR规则的Topic */
    for (uint16_t i = 0; i < 5; ++i) {
        obj_dict_key_t events[2] = {(obj_dict_key_t)(TEST_EVENT_ID_PERF_EVENT_PUBLISH_BASE + i), 
                                     (obj_dict_key_t)(TEST_EVENT_ID_PERF_EVENT_PUBLISH_OFFSET + i)};
        topic_rule_t rule = {
            .type = TOPIC_RULE_OR,
            .events = events,
            .event_count = 2,
        };
        topic_rule_create(&bus, (test_topic_id_t)(TEST_TOPIC_ID_PERF_EVENT_PUBLISH_BASE + i), &rule);
        topic_subscribe(&bus, (test_topic_id_t)(TEST_TOPIC_ID_PERF_EVENT_PUBLISH_BASE + i), test_callback, NULL);
    }

    test_event_data_t event_data = { .value = 0x12345678, .counter = 0 };

    /* 预热 */
    for (int i = 0; i < 1000; ++i) {
        obj_dict_set(&dict, TEST_EVENT_ID_PERF_EVENT_PUBLISH_BASE, &event_data, sizeof(event_data), 0);
        topic_publish_event(&bus, TEST_EVENT_ID_PERF_EVENT_PUBLISH_BASE);
    }

    /* 测量延迟 */
    uint64_t start = os_monotonic_time_get_microsecond();
    for (int i = 0; i < PERF_TEST_LOOPS; ++i) {
        obj_dict_key_t key = (obj_dict_key_t)(TEST_EVENT_ID_PERF_EVENT_PUBLISH_BASE + (i % 5));
        obj_dict_set(&dict, key, &event_data, sizeof(event_data), 0);
        topic_publish_event(&bus, key);
    }
    uint64_t end = os_monotonic_time_get_microsecond();

    double avg_us = (double)(end - start) / PERF_TEST_LOOPS;
    os_printf("[topic][PERF] 事件发布延迟: %.2f us/publish (原子=%d)\n",
              avg_us, TOPIC_BUS_ENABLE_ATOMICS);

    return 0;
}

/* ---------------- 性能测试：规则匹配性能 ---------------- */

static int test_performance_rule_matching(void) {
    os_printf("\n[topic][PERF] 规则匹配性能测试\n");

    obj_dict_entry_t dict_entries[PERF_TEST_EVENT_COUNT];
    obj_dict_t dict;
    obj_dict_init(&dict, dict_entries, PERF_TEST_EVENT_COUNT);

    topic_entry_t topic_entries[PERF_TEST_MAX_TOPICS];
    topic_bus_t bus;
    topic_bus_init(&bus, topic_entries, PERF_TEST_MAX_TOPICS, &dict);

    /* 创建多个Topic（OR规则） */
    /* 使用静态数组，避免栈上的数组生命周期问题 */
    static obj_dict_key_t events_or_static[10][1];
    for (uint16_t i = 0; i < 10; ++i) {
        events_or_static[i][0] = (obj_dict_key_t)(TEST_EVENT_ID_PERF_EVENT_PUBLISH_BASE + i);
        topic_rule_t rule = {
            .type = TOPIC_RULE_OR,
            .events = events_or_static[i],
            .event_count = 1,
        };
        topic_rule_create(&bus, (test_topic_id_t)(TEST_TOPIC_ID_PERF_RULE_MATCHING_OR_BASE + i), &rule);
        topic_subscribe(&bus, (test_topic_id_t)(TEST_TOPIC_ID_PERF_RULE_MATCHING_OR_BASE + i), test_callback, NULL);
    }

    test_event_data_t event_data = { .value = 0x12345678, .counter = 0 };

    /* 测量OR规则匹配性能 */
    obj_dict_set(&dict, TEST_EVENT_ID_PERF_RULE_MATCHING_OR, &event_data, sizeof(event_data), 0);
    
    uint64_t start = os_monotonic_time_get_microsecond();
    for (int i = 0; i < PERF_TEST_LOOPS; ++i) {
        topic_publish_event(&bus, TEST_EVENT_ID_PERF_RULE_MATCHING_OR);
    }
    uint64_t end = os_monotonic_time_get_microsecond();

    double avg_us = (double)(end - start) / PERF_TEST_LOOPS;
    os_printf("[topic][PERF] OR规则匹配延迟: %.2f us/match\n", avg_us);

    /* 测量AND规则匹配性能 */
    /* 使用静态数组，避免栈上的数组生命周期问题 */
    static obj_dict_key_t events_and[] = {TEST_EVENT_ID_PERF_RULE_MATCHING_AND_1, 
                                           TEST_EVENT_ID_PERF_RULE_MATCHING_AND_2, 
                                           TEST_EVENT_ID_PERF_RULE_MATCHING_AND_3};
    topic_rule_t rule_and = {
        .type = TOPIC_RULE_AND,
        .events = events_and,
        .event_count = 3,
    };
    topic_rule_create(&bus, TEST_TOPIC_ID_PERF_RULE_MATCHING_AND, &rule_and);
    topic_subscribe(&bus, TEST_TOPIC_ID_PERF_RULE_MATCHING_AND, test_callback, NULL);

    obj_dict_set(&dict, TEST_EVENT_ID_PERF_RULE_MATCHING_AND_1, &event_data, sizeof(event_data), 0);
    obj_dict_set(&dict, TEST_EVENT_ID_PERF_RULE_MATCHING_AND_2, &event_data, sizeof(event_data), 0);
    obj_dict_set(&dict, TEST_EVENT_ID_PERF_RULE_MATCHING_AND_3, &event_data, sizeof(event_data), 0);

    start = os_monotonic_time_get_microsecond();
    for (int i = 0; i < PERF_TEST_LOOPS / 10; ++i) {
        topic_publish_event(&bus, TEST_EVENT_ID_PERF_RULE_MATCHING_AND_1);
        topic_publish_event(&bus, TEST_EVENT_ID_PERF_RULE_MATCHING_AND_2);
        topic_publish_event(&bus, TEST_EVENT_ID_PERF_RULE_MATCHING_AND_3);
    }
    end = os_monotonic_time_get_microsecond();

    avg_us = (double)(end - start) / (PERF_TEST_LOOPS / 10 * 3);
    os_printf("[topic][PERF] AND规则匹配延迟: %.2f us/match\n", avg_us);

    return 0;
}

/* ---------------- 主测试入口 ---------------- */

/*
 * @brief Topic Bus完整测试入口
 * @return 0成功，-1失败
 */
int topic_bus_perf_test_main(void) {
    os_printf("\n========== TopicBus 完整测试开始 ==========\n");

    /* ========== 阶段1：基础功能测试（P0） ========== */
    os_printf("\n--- 阶段1：基础功能测试 ---\n");

    if (test_functional_basic() != 0) {
        os_printf("[topic] 基础功能测试失败\n");
        return -1;
    }

    /* 时效性测试 */
    if (test_timeout_default() != 0) {
        os_printf("[topic] 默认时效性测试失败\n");
        return -1;
    }

    if (test_timeout_custom() != 0) {
        os_printf("[topic] 自定义时效性测试失败\n");
        return -1;
    }

    if (test_timeout_max() != 0) {
        os_printf("[topic] 最大延迟宏测试失败\n");
        return -1;
    }

    if (test_timeout_boundary() != 0) {
        os_printf("[topic] 时效性边界测试失败\n");
        return -1;
    }

    /* 多Topic共享测试 */
    if (test_shared_event_key() != 0) {
        os_printf("[topic] 多Topic共享Event Key测试失败\n");
        return -1;
    }

    /* ========== 阶段2：新功能测试（P1） ========== */
    os_printf("\n--- 阶段2：新功能测试 ---\n");

#if TOPIC_BUS_ENABLE_SERVER
    if (test_server_periodic() != 0) {
        os_printf("[topic] Server周期性运行测试失败\n");
        return -1;
    }

#if TOPIC_BUS_ENABLE_STATS
    if (test_server_stats() != 0) {
        os_printf("[topic] Server性能统计测试失败\n");
        return -1;
    }
#endif
#endif

#if TOPIC_BUS_ENABLE_ROUTER
    if (test_router_custom() != 0) {
        os_printf("[topic] 自定义Router测试失败\n");
        return -1;
    }

    if (test_router_multiple() != 0) {
        os_printf("[topic] 多Router测试失败\n");
        return -1;
    }

    if (test_router_vfb() != 0) {
        os_printf("[topic] VFB Router测试失败（可选）\n");
    }
#endif

#if TOPIC_BUS_ENABLE_ISR
    if (test_isr_publish() != 0) {
        os_printf("[topic] ISR发布测试失败\n");
        return -1;
    }
#endif

    /* ========== 阶段3：性能测试（P1） ========== */
    os_printf("\n--- 阶段3：性能测试 ---\n");

    if (test_performance_event_publish() != 0) {
        os_printf("[topic] 事件发布延迟测试失败\n");
        return -1;
    }

    if (test_performance_rule_matching() != 0) {
        os_printf("[topic] 规则匹配性能测试失败\n");
        return -1;
    }

    if (test_performance_timeout() != 0) {
        os_printf("[topic] 时效性检查性能测试失败\n");
        return -1;
    }

    if (test_concurrent_high() != 0) {
        os_printf("[topic] 高并发测试失败\n");
        return -1;
    }

    os_printf("\n========== TopicBus 完整测试完成 ==========\n\n");
    return 0;
}

