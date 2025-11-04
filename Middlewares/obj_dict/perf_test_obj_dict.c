#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>
#include "obj_dict.h"
#include "../../Rte/inc/os_timestamp.h"
#include "../../Rte/inc/os_printf.h"
#include "../../Rte/inc/os_thread.h"
#include "../../Rte/inc/os_heap.h"

/* 测试配置 */
#define PERF_TEST_MAX_KEYS        100
#define PERF_TEST_DATA_SIZE       64
#define PERF_TEST_LOOPS_SINGLE    100000
#define PERF_TEST_LOOPS_THREAD    100000
#define PERF_TEST_THREAD_COUNT    4

/* 测试数据结构 */
typedef struct {
    uint32_t value;
    uint32_t counter;
    uint8_t  padding[56];  /* 总64字节 */
} test_data_t;

/* 线程测试参数 */
typedef struct {
    obj_dict_t* dict;
    obj_dict_key_t start_key;
    obj_dict_key_t key_count;
    uint32_t iterations;
    atomic_uint errors;
    atomic_uint writes;
    atomic_uint reads;
} thread_test_param_t;

/* ========== 基础功能测试 ========== */

static int test_functional_basic(void) {
    os_printf("\n[objdict][FUNC] 基础功能测试: set/get/iterate/version\n");

    obj_dict_entry_t entry_array[10];
    obj_dict_t dict;
    
    if (obj_dict_init(&dict, entry_array, 10) != 0) {
        os_printf("[objdict][FUNC] 初始化失败\n");
        return -1;
    }

    /* 测试set/get */
    test_data_t data_in = { .value = 0x12345678, .counter = 0, .padding = {0} };
    test_data_t data_out = {0};
    
    if (obj_dict_set(&dict, 100, &data_in, sizeof(data_in), 0) != 0) {
        os_printf("[objdict][FUNC] set失败\n");
        return -1;
    }

    ssize_t len = obj_dict_get(&dict, 100, &data_out, sizeof(data_out), NULL, NULL, NULL);
    if (len != sizeof(data_in) || data_out.value != data_in.value) {
        os_printf("[objdict][FUNC] get失败 len=%zd value=0x%08x\n", len, data_out.value);
        return -1;
    }

    /* 测试版本号 */
    uint32_t ver1 = 0, ver2 = 0;
    data_in.value = 0xABCDEF00;
    obj_dict_set(&dict, 100, &data_in, sizeof(data_in), 0);
    obj_dict_get(&dict, 100, &data_out, sizeof(data_out), NULL, &ver1, NULL);
    
    data_in.value = 0x11111111;
    obj_dict_set(&dict, 100, &data_in, sizeof(data_in), 0);
    obj_dict_get(&dict, 100, &data_out, sizeof(data_out), NULL, &ver2, NULL);
    
    if (ver2 <= ver1) {
        os_printf("[objdict][FUNC] 版本号未递增 ver1=%u ver2=%u\n", ver1, ver2);
        return -1;
    }

    /* 测试遍历 */
    obj_dict_set(&dict, 101, &data_in, sizeof(data_in), 0);
    obj_dict_set(&dict, 102, &data_in, sizeof(data_in), 0);
    
    int found = 0;
    for (int idx = -1; (idx = obj_dict_iterate(&dict, idx)) >= 0;) {
        found++;
    }
    if (found != 3) {  /* 应该包含100,101,102三个key */
        os_printf("[objdict][FUNC] 遍历结果不对 found=%d\n", found);
        return -1;
    }

    os_printf("[objdict][FUNC] 基础功能测试: 通过\n");
    return 0;
}

/* ========== 性能测试：吞吐量 ========== */

static int test_performance_throughput(void) {
    os_printf("\n[objdict][PERF] 吞吐量测试: 连续写入/读取\n");

    obj_dict_entry_t entry_array[PERF_TEST_MAX_KEYS];
    obj_dict_t dict;
    
    if (obj_dict_init(&dict, entry_array, PERF_TEST_MAX_KEYS) != 0) {
        os_printf("[objdict][PERF] 初始化失败\n");
        return -1;
    }

    test_data_t data = {0};
    const size_t loop_count = PERF_TEST_LOOPS_SINGLE;
    
    /* 写入吞吐量 */
    uint64_t t0 = os_monotonic_time_get_microsecond();
    for (size_t i = 0; i < loop_count; ++i) {
        data.value = (uint32_t)i;
        data.counter = (uint32_t)i;
        obj_dict_key_t key = (obj_dict_key_t)(i % PERF_TEST_MAX_KEYS);
        obj_dict_set(&dict, key, &data, sizeof(data), 0);
    }
    uint64_t t1 = os_monotonic_time_get_microsecond();
    uint64_t us_write = (t1 >= t0) ? (t1 - t0) : 0;
    double write_ns_per_op = loop_count ? (double)us_write * 1000.0 / (double)loop_count : 0.0;

    /* 读取吞吐量 */
    t0 = os_monotonic_time_get_microsecond();
    for (size_t i = 0; i < loop_count; ++i) {
        obj_dict_key_t key = (obj_dict_key_t)(i % PERF_TEST_MAX_KEYS);
        test_data_t read_data = {0};
        obj_dict_get(&dict, key, &read_data, sizeof(read_data), NULL, NULL, NULL);
    }
    t1 = os_monotonic_time_get_microsecond();
    uint64_t us_read = (t1 >= t0) ? (t1 - t0) : 0;
    double read_ns_per_op = loop_count ? (double)us_read * 1000.0 / (double)loop_count : 0.0;

    /* 混合吞吐量（交替读写） */
    t0 = os_monotonic_time_get_microsecond();
    for (size_t i = 0; i < loop_count; ++i) {
        data.value = (uint32_t)i;
        data.counter = (uint32_t)i;
        obj_dict_key_t key = (obj_dict_key_t)(i % PERF_TEST_MAX_KEYS);
        obj_dict_set(&dict, key, &data, sizeof(data), 0);
        
        test_data_t read_data = {0};
        obj_dict_get(&dict, key, &read_data, sizeof(read_data), NULL, NULL, NULL);
    }
    t1 = os_monotonic_time_get_microsecond();
    uint64_t us_mixed = (t1 >= t0) ? (t1 - t0) : 0;
    double mixed_ns_per_op = loop_count ? (double)us_mixed * 1000.0 / (double)(loop_count * 2) : 0.0;

    os_printf("[objdict][PERF] 写入: loops=%zu time=%llu us avg=%.2f ns/op\n",
              loop_count, (unsigned long long)us_write, write_ns_per_op);
    os_printf("[objdict][PERF] 读取: loops=%zu time=%llu us avg=%.2f ns/op\n",
              loop_count, (unsigned long long)us_read, read_ns_per_op);
    os_printf("[objdict][PERF] 混合: loops=%zu time=%llu us avg=%.2f ns/op\n",
              loop_count, (unsigned long long)us_mixed, mixed_ns_per_op);

    return 0;
}

/* ========== 性能测试：不同数据大小 ========== */

static int test_performance_different_sizes(void) {
    os_printf("\n[objdict][PERF] 不同数据大小性能测试\n");

    obj_dict_entry_t entry_array[PERF_TEST_MAX_KEYS];
    obj_dict_t dict;
    
    if (obj_dict_init(&dict, entry_array, PERF_TEST_MAX_KEYS) != 0) {
        os_printf("[objdict][PERF] 初始化失败\n");
        return -1;
    }

    const size_t data_sizes[] = { 4, 8, 16, 32, 64, 128, 256 };
    const size_t loop_count = PERF_TEST_LOOPS_SINGLE;

    for (size_t si = 0; si < sizeof(data_sizes)/sizeof(data_sizes[0]); ++si) {
        size_t data_size = data_sizes[si];
        uint8_t* wbuf = (uint8_t*)os_malloc(data_size);
        uint8_t* rbuf = (uint8_t*)os_malloc(data_size);
        
        if (!wbuf || !rbuf) {
            os_printf("[objdict][PERF] 分配内存失败 size=%zu\n", data_size);
            if (wbuf) os_free(wbuf);
            if (rbuf) os_free(rbuf);
            continue;
        }
        
        memset(wbuf, 0x5A, data_size);
        memset(rbuf, 0x00, data_size);

        uint64_t t0 = os_monotonic_time_get_microsecond();
        for (size_t i = 0; i < loop_count; ++i) {
            obj_dict_key_t key = (obj_dict_key_t)(i % PERF_TEST_MAX_KEYS);
            obj_dict_set(&dict, key, wbuf, data_size, 0);
            obj_dict_get(&dict, key, rbuf, data_size, NULL, NULL, NULL);
        }
        uint64_t t1 = os_monotonic_time_get_microsecond();
        uint64_t us = (t1 >= t0) ? (t1 - t0) : 0;
        double ns_per_op = loop_count ? (double)us * 1000.0 / (double)(loop_count * 2) : 0.0;

        os_printf("[objdict][PERF] size=%4zuB  loops=%zu  time=%llu us  avg=%.2f ns/op\n",
                  data_size, loop_count, (unsigned long long)us, ns_per_op);

        os_free(wbuf);
        os_free(rbuf);
    }

    return 0;
}

/* ========== 多线程测试：写线程 ========== */

static void* thread_write_entry(void* param) {
    thread_test_param_t* p = (thread_test_param_t*)param;
    test_data_t data = {0};
    uint32_t success_count = 0;

    for (uint32_t i = 0; i < p->iterations; ++i) {
        data.value = i;
        data.counter = success_count;
        obj_dict_key_t key = p->start_key + (i % p->key_count);
        
        if (obj_dict_set(p->dict, key, &data, sizeof(data), 0) == 0) {
            atomic_fetch_add_explicit(&p->writes, 1, memory_order_relaxed);
            success_count++;
        } else {
            atomic_fetch_add_explicit(&p->errors, 1, memory_order_relaxed);
        }
    }

    return NULL;
}

/* ========== 多线程测试：读线程 ========== */

static void* thread_read_entry(void* param) {
    thread_test_param_t* p = (thread_test_param_t*)param;
    test_data_t data = {0};
    uint32_t success_count = 0;

    for (uint32_t i = 0; i < p->iterations; ++i) {
        obj_dict_key_t key = p->start_key + (i % p->key_count);
        
        if (obj_dict_get(p->dict, key, &data, sizeof(data), NULL, NULL, NULL) > 0) {
            atomic_fetch_add_explicit(&p->reads, 1, memory_order_relaxed);
            success_count++;
        } else {
            atomic_fetch_add_explicit(&p->errors, 1, memory_order_relaxed);
        }
    }

    return NULL;
}

/* ========== 多线程测试：读写混合线程 ========== */

static void* thread_rw_entry(void* param) {
    thread_test_param_t* p = (thread_test_param_t*)param;
    test_data_t data = {0};
    uint32_t success_count = 0;

    for (uint32_t i = 0; i < p->iterations; ++i) {
        data.value = i;
        data.counter = success_count;
        obj_dict_key_t key = p->start_key + (i % p->key_count);
        
        /* 写入 */
        if (obj_dict_set(p->dict, key, &data, sizeof(data), 0) == 0) {
            atomic_fetch_add_explicit(&p->writes, 1, memory_order_relaxed);
            
            /* 读取 */
            test_data_t read_data = {0};
            if (obj_dict_get(p->dict, key, &read_data, sizeof(read_data), NULL, NULL, NULL) > 0) {
                atomic_fetch_add_explicit(&p->reads, 1, memory_order_relaxed);
                if (read_data.value == data.value) {
                    success_count++;
                } else {
                    atomic_fetch_add_explicit(&p->errors, 1, memory_order_relaxed);
                }
            }
        } else {
            atomic_fetch_add_explicit(&p->errors, 1, memory_order_relaxed);
        }
    }

    return NULL;
}

/* ========== 多线程并发测试：纯写 ========== */

static int test_threads_write_only(void) {
    os_printf("\n[objdict][THREAD] 纯写入并发测试: %d线程\n", PERF_TEST_THREAD_COUNT);

    obj_dict_entry_t entry_array[PERF_TEST_MAX_KEYS];
    obj_dict_t dict;
    
    if (obj_dict_init(&dict, entry_array, PERF_TEST_MAX_KEYS) != 0) {
        os_printf("[objdict][THREAD] 初始化失败\n");
        return -1;
    }

    /* 创建线程 */
    OsThread_t* threads[PERF_TEST_THREAD_COUNT];
    thread_test_param_t params[PERF_TEST_THREAD_COUNT];
    ThreadAttr_t attr = {
        .pName = "Writer",
        .Priority = 5,
        .StackSize = 4096,
        .ScheduleType = 0
    };

    for (int i = 0; i < PERF_TEST_THREAD_COUNT; ++i) {
        params[i].dict = &dict;
        params[i].start_key = (obj_dict_key_t)(i * (PERF_TEST_MAX_KEYS / PERF_TEST_THREAD_COUNT));
        params[i].key_count = PERF_TEST_MAX_KEYS / PERF_TEST_THREAD_COUNT;
        params[i].iterations = PERF_TEST_LOOPS_THREAD;
        atomic_init(&params[i].errors, 0);
        atomic_init(&params[i].writes, 0);
        atomic_init(&params[i].reads, 0);

        attr.pName = "Writer";
        threads[i] = os_thread_create(thread_write_entry, &params[i], &attr);
        if (!threads[i]) {
            os_printf("[objdict][THREAD] 创建线程失败 i=%d\n", i);
            return -1;
        }
    }

    /* 测试耗时 */
    uint64_t t0 = os_monotonic_time_get_microsecond();
    
    /* 等待所有线程完成 */
    for (int i = 0; i < PERF_TEST_THREAD_COUNT; ++i) {
        os_thread_join(threads[i]);
        os_thread_destroy(threads[i]);
    }
    
    uint64_t t1 = os_monotonic_time_get_microsecond();
    uint64_t us = (t1 >= t0) ? (t1 - t0) : 0;

    /* 统计结果 */
    uint32_t total_writes = 0, total_errors = 0;
    for (int i = 0; i < PERF_TEST_THREAD_COUNT; ++i) {
        total_writes += atomic_load_explicit(&params[i].writes, memory_order_relaxed);
        total_errors += atomic_load_explicit(&params[i].errors, memory_order_relaxed);
    }

    double total_ops = (double)total_writes;
    double ns_per_op = total_ops ? (double)us * 1000.0 / total_ops : 0.0;

    os_printf("[objdict][THREAD] 纯写: threads=%d iter=%u  writes=%u errors=%u  time=%llu us  avg=%.2f ns/op\n",
              PERF_TEST_THREAD_COUNT, PERF_TEST_LOOPS_THREAD, total_writes, total_errors,
              (unsigned long long)us, ns_per_op);

    if (total_errors > total_writes / 100) {  /* 允许1%错误率 */
        os_printf("[objdict][THREAD] 错误率过高\n");
        return -1;
    }

    return 0;
}

/* ========== 多线程并发测试：读写混合 ========== */

static int test_threads_read_write_mixed(void) {
    os_printf("\n[objdict][THREAD] 读写混合并发测试: %d线程\n", PERF_TEST_THREAD_COUNT);

    obj_dict_entry_t entry_array[PERF_TEST_MAX_KEYS];
    obj_dict_t dict;
    
    if (obj_dict_init(&dict, entry_array, PERF_TEST_MAX_KEYS) != 0) {
        os_printf("[objdict][THREAD] 初始化失败\n");
        return -1;
    }

    /* 预先写入一些数据 */
    test_data_t init_data = { .value = 0xABCDEF00, .counter = 0, .padding = {0} };
    for (int i = 0; i < PERF_TEST_MAX_KEYS; ++i) {
        obj_dict_set(&dict, (obj_dict_key_t)i, &init_data, sizeof(init_data), 0);
    }

    /* 创建线程：混合读写 */
    OsThread_t* threads[PERF_TEST_THREAD_COUNT];
    thread_test_param_t params[PERF_TEST_THREAD_COUNT];
    ThreadAttr_t attr = {
        .pName = "RW",
        .Priority = 5,
        .StackSize = 4096,
        .ScheduleType = 0
    };

    for (int i = 0; i < PERF_TEST_THREAD_COUNT; ++i) {
        params[i].dict = &dict;
        params[i].start_key = (obj_dict_key_t)(i * (PERF_TEST_MAX_KEYS / PERF_TEST_THREAD_COUNT));
        params[i].key_count = PERF_TEST_MAX_KEYS / PERF_TEST_THREAD_COUNT;
        params[i].iterations = PERF_TEST_LOOPS_THREAD;
        atomic_init(&params[i].errors, 0);
        atomic_init(&params[i].writes, 0);
        atomic_init(&params[i].reads, 0);

        attr.pName = "RW";
        threads[i] = os_thread_create(thread_rw_entry, &params[i], &attr);
        if (!threads[i]) {
            os_printf("[objdict][THREAD] 创建线程失败 i=%d\n", i);
            return -1;
        }
    }

    /* 测试耗时 */
    uint64_t t0 = os_monotonic_time_get_microsecond();
    
    /* 等待所有线程完成 */
    for (int i = 0; i < PERF_TEST_THREAD_COUNT; ++i) {
        os_thread_join(threads[i]);
        os_thread_destroy(threads[i]);
    }
    
    uint64_t t1 = os_monotonic_time_get_microsecond();
    uint64_t us = (t1 >= t0) ? (t1 - t0) : 0;

    /* 统计结果 */
    uint32_t total_writes = 0, total_reads = 0, total_errors = 0;
    for (int i = 0; i < PERF_TEST_THREAD_COUNT; ++i) {
        total_writes += atomic_load_explicit(&params[i].writes, memory_order_relaxed);
        total_reads += atomic_load_explicit(&params[i].reads, memory_order_relaxed);
        total_errors += atomic_load_explicit(&params[i].errors, memory_order_relaxed);
    }

    double total_ops = (double)(total_writes + total_reads);
    double ns_per_op = total_ops ? (double)us * 1000.0 / total_ops : 0.0;

    os_printf("[objdict][THREAD] 混合: threads=%d iter=%u  writes=%u reads=%u errors=%u  time=%llu us  avg=%.2f ns/op\n",
              PERF_TEST_THREAD_COUNT, PERF_TEST_LOOPS_THREAD, total_writes, total_reads, total_errors,
              (unsigned long long)us, ns_per_op);

    if (total_errors > total_writes / 50) {  /* 允许2%错误率 */
        os_printf("[objdict][THREAD] 错误率过高\n");
        return -1;
    }

    return 0;
}

/* ========== 版本一致性测试 ========== */

static int test_version_consistency(void) {
    os_printf("\n[objdict][VERSION] 版本一致性测试\n");

    obj_dict_entry_t entry_array[10];
    obj_dict_t dict;
    
    if (obj_dict_init(&dict, entry_array, 10) != 0) {
        os_printf("[objdict][VERSION] 初始化失败\n");
        return -1;
    }

    test_data_t data = {0};
    uint32_t versions[100] = {0};
    
    /* 连续写入同一key 100次，版本号应严格递增 */
    for (int i = 0; i < 100; ++i) {
        data.value = i;
        obj_dict_set(&dict, 100, &data, sizeof(data), 0);
        obj_dict_get(&dict, 100, NULL, 0, NULL, &versions[i], NULL);
    }

    /* 检查版本号是否递增 */
    for (int i = 1; i < 100; ++i) {
        if (versions[i] <= versions[i-1]) {
            os_printf("[objdict][VERSION] 版本号未递增 i=%d v[%d]=%u v[%d]=%u\n",
                      i, i-1, versions[i-1], i, versions[i]);
            return -1;
        }
    }

    /* 检查版本号是否连续（允许不连续，但不应该回退） */
    os_printf("[objdict][VERSION] 版本号检查: 通过 (最终版本号=%u)\n", versions[99]);
    return 0;
}

/* ========== 主测试入口 ========== */

int obj_dict_perf_test_main(void) {
    os_printf("\n========== ObjDict 接口功能、性能与并发测试 =========\n");

    /* 功能测试 */
    if (test_functional_basic() != 0) {
        os_printf("[objdict] 功能测试失败\n");
        return -1;
    }

    /* 性能测试：吞吐量 */
    if (test_performance_throughput() != 0) {
        os_printf("[objdict] 吞吐量测试失败\n");
        return -1;
    }

    /* 性能测试：不同数据大小 */
    if (test_performance_different_sizes() != 0) {
        os_printf("[objdict] 数据大小测试失败\n");
        return -1;
    }

    /* 版本一致性测试 */
    if (test_version_consistency() != 0) {
        os_printf("[objdict] 版本一致性测试失败\n");
        return -1;
    }

    /* 多线程测试：纯写 */
    if (test_threads_write_only() != 0) {
        os_printf("[objdict] 多线程写入测试失败\n");
        return -1;
    }

    /* 多线程测试：读写混合 */
    if (test_threads_read_write_mixed() != 0) {
        os_printf("[objdict] 多线程读写混合测试失败\n");
        return -1;
    }

    os_printf("========== ObjDict 测试完成 =========\n\n");
    return 0;
}

