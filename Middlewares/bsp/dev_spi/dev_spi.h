#ifndef __DEV_SPI_H
#define __DEV_SPI_H
// #include "semphr.h"
#include "dev_basic.h"
#include "dev_dma.h"
#include "dev_pin.h"

typedef enum {
    DevSpiModeOutput = 0,  // 输出模式
    DevSpiModeInput,       // 输入模式
    DevSpiModeAF,          // 输入模式
} DevSpiModeEnum;
typedef struct
{
    uint32_t base;
    /* GPIO */
    DevPinHandleStruct nss;
    DevPinHandleStruct clk;
    DevPinHandleStruct mosi;
    DevPinHandleStruct miso;
    spi_parameter_struct spi_cfg;

    DevDMAHandleStruct dam_tx;

} DevSpiHandleStruct;

int DevSpiInit(const DevSpiHandleStruct *ptrDevSpiHandle);
int DevSpiDMAWrite(const DevSpiHandleStruct *ptrDevSpiHandle, uint8_t *buffer, uint32_t length);
int DevSpiWrite(const DevSpiHandleStruct *ptrDevSpiHandle, uint8_t *buffer, uint32_t length) ;
uint8_t DevSpiRead(const DevSpiHandleStruct *ptrDevSpiHandle);
uint8_t DevSpiWriteRead(const DevSpiHandleStruct *ptrDevSpiHandle, uint8_t *snd, uint8_t *rcv, int size) ;
#endif  // __DEV_SPI_H