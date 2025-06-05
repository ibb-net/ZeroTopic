
#ifndef __SDK_TASK_PRIORITY_H__
#define __SDK_TASK_PRIORITY_H__

typedef enum
{
    SDK_TASK_PRIORITY_IDLE = 0, // 空闲任务优先级
    AppTimereEncoderTaskPriority,
    AppLEDTaskPriority,
    AppDemoTaskPriority,

    SDK_TASK_PRIORITY_LOW, // 低优先级任务

    SDK_TASK_PRIORITY_MEDIUM, // 中等优先级任务


    SDK_TASK_PRIORITY_HIGH, // 高优先级任务
   
    BSP_DEMO_TASK_PRIORITY, // BSP演示任务优先级
    
    SDK_TASK_PRIORITY_HIGHEST, // 最高优先级任务
    BspShellTaskPriority,
    BspDebugTaskPriority,

    SDK_TASK_PRIORITY_REALTIME, // 实时任务优先级
    

    SDK_TASK_PRIORITY_MAX // 最大优先级值

} EnumPriority;
#endif