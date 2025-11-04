#ifndef __LINUX_DEMO_CONFIG_H__
#define __LINUX_DEMO_CONFIG_H__

// ==================== 测试功能使能开关 ====================
#define ENABLE_THREAD_DEMO      0  // 线程通信测试
#define ENABLE_VFB_TEST         0  // VFB消息总线测试
#define ENABLE_MUTEX_TEST       0  // 互斥锁测试（预留）
#define ENABLE_QUEUE_TEST       0  // 队列测试（预留）
#define ENABLE_RING_BUFFER_TEST 0  // 环形缓冲区测试
#define ENABLE_OBJ_DICT_TEST    0  // 对象字典性能与并发测试
#define ENABLE_TOPIC_BUS_TEST   1  // Topic Bus完整测试

// ==================== VFB测试配置 ====================
#define VFB_TEST_TIMEOUT_MS     1000   // 接收超时时间(ms)
#define VFB_TEST_MAX_RETRIES    10     // 最大超时重试次数

// ==================== 调试配置 ====================
#define DEBUG_VERBOSE           1      // 详细调试信息

// ==================== MCU模拟环境配置 ====================
#ifndef MCU_SIM_CORE_FREQ_MHZ
#define MCU_SIM_CORE_FREQ_MHZ 180      // MCU主频（MHz），默认180MHz
#endif

#define MCU_SIM_SYSTEM_BITS 32        // MCU系统位数，32位
#define MCU_SIM_ENABLE_PRINT 1         // 是否在启动时打印MCU配置信息

#endif // __LINUX_DEMO_CONFIG_H__

