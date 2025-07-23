#ifndef __DEV_PIN_H
#define __DEV_PIN_H
// #include "semphr.h"
#include "dev_basic.h"

typedef enum {
    DevPinModeOutput = 0,  // 输出模式
    DevPinModeInput,       // 输入模式
    DevPinModeAF,          // 输入模式
} DevPinModeEnum;
typedef struct
{
    char device_name[DEVICE_NAME_MAX];
    uint32_t base;
    uint32_t af;
    uint32_t pin;
    DevPinModeEnum pin_mode;  // 0: output, 1: input
    uint8_t bit_value;        // 0: low, 1: high


} DevPinHandleStruct;

void DevPinInit(const DevPinHandleStruct *ptrDevPinHandle);
void DevPinWrite(const DevPinHandleStruct *ptrDevPinHandle, uint8_t bit_value);
uint8_t DevPinRead(const DevPinHandleStruct *ptrDevPinHandle);
void DevErrorLED(uint8_t is_on) ;
void DevErrorLEDToggle() ;
void DevErrorLED1Toggle();
void DevPinSetIsrCallback(const DevPinHandleStruct *ptrDevPinHandle,
                                 void* callback);
extern const TypedefDevPinMap DevPinMap[GD32H7XXZ_PIN_MAP_MAX];
extern void EXTI0_IRQHandler(void);
extern void EXTI9_IRQHandler(void);
#endif  // __DEV_PIN_H