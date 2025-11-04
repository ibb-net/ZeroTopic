#ifndef RING_BUFFER_CONFIG_H_
#define RING_BUFFER_CONFIG_H_

/* 是否启用阻塞读写(通过信号量)，0=非阻塞，1=支持阻塞接口 */
#ifndef RING_BUFFER_ENABLE_BLOCKING
#define RING_BUFFER_ENABLE_BLOCKING 1
#endif

/* 是否启用ISR安全写接口 */
#ifndef RING_BUFFER_ENABLE_ISR
#define RING_BUFFER_ENABLE_ISR 1
#endif

/* 环形队列容量是否要求为2的幂(便于位运算优化) */
#ifndef RING_BUFFER_REQUIRE_POWER_OF_TWO
#define RING_BUFFER_REQUIRE_POWER_OF_TWO 1
#endif

/* 是否默认启用零拷贝写入(传入外部缓冲指针) */
#ifndef RING_BUFFER_ENABLE_ZEROCOPY
#define RING_BUFFER_ENABLE_ZEROCOPY 1
#endif

/* 默认元素大小(字节)，仅在创建时未指定时生效 */
#ifndef RING_BUFFER_DEFAULT_ITEM_SIZE
#define RING_BUFFER_DEFAULT_ITEM_SIZE sizeof(uintptr_t)
#endif

#endif /* RING_BUFFER_CONFIG_H_ */

