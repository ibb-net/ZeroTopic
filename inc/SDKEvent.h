#ifndef __SDK_EVENT_H__
#define __SDK_EVENT_H__
#define SDK_EVENT_LIST_APP_START 2000
typedef enum {

    EVENT_LIST_SDK = 0,
    EVENT_LIST_STARTUP,   // 启动事件
    EVENT_LIST_SHUTDOWN,  // 关机事件
    EVENT_LIST_SYSTEM,    // 系统事件
    EVENT_LIST_USER,      // 用户事件
    SDK_EVENT_LIST_DEVICE_START = 1000,
    ENCODER_TIMER_READY,
    ENCODER_TIMER_START,
    ENCODER_TIMER_STOP,
    ENCODER_TIMER_GET_PHY,
    ENCODER_TIMER_SET_PHY,
    ENCODER_TIMER_SET_MODE,
    DebugStart,
    DebugStop,
    DebugSet,
    DebugGet,
    DebugPrint,
    DebugRcv,
    KeyRegister,  // Register key event
    KeyUnRister,  // Unregister key event
    KeyStart,     // Start key task
    KeyStop,      // Stop key task
    UartStart,
    UartStop,
    UartSet,
    UartGet,
    UartSend,
    UartRcv,
    Ds18b20Start,
    Ds18b20Stop,
    Ds18b20Covert,
    Ds18b20GetTemperature,
    DACSgm3533Start,
    DACSgm3533Stop,
    DACSgm3533Get,
    DACSgm3533Set,
    /************************************************* */
    SDK_EVENT_NUM,
    SDK_EVENT_LIST_MAX = SDK_EVENT_LIST_APP_START - 1
} SDK_EVENT_LIST;
#endif  // __SDK_EVENT_H__