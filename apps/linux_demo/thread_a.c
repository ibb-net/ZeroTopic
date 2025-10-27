#include <stdio.h>
#include "../../Rte/inc/os_printf.h"
#include "../../Rte/inc/os_thread.h"
#include "thread_common.h"

// 线程A函数实现
void* thread_a_entry(void* pParam)
{
    ThreadData_t* pData = (ThreadData_t*)pParam;
    
    os_printf("[线程A] 启动\n");
    
    for (int i = 0; i < 5; i++) {
        // 获取互斥锁
        if (os_mutex_take(g_mutex) < 0) {
            os_printf("[线程A] 获取互斥锁失败\n");
            break;
        }
        
        // 发送消息
        pData->message = 100 + i;  // 消息内容：100, 101, 102, 103, 104
        pData->count = i + 1;
        
        os_printf("[线程A] 发送消息: %d (第%d次交互)\n", pData->message, pData->count);
        
        // 释放互斥锁
        os_mutex_give(g_mutex);
        
        // 休眠500ms
        os_thread_sleep_ms(500);
        
        // 检查停止标志
        if (atomic_load(&pData->stop_flag)) {
            break;
        }
    }
    
    os_printf("[线程A] 完成所有交互，退出\n");
    return NULL;
}
