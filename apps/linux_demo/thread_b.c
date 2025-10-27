#include <stdio.h>
#include "../../Rte/inc/os_printf.h"
#include "../../Rte/inc/os_thread.h"
#include "thread_common.h"

// 线程B函数实现
void* thread_b_entry(void* pParam)
{
    ThreadData_t* pData = (ThreadData_t*)pParam;
    
    os_printf("[线程B] 启动\n");
    
    for (int i = 0; i < 5; i++) {
        // 休眠200ms，让线程A先发送消息
        os_thread_sleep_ms(200);
        
        // 获取互斥锁
        if (os_mutex_take(g_mutex) < 0) {
            os_printf("[线程B] 获取互斥锁失败\n");
            break;
        }
        
        // 接收并处理消息
        int received_msg = pData->message;
        int received_count = pData->count;
        
        os_printf("[线程B] 接收消息: %d (第%d次交互)\n", received_msg, received_count);
        
        // 发送回复消息
        pData->message = 200 + i;  // 回复消息：200, 201, 202, 203, 204
        os_printf("[线程B] 回复消息: %d\n", pData->message);
        
        // 释放互斥锁
        os_mutex_give(g_mutex);
        
        // 休眠300ms
        os_thread_sleep_ms(300);
        
        // 检查停止标志
        if (atomic_load(&pData->stop_flag)) {
            break;
        }
    }
    
    os_printf("[线程B] 完成所有交互，退出\n");
    return NULL;
}
