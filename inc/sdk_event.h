#ifndef __SDK_EVENT_H__
#define __SDK_EVENT_H__

enum
{

    EVENT_LIST_SDK = 0,
    EVENT_LIST_STARTUP,  // 启动事件
    EVENT_LIST_SHUTDOWN, // 关机事件
    EVENT_LIST_SYSTEM,   // 系统事件
    EVENT_LIST_USER,     // 用户事件
    EVENT_LIST_DEVICE_START = 1000,
    EVENT_LIST_APP_START = 2000, // 应用事件起始编号
} EVENT_LIST;
#endif // __SDK_EVENT_H__