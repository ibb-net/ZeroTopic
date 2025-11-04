#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdatomic.h>

// 配置文件
#include "config.h"

// OpenIBBOs RTE头文件
#include "../../Rte/inc/os_init.h"
#include "../../Rte/inc/os_printf.h"
#include "../../Rte/inc/os_thread.h"
#include "../../Rte/inc/os_mutex.h"

// 项目头文件
#include "thread_common.h"
#include "mcu_sim.h"

// VFB测试函数声明
int vfb_test_main(void);
// 环形队列性能测试
int ring_buffer_perf_test_main(void);
// 对象字典性能/并发测试
int obj_dict_perf_test_main(void);
// Topic Bus完整测试
int topic_bus_perf_test_main(void);

// 全局变量定义
ThreadData_t g_thread_data = {0, 0, 0};
OsMutex_t* g_mutex = NULL;

int main(void)
{
    OsThread_t* pThreadA = NULL;
    OsThread_t* pThreadB = NULL;
    ThreadAttr_t threadAttr = {
        .pName = "ThreadA",
        .Priority = 5,
        .StackSize = 4096,
        .ScheduleType = 0
    };
    
    // 初始化OpenIBBOs系统
    if (os_init() < 0) {
        printf("Failed to initialize OpenIBBOs system\n");
        return -1;
    }
    
#if MCU_SIM_ENABLE_PRINT
    // 打印MCU模拟环境配置
    mcu_sim_print_config();
#endif
    
#if ENABLE_THREAD_DEMO
    os_printf("=== OpenIBBOs 线程通信演示 ===\n");
    os_printf("创建两个线程A和B进行5次通信交互\n");
    
    // 创建互斥锁
    g_mutex = os_mutex_create("ThreadCommMutex");
    if (g_mutex == NULL) {
        os_printf("创建互斥锁失败\n");
        os_exit();
        return -1;
    }
    
    // 初始化线程数据
    g_thread_data.message = 0;
    g_thread_data.count = 0;
    g_thread_data.stop_flag = 0;
    
    // 创建线程A
    threadAttr.pName = "ThreadA";
    pThreadA = os_thread_create(thread_a_entry, &g_thread_data, &threadAttr);
    if (pThreadA == NULL) {
        os_printf("创建线程A失败\n");
        os_mutex_destroy(g_mutex);
        os_exit();
        return -1;
    }
    
    // 创建线程B
    threadAttr.pName = "ThreadB";
    pThreadB = os_thread_create(thread_b_entry, &g_thread_data, &threadAttr);
    if (pThreadB == NULL) {
        os_printf("创建线程B失败\n");
        os_thread_destroy(pThreadA);
        os_mutex_destroy(g_mutex);
        os_exit();
        return -1;
    }
    
    os_printf("线程A和线程B已创建，开始通信交互...\n");
    
    // 等待线程A完成
    os_thread_join(pThreadA);
    os_printf("线程A已结束\n");
    
    // 等待线程B完成
    os_thread_join(pThreadB);
    os_printf("线程B已结束\n");
    
    os_printf("=== 所有线程通信交互完成 ===\n");
    os_printf("总共进行了5次交互\n");
    
    // 清理资源
    os_thread_destroy(pThreadA);
    os_thread_destroy(pThreadB);
    os_mutex_destroy(g_mutex);
#endif // ENABLE_THREAD_DEMO
    
#if ENABLE_VFB_TEST
    os_printf("\n=== 开始VFB测试 ===\n");
    
    // 运行VFB测试
    int vfb_result = vfb_test_main();
    if (vfb_result != 0) {
        os_printf("VFB测试失败，错误码: %d\n", vfb_result);
    } else {
        os_printf("VFB测试成功完成\n");
    }
#endif // ENABLE_VFB_TEST

#if ENABLE_RING_BUFFER_TEST
    os_printf("\n=== 开始RingBuffer性能测试 ===\n");
    int rb_result = ring_buffer_perf_test_main();
    if (rb_result != 0) {
        os_printf("RingBuffer测试失败，错误码: %d\n", rb_result);
    } else {
        os_printf("RingBuffer测试成功完成\n");
    }
#endif // ENABLE_RING_BUFFER_TEST

#if ENABLE_OBJ_DICT_TEST
    os_printf("\n=== 开始ObjDict功能/性能/并发测试 ===\n");
    int od_result = obj_dict_perf_test_main();
    if (od_result != 0) {
        os_printf("ObjDict测试失败，错误码: %d\n", od_result);
    } else {
        os_printf("ObjDict测试成功完成\n");
    }
#endif // ENABLE_OBJ_DICT_TEST

#if ENABLE_TOPIC_BUS_TEST
    os_printf("\n=== 开始Topic Bus完整测试 ===\n");
    int tb_result = topic_bus_perf_test_main();
    if (tb_result != 0) {
        os_printf("Topic Bus测试失败，错误码: %d\n", tb_result);
    } else {
        os_printf("Topic Bus测试成功完成\n");
    }
#endif // ENABLE_TOPIC_BUS_TEST
    
    // 清理系统资源
    os_exit();
    
    return 0;
}
