#ifndef __DEV_PIN_H
#define __DEV_PIN_H
// #include "semphr.h"
#include "dev_basic.h"

typedef struct
{
    char device_name[DEVICE_NAME_MAX];
    uint32_t base;
    uint32_t af;
    uint32_t pin;

} DevPinHandleStruct;


void DevPinInit(const DevPinHandleStruct * ptrDevPinHandle);



#endif // __DEV_PIN_H