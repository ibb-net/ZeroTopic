
#ifndef __SDK_TASK_PRIORITY_H__
#define __SDK_TASK_PRIORITY_H__

#define PRIORITY_IDLE_GROUP         (0)
#define PRIORITY_DISPLAY_GROUP      (4)
#define PRIORITY_NORMAL_CYCLE_GROUP (6)
#define PRIORITY_NORMAL_EVENT_GROUP (11)
#define PRIORITY_PROTOCOL_GROUP     (17)
#define PRIORITY_OPERATION_GROUP    (20)
#define PRIORITY_SPECIAL_GROUP      (23)
#define PRIORITY_COMMUNIT_GROUP     (26)
#define PRIORITY_SYSTEM_PRI_GROUP   (30)
#define PRIORITY_MAX_GROUP          (31)
typedef enum {
    /*****************************************************************************/
    PriorityIdleGroup0 = PRIORITY_IDLE_GROUP,  // 空闲组
    PriorityIdleGroup1,
    PriorityIdleGroup2,
    PriorityIdleGroup3,

    /*****************************************************************************/
    PriorityDisplayGroup0 = PRIORITY_DISPLAY_GROUP,  // 显示组
    PriorityDisplayGroup1,

    /*****************************************************************************/
    PriorityNormalCycleGroup0 = PRIORITY_NORMAL_CYCLE_GROUP,  // 普通循环组
    PriorityNormalCycleGroup1,
    PriorityNormalCycleGroup2,
    PriorityNormalCycleGroup3,
    PriorityNormalCycleGroup4,
    PriorityNormalCycleGroup5,

    /*****************************************************************************/
    PriorityNormalEventGroup0 = PRIORITY_NORMAL_EVENT_GROUP,  // 普通事件组
    PriorityNormalEventGroup1,
    PriorityNormalEventGroup2,
    PriorityNormalEventGroup3,
    PriorityNormalEventGroup4,
    PriorityNormalEventGroup5,

    /*****************************************************************************/
    PriorityProtocolGroup0 = PRIORITY_PROTOCOL_GROUP,  // 协议组
    PriorityProtocolGroup1,
    PriorityProtocolGroup2,

    /*****************************************************************************/
    PriorityOperationGroup0 = PRIORITY_OPERATION_GROUP,  // 操作组,关键交互
    PriorityOperationGroup1,
    PriorityOperationGroup2,

    /*****************************************************************************/
    PrioritySpecialGroup0 = PRIORITY_SPECIAL_GROUP,  // 特殊组
    PrioritySpecialGroup1,
    PrioritySpecialGroup2,

    /*****************************************************************************/
    PriorityCommunitGroup = PRIORITY_COMMUNIT_GROUP,  // 通信组
    PriorityCommunitGroup1,
    PriorityCommunitGroup2,
    PriorityCommunitGroup3,

    /*****************************************************************************/
    PrioritySystemPriGroup0 = PRIORITY_SYSTEM_PRI_GROUP,  // 系统优先级组

    /*****************************************************************************/
    PriorityMaxGroup = PRIORITY_MAX_GROUP,  // 最大优先级组
    /*****************************************************************************/

} EnumPriority;
#endif