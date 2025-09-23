#ifndef __DEV_PIN_H
#define __DEV_PIN_H
// #include "semphr.h"
#include "dev_basic.h"
#define ONEWIRE_ROM_SIZE              8
#define ONEWIRE_MAX_DEVICES           1
#define ONEWIRE_RESET_TIME            480 /*!< us */
#define ONEWIRE_RESET_START_WAIT_TIME 15  /*!< us */
#define ONEWIRE_RESET_WAIT_STEP       15  /*!< us */
#define ONEWIRE_RESET_DURATION_START  60  /*!< us */
#define ONEWIRE_RESET_DURATION_END    240 /*!< us */
#define ONEWIRE_RESET_WAIT_STEP       15  /*!< us */

#define ONEWIRE_READ_ROM_CMD 0x33
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
int DevOneWireReadRom(const DevOneWireHandleStruct *handle, uint8_t *rom_code);
void DevOneWirePinWrite(const DevOneWireHandleStruct *handle, uint8_t bit_value);

void DMA0_Channel2_IRQHandler(void);
void DMA1_Channel2_IRQHandler(void);
void onewire_usart_init(void);
void ow_uart_dma_send(uint8_t *data, uint8_t len);
void ow_uart_dma_recv(uint8_t *data, size_t len);
uint8_t onewire_reset(void);
void onewire_convert_temperature(void);
double onewire_read_temperature(void) ;
int onewire_write_byte(uint8_t *data, uint8_t len);
int onewire_read_byte(uint8_t *data, uint8_t len) ;
int onewire_rom(uint8_t *rom_code);

#endif  // __DEV_PIN_H