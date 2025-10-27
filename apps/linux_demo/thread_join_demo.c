#include <stdio.h>
#include <unistd.h>
#include <stdatomic.h>

// 演示os_thread_join的作用
typedef struct {
    atomic_int counter;
} DemoData_t;

static DemoData_t g_demo_data = {0};

void* worker_thread(void* pParam)
{
    DemoData_t* pData = (DemoData_t*)pParam;
    
    printf("[工作线程] 开始工作...\n");
    
    for (int i = 0; i < 3; i++) {
        atomic_fetch_add(&pData->counter, 1);
        printf("[工作线程] 完成第%d项任务\n", i + 1);
        sleep(1);  // 模拟工作耗时
    }
    
    printf("[工作线程] 所有工作完成，准备退出\n");
    return NULL;
}

int main(void)
{
    printf("=== os_thread_join 演示 ===\n");
    
    // 创建线程
    pthread_t thread;
    pthread_create(&thread, NULL, worker_thread, &g_demo_data);
    
    printf("[主线程] 工作线程已创建，开始等待...\n");
    
    // 使用pthread_join等待线程完成（类似os_thread_join）
    pthread_join(thread, NULL);
    
    printf("[主线程] 工作线程已完成，计数器值: %d\n", atomic_load(&g_demo_data.counter));
    printf("[主线程] 现在可以安全退出\n");
    
    return 0;
}
