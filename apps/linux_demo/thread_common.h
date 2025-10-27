#ifndef THREAD_COMMON_H
#define THREAD_COMMON_H

#include <stdatomic.h>
#include "../../Rte/inc/os_mutex.h"

// 线程间通信的共享数据结构
typedef struct {
    int message;
    int count;
    atomic_int stop_flag;
} ThreadData_t;

// 全局变量声明
extern ThreadData_t g_thread_data;
extern OsMutex_t* g_mutex;

// 线程函数声明
void* thread_a_entry(void* pParam);
void* thread_b_entry(void* pParam);

#endif // THREAD_COMMON_H
