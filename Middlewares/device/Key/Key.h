#ifndef __KEY_H__
#define __KEY_H__
#include <stdio.h>

#include "dev_basic.h"
#include "dev_pin.h"
typedef enum {
    KEY_TRIGGER_MODE_NONE = 0,  // 无触发模式
    KEY_TRIGGER_MODE_PRESS,   // 按键模式
    KEY_TRIGGER_MODE_TOGGLE,  // 自锁模式

} EnumKeyTirgerMode;
typedef enum {
    KEY_INACTIVE,
    KEY_ACTIVE,  


} EnumKeyActiveState;
typedef struct
{
    char device_name[DEVICE_NAME_MAX];
    DevPinHandleStruct pin;          // GPIO 配置
    EnumKeyTirgerMode trigger_mode;  // 按键触发模式
    uint8_t active_level;            // Active level, 0 for low, 1 for high
    uint32_t short_press_event;      // Short press event
    uint32_t long_press_event;       // Long press event
    uint32_t toggle_event;           // Toggle event

} TypdefKeyBSPCfg;

typedef struct
{
    uint8_t enable;  // Enable flag
    TypdefKeyBSPCfg *cfg;
    uint8_t state;//用于按键模式
    uint8_t last_pin_value;  // Last pin value ,用于自锁模式,位置为0xFF表示初始化
    uint32_t press_time;
    uint8_t long_press_triggered;
} TypdefKeyStatus;
#endif  // __KEY_H__