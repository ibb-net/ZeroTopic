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

// define commands
#define SGM58601_CMD_WAKEUP   0x00
#define SGM58601_CMD_RDATA    0x01
#define SGM58601_CMD_RDATAC   0x03
#define SGM58601_CMD_SDATAC   0x0f
#define SGM58601_CMD_RREG     0x10
#define SGM58601_CMD_WREG     0x50
#define SGM58601_CMD_SELFCAL  0xf0
#define SGM58601_CMD_SELFOCAL 0xf1
#define SGM58601_CMD_SELFGCAL 0xf2
#define SGM58601_CMD_SYSOCAL  0xf3
#define SGM58601_CMD_SYSGCAL  0xf4
#define SGM58601_CMD_SYNC     0xfc
#define SGM58601_CMD_STANDBY  0xfd
#define SGM58601_CMD_REST     0xfe

// define the SGM58601 register values
#define SGM58601_STATUS 0x00
#define SGM58601_MUX    0x01
#define SGM58601_ADCON  0x02
#define SGM58601_DRATE  0x03
#define SGM58601_IO     0x04
#define SGM58601_OFC0   0x05
#define SGM58601_OFC1   0x06
#define SGM58601_OFC2   0x07
#define SGM58601_FSC0   0x08
#define SGM58601_FSC1   0x09
#define SGM58601_FSC2   0x0A

// define multiplexer codes
#define SGM58601_MUXP_AIN0   0x00
#define SGM58601_MUXP_AIN1   0x10
#define SGM58601_MUXP_AIN2   0x20
#define SGM58601_MUXP_AIN3   0x30
#define SGM58601_MUXP_AIN4   0x40
#define SGM58601_MUXP_AIN5   0x50
#define SGM58601_MUXP_AIN6   0x60
#define SGM58601_MUXP_AIN7   0x70
#define SGM58601_MUXP_AINCOM 0x80

#define SGM58601_MUXN_AIN0   0x00
#define SGM58601_MUXN_AIN1   0x01
#define SGM58601_MUXN_AIN2   0x02
#define SGM58601_MUXN_AIN3   0x03
#define SGM58601_MUXN_AIN4   0x04
#define SGM58601_MUXN_AIN5   0x05
#define SGM58601_MUXN_AIN6   0x06
#define SGM58601_MUXN_AIN7   0x07
#define SGM58601_MUXN_AINCOM 0x08

// define gain codes
#define SGM58601_GAIN_1  0x00
#define SGM58601_GAIN_2  0x01
#define SGM58601_GAIN_4  0x02
#define SGM58601_GAIN_8  0x03
#define SGM58601_GAIN_16 0x04
#define SGM58601_GAIN_32 0x05
#define SGM58601_GAIN_64 0x06
// #define SGM58601_GAIN_64     0x07

// define drate codes
#define SGM58601_DRATE_30000SPS 0xF0
#define SGM58601_DRATE_15000SPS 0xE0
#define SGM58601_DRATE_7500SPS  0xD0
#define SGM58601_DRATE_3750SPS  0xC0
#define SGM58601_DRATE_2000SPS  0xB0
#define SGM58601_DRATE_1000SPS  0xA1
#define SGM58601_DRATE_500SPS   0x92
#define SGM58601_DRATE_100SPS   0x82
#define SGM58601_DRATE_60SPS    0x72
#define SGM58601_DRATE_50SPS    0x63
#define SGM58601_DRATE_30SPS    0x53
#define SGM58601_DRATE_25SPS    0x43
#define SGM58601_DRATE_15SPS    0x33
#define SGM58601_DRATE_10SPS    0x23
#define SGM58601_DRATE_5SPS     0x13
#define SGM58601_DRATE_2_5SPS   0x03

// define commands
#define SGM58601_CMD_WAKEUP   0x00
#define SGM58601_CMD_RDATA    0x01
#define SGM58601_CMD_RDATAC   0x03
#define SGM58601_CMD_SDATAC   0x0f
#define SGM58601_CMD_RREG     0x10
#define SGM58601_CMD_WREG     0x50
#define SGM58601_CMD_SELFCAL  0xf0
#define SGM58601_CMD_SELFOCAL 0xf1
#define SGM58601_CMD_SELFGCAL 0xf2
#define SGM58601_CMD_SYSOCAL  0xf3
#define SGM58601_CMD_SYSGCAL  0xf4
#define SGM58601_CMD_SYNC     0xfc
#define SGM58601_CMD_STANDBY  0xfd
#define SGM58601_CMD_REST     0xfe

// define the SGM58601 register values
#define SGM58601_STATUS 0x00
#define SGM58601_MUX    0x01
#define SGM58601_ADCON  0x02
#define SGM58601_DRATE  0x03
#define SGM58601_IO     0x04
#define SGM58601_OFC0   0x05
#define SGM58601_OFC1   0x06
#define SGM58601_OFC2   0x07
#define SGM58601_FSC0   0x08
#define SGM58601_FSC1   0x09
#define SGM58601_FSC2   0x0A

// define multiplexer codes
#define SGM58601_MUXP_AIN0   0x00
#define SGM58601_MUXP_AIN1   0x10
#define SGM58601_MUXP_AIN2   0x20
#define SGM58601_MUXP_AIN3   0x30
#define SGM58601_MUXP_AIN4   0x40
#define SGM58601_MUXP_AIN5   0x50
#define SGM58601_MUXP_AIN6   0x60
#define SGM58601_MUXP_AIN7   0x70
#define SGM58601_MUXP_AINCOM 0x80

#define SGM58601_MUXN_AIN0   0x00
#define SGM58601_MUXN_AIN1   0x01
#define SGM58601_MUXN_AIN2   0x02
#define SGM58601_MUXN_AIN3   0x03
#define SGM58601_MUXN_AIN4   0x04
#define SGM58601_MUXN_AIN5   0x05
#define SGM58601_MUXN_AIN6   0x06
#define SGM58601_MUXN_AIN7   0x07
#define SGM58601_MUXN_AINCOM 0x08

// define gain codes
#define SGM58601_GAIN_1  0x00
#define SGM58601_GAIN_2  0x01
#define SGM58601_GAIN_4  0x02
#define SGM58601_GAIN_8  0x03
#define SGM58601_GAIN_16 0x04
#define SGM58601_GAIN_32 0x05
#define SGM58601_GAIN_64 0x06
// #define SGM58601_GAIN_64     0x07

// define drate codes
#define SGM58601_DRATE_30000SPS 0xF0
#define SGM58601_DRATE_15000SPS 0xE0
#define SGM58601_DRATE_7500SPS  0xD0
#define SGM58601_DRATE_3750SPS  0xC0
#define SGM58601_DRATE_2000SPS  0xB0
#define SGM58601_DRATE_1000SPS  0xA1
#define SGM58601_DRATE_500SPS   0x92
#define SGM58601_DRATE_100SPS   0x82
#define SGM58601_DRATE_60SPS    0x72
#define SGM58601_DRATE_50SPS    0x63
#define SGM58601_DRATE_30SPS    0x53
#define SGM58601_DRATE_25SPS    0x43
#define SGM58601_DRATE_15SPS    0x33
#define SGM58601_DRATE_10SPS    0x23
#define SGM58601_DRATE_5SPS     0x13
#define SGM58601_DRATE_2_5SPS   0x03

int DevSpiInit(const DevSpiHandleStruct *ptrDevSpiHandle);
int DevSpiDMAWrite(const DevSpiHandleStruct *ptrDevSpiHandle, uint8_t *buffer, uint32_t length);
int DevSpiWrite(const DevSpiHandleStruct *ptrDevSpiHandle, uint8_t *buffer, uint32_t length);
uint8_t DevSpiRead(const DevSpiHandleStruct *ptrDevSpiHandle);
uint8_t DevSpiWriteRead(const DevSpiHandleStruct *ptrDevSpiHandle, uint8_t *snd, uint8_t *rcv, int size);
#endif  // __DEV_SPI_H