#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "ring_buffer.h"
#include "../../Rte/inc/os_timestamp.h"
#include "../../Rte/inc/os_printf.h"

typedef struct {
    uint32_t v;
} sample_item_t;

typedef struct {
    uint32_t len;
    uint8_t  data[256];
} var_item_256_t;

/*
 * @brief 基础功能测试：满/空边界、顺序、阻塞超时
 * @return 0成功，-1失败
 */
static int test_functional_basic(void) {
    os_printf("\n[ringbuf][FUNC] 基础功能测试: 满/空/顺序/非阻塞/阻塞\n");

    const size_t cap = 16;
    ring_buffer_t* rb = ring_buffer_create(cap, sizeof(sample_item_t));
    if (!rb) {
        os_printf("[ringbuf][FUNC] 创建失败\n");
        return -1;
    }

    /* 非阻塞：写满 */
    for (size_t i = 0; i < cap; ++i) {
        sample_item_t it = { .v = (uint32_t)i };
        if (ring_buffer_write(rb, &it) != 0) {
            os_printf("[ringbuf][FUNC] 非阻塞写失败 i=%zu\n", i);
            ring_buffer_destroy(rb);
            return -1;
        }
    }
    /* 满时应失败 */
    sample_item_t tmp = { .v = 0xFFFFFFFFu };
    if (ring_buffer_write(rb, &tmp) == 0) {
        os_printf("[ringbuf][FUNC] 非阻塞写在满队列时未失败\n");
        ring_buffer_destroy(rb);
        return -1;
    }

    /* 非阻塞：读空并校验顺序 */
    for (size_t i = 0; i < cap; ++i) {
        sample_item_t it = {0};
        if (ring_buffer_read(rb, &it) != 0 || it.v != (uint32_t)i) {
            os_printf("[ringbuf][FUNC] 非阻塞读校验失败 i=%zu v=%u\n", i, it.v);
            ring_buffer_destroy(rb);
            return -1;
        }
    }
    /* 空时应失败 */
    if (ring_buffer_read(rb, &tmp) == 0) {
        os_printf("[ringbuf][FUNC] 非阻塞读在空队列时未失败\n");
        ring_buffer_destroy(rb);
        return -1;
    }

#if RING_BUFFER_ENABLE_BLOCKING
    /* 阻塞接口：在空情况下读应超时，在满情况下写应超时 */
    sample_item_t it = { .v = 0xA5A5A5A5u };
    if (ring_buffer_read_blocking(rb, &tmp, 5) == 0) {
        os_printf("[ringbuf][FUNC] 阻塞读在空队列未超时\n");
        ring_buffer_destroy(rb);
        return -1;
    }
    for (size_t i = 0; i < cap; ++i) {
        if (ring_buffer_write_blocking(rb, &it, 5) != 0) {
            os_printf("[ringbuf][FUNC] 阻塞写提前失败 i=%zu\n", i);
            ring_buffer_destroy(rb);
            return -1;
        }
    }
    if (ring_buffer_write_blocking(rb, &it, 5) == 0) {
        os_printf("[ringbuf][FUNC] 阻塞写在满队列未超时\n");
        ring_buffer_destroy(rb);
        return -1;
    }
#endif

    ring_buffer_destroy(rb);
    os_printf("[ringbuf][FUNC] 基础功能测试: 通过\n");
    return 0;
}

/*
 * @brief 固定item大小的吞吐性能测试
 * @return 0成功，-1失败
 */
static int test_performance_sizes(void) {
    os_printf("\n[ringbuf][PERF] 固定item大小吞吐测试\n");
    const size_t sizes[] = { 1, 4, 8, 16, 64, 128, 256 };
    const size_t cap_items = 1024;
    const size_t loops = 200000u;

    for (size_t si = 0; si < sizeof(sizes)/sizeof(sizes[0]); ++si) {
        size_t item_size = sizes[si];
        ring_buffer_t* rb = ring_buffer_create(cap_items, item_size);
        if (!rb) {
            os_printf("[ringbuf][PERF] 创建失败 item_size=%zu\n", item_size);
            return -1;
        }

        uint8_t wbuf[256];
        uint8_t rbuf[256];
        memset(wbuf, 0x5A, sizeof(wbuf));
        memset(rbuf, 0x00, sizeof(rbuf));

        uint64_t t0 = os_monotonic_time_get_microsecond();
        for (size_t i = 0; i < loops; ++i) {
            while (ring_buffer_write(rb, wbuf) != 0) {
                /* busy */
            }
            while (ring_buffer_read(rb, rbuf) != 0) {
                /* busy */
            }
        }
        uint64_t t1 = os_monotonic_time_get_microsecond();
        uint64_t us = (t1 >= t0) ? (t1 - t0) : 0;
        double ns_per_op = loops ? (double)us * 1000.0 / (double)(loops * 2) : 0.0;
        double mbps = loops ? ((double)item_size * (double)loops * 2.0) / ((double)us) : 0.0; /* MB/s近似 */

        os_printf("[ringbuf][PERF] item=%4zuB  loops=%zu  time=%llu us  avg=%.2f ns/op  thr=%.2f MB/s\n",
                  item_size, loops, (unsigned long long)us, ns_per_op, mbps);

        ring_buffer_destroy(rb);
    }

    return 0;
}

/*
 * @brief 变长item（头部含len）吞吐性能与正确性测试
 * @return 0成功，-1失败
 */
static int test_performance_varlen(void) {
    os_printf("\n[ringbuf][PERF] 变长item(头含len)吞吐测试 (max 256B)\n");
    const size_t cap_items = 1024;
    ring_buffer_t* rb = ring_buffer_create(cap_items, sizeof(var_item_256_t));
    if (!rb) {
        os_printf("[ringbuf][PERF] 创建失败 varlen\n");
        return -1;
    }

    const uint8_t lengths[] = { 0, 1, 7, 31, 63, 127, 255 };
    const size_t loops = 100000u;
    var_item_256_t w = {0};
    var_item_256_t r = {0};

    uint64_t t0 = os_monotonic_time_get_microsecond();
    for (size_t i = 0; i < loops; ++i) {
        w.len = lengths[i % (sizeof(lengths))];
        memset(w.data, (int)(w.len & 0xFF), w.len);
        /* 整个结构搬运，len字段用于模拟变长 */
        while (ring_buffer_write(rb, &w) != 0) {
            /* busy */
        }
        while (ring_buffer_read(rb, &r) != 0) {
            /* busy */
        }
        if (r.len != w.len || (r.len && memcmp(r.data, w.data, r.len) != 0)) {
            os_printf("[ringbuf][PERF] 变长数据校验失败 len=%u\n", r.len);
            ring_buffer_destroy(rb);
            return -1;
        }
    }
    uint64_t t1 = os_monotonic_time_get_microsecond();
    uint64_t us = (t1 >= t0) ? (t1 - t0) : 0;
    double avg_ns = loops ? (double)us * 1000.0 / (double)(loops * 2) : 0.0;
    os_printf("[ringbuf][PERF] varlen(max256) loops=%zu time=%llu us avg=%.2f ns/op\n",
              loops, (unsigned long long)us, avg_ns);

    ring_buffer_destroy(rb);
    return 0;
}

/*
 * @brief 环形队列接口功能与性能测试入口
 * @return 0成功，-1失败
 */
int ring_buffer_perf_test_main(void) {
    os_printf("\n========== RingBuffer 接口功能与性能测试 =========\n");

    /* 功能测试 */
    if (test_functional_basic() != 0) {
        os_printf("[ringbuf] 功能测试失败\n");
        return -1;
    }

    /* 性能测试：不同固定item大小 */
    if (test_performance_sizes() != 0) {
        os_printf("[ringbuf] 固定大小性能测试失败\n");
        return -1;
    }

    /* 性能测试：变长语义（头携带len） */
    if (test_performance_varlen() != 0) {
        os_printf("[ringbuf] 变长性能测试失败\n");
        return -1;
    }

    os_printf("========== RingBuffer 测试完成 =========\n\n");
    return 0;
}


