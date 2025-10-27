#ifndef __LINUX_DEMO_CONFIG_H__
#define __LINUX_DEMO_CONFIG_H__

// ==================== 测试功能使能开关 ====================
#define ENABLE_THREAD_DEMO      1  // 线程通信测试
#define ENABLE_VFB_TEST         1  // VFB消息总线测试
#define ENABLE_MUTEX_TEST       0  // 互斥锁测试（预留）
#define ENABLE_QUEUE_TEST       0  // 队列测试（预留）

// ==================== VFB测试配置 ====================
#define VFB_TEST_TIMEOUT_MS     1000   // 接收超时时间(ms)
#define VFB_TEST_MAX_RETRIES    10     // 最大超时重试次数

// ==================== 调试配置 ====================
#define DEBUG_VERBOSE           1      // 详细调试信息

#endif // __LINUX_DEMO_CONFIG_H__

