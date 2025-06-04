#ifndef __GPIO_H
#define __GPIO_H
#include <stdio.h>
#include "gd32h7xx.h"
// #include "semphr.h"
#include "Device.h"

typedef struct
{
    char device_name[DEVICE_NAME_MAX];
    uint32_t port;
    uint32_t af;
    uint32_t pin;

} TypdefGPIOCfg;

typedef struct
{
    TypdefGPIOCfg *TxGPIOCfg;
    TypdefGPIOCfg *RxGPIOCfg;

} TypdefGPIOStatus;
void DevPinInit(const TypdefGPIOCfg *gpio_cfg);


#endif // __GPIO_H