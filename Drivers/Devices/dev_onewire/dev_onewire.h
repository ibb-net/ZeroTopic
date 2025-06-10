#ifndef __DEV_PIN_H
#define __DEV_PIN_H
// #include "semphr.h"
#include "dev_basic.h"

typedef enum {
    DevPinModeOutput = 0,  // 输出模式
    DevPinModeInput,       // 输入模式
    DevPinModeAF,          // 输入模式
} DevOneWireModeEnum;
typedef struct
{
    char device_name[DEVICE_NAME_MAX];
    EnumKeyPinMapID dev_pin_id;

} DevOneWireHandleStruct;

void DevOneWireInit(DevOneWireHandleStruct *handle);
int DevOneWireReset(const DevOneWireHandleStruct *handle);
int DevOneWireStop(const DevOneWireHandleStruct *handle);
void DevOneWireWriteByte(const DevOneWireHandleStruct *handle, uint8_t byte_value);
uint8_t DevOneWireReadByte(const DevOneWireHandleStruct *handle);
int DevOneWireReadRom(const DevOneWireHandleStruct *handle, uint8_t *rom_code) ;
void DevOneWirePinWrite(const DevOneWireHandleStruct *handle, uint8_t bit_value) ;

#endif  // __DEV_PIN_H