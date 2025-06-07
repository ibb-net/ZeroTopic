#ifndef __KEY_H__
#define __KEY_H__
#include <stdio.h>
#include "dev_basic.h"
#include "dev_pin.h"

typedef struct
{
    char device_name[DEVICE_NAME_MAX];
    DevPinHandleStruct pin;      // GPIO 配置
    uint8_t active_level;        // Active level, 0 for low, 1 for high
    uint32_t short_press_event;  // Short press event
    uint32_t long_press_event;   // Long press event

} TypdefKeyBSPCfg;

typedef struct
{
    uint8_t enable;  // Enable flag
    TypdefKeyBSPCfg *cfg;
    uint8_t state;
    uint32_t press_time;
    uint8_t long_press_triggered;
} TypdefKeyStatus;
#endif // __KEY_H__