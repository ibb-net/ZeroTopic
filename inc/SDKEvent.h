#ifndef __SDK_EVENT_H__
#define __SDK_EVENT_H__
#define SDK_EVENT_LIST_APP_START 2000
typedef enum {

    EVENT_LIST_SDK = 0,   // 0
    EVENT_LIST_STARTUP,   // 1 启动事件
    EVENT_LIST_SHUTDOWN,  // 2 关机事件
    EVENT_LIST_SYSTEM,    // 3 系统事件
    EVENT_LIST_USER,      // 4 用户事件

    SDK_EVENT_LIST_DEVICE_START = 1000,  // 1000
    ENCODER_TIMER_READY,                 // 1001
    ENCODER_TIMER_START,                 // 1002
    ENCODER_TIMER_STOP,                  // 1003
    ENCODER_TIMER_GET_PHY,               // 1004
    ENCODER_TIMER_SET_PHY,               // 1005
    ENCODER_TIMER_SET_MODE,              // 1006
    DebugStart,                          // 1007
    DebugStop,                           // 1008
    DebugSet,                            // 1009
    DebugGet,                            // 1010
    DebugPrint,                          // 1011
    DebugRcv,                            // 1012
    KeyRegister,                         // 1013 Register key event
    KeyUnRister,                         // 1014 Unregister key event
    KeyStart,                            // 1015 Start key task
    KeyStop,                             // 1016 Stop key task
    UartStart,                           // 1017
    UartStop,                            // 1018
    UartSet,                             // 1019
    UartGet,                             // 1020
    UartSend,                            // 1021
    UartRcv,                             // 1022
    Ds18b20Start,                        // 1023
    Ds18b20Stop,                         // 1024
    Ds18b20Covert,                       // 1025
    Ds18b20GetTemperature,               // 1026
    DACSgm3533Start,                     // 1027
    DACSgm3533Stop,                      // 1028
    DACSgm3533Get,                       // 1029
    DACSgm3533Set,                       // 1030
    sgm5860xStart,                       // 1031
    sgm5860xStop,                        // 1032
    sgm5860xGet,                         // 1033
    sgm5860xSet,                         // 1034
    sgm5860xCH1,                         // 1035
    sgm5860xCH2,                         // 1036
    sgm5860xCH3,                         // 1037
    sgm5860xCH4,                         // 1038
    ADCUpdate,                           // 1039
                  
    /************************************************* */
    SDK_EVENT_NUM,
    SDK_EVENT_LIST_MAX = SDK_EVENT_LIST_APP_START - 1
} SDK_EVENT_LIST;
#endif  // __SDK_EVENT_H__