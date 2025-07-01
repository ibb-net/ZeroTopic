#ifndef __DEV_Sgm5860x_H
#define __DEV_Sgm5860x_H
// #include "semphr.h"
#include "dev_Sgm5860x.h"
#include "dev_basic.h"
#include "dev_dma.h"
#include "dev_pin.h"
#include "dev_spi.h"

typedef struct {
    DevPinHandleStruct drdy;
    DevPinHandleStruct nest;
    DevPinHandleStruct sync;
    DevSpiHandleStruct spi;  // Sgm5860x 配置
} DevSgm5860xHandleStruct;
typedef union {
    struct {
        uint8_t nDRDY : 1;  // Bit 0: Data Ready (Active Low)
        uint8_t BUFEN : 1;  // Bit 1: Buffer Enable
        uint8_t ACAL : 1;   // Bit 2: Auto Calibration Enable
        uint8_t ORDER : 1;  // Bit 3: Data Order (0: MSB First, 1: LSB First)
        uint8_t ID : 4;     // Bits 4-7: Device ID
    } bits;                 // 位域结构体
    uint8_t raw;            // 原始寄存器值
} SGM5860xStatusReg_t;      // 0x00 STATUS: Status Register

typedef union {
    struct {
        uint8_t NSEL : 4;  // Bits 0-3: Negative Input Channel Selection
        uint8_t PSEL : 4;  // Bits 4-7: Positive Input Channel Selection
    } bits;                // Bit-field structure
    uint8_t raw;           // Raw register value
} SGM5860xMuxReg_t;        // 0x01 MUX: Input Multiplexer Control Register

typedef union {
    struct {
        uint8_t PGA : 3;   // Bits 0-2 Programmable Gain Amplifier
        uint8_t SDCS : 2;  // Bits 3-4: Sensor Detection Current Source

        uint8_t CLK : 2;       // Bits 5-6: Clock Output Frequency
        uint8_t reserved : 1;  // Bit 7: Reserved
    } bits;                    // Bit-field structure
    uint8_t raw;               // Raw register value
} SGM5860xAdconReg_t;          // 0x02 ADCON: A/D Control Register

typedef union {
    struct {
        uint8_t DR : 8;  // Bits 0-7: Data Rate
    } bits;              // Bit-field structure
    uint8_t raw;         // Raw register value
} SGM5860xDrateReg_t;    // 0x03 DRATE: A/D Data Rate Register

typedef union {
    struct {
        uint8_t DIO : 4;  // Bits 0-3: Digital Input/Output State
        uint8_t DIR : 4;  // Bits 4-7: Digital Input/Output Direction
    } bits;               // Bit-field structure
    uint8_t raw;          // Raw register value
} SGM5860xIoReg_t;        // 0x04 IO: GPIO Control Register

typedef union {
    struct {
        uint8_t OFC : 8;  // Bits 0-7: Offset Calibration Byte 0 (Least Significant Byte)
    } bits;               // Bit-field structure
    uint8_t raw;          // Raw register value
} SGM5860xOfc0Reg_t;      // 0x05 OFC0: Offset Calibration Byte 0

typedef union {
    struct {
        uint8_t OFC : 8;  // Bits 0-7: Offset Calibration Byte 1
    } bits;               // Bit-field structure
    uint8_t raw;          // Raw register value
} SGM5860xOfc1Reg_t;      // 0x06 OFC1: Offset Calibration Byte 1

typedef union {
    struct {
        uint8_t OFC : 8;  // Bits 0-7: Offset Calibration Byte 2 (Most Significant Byte)
    } bits;               // Bit-field structure
    uint8_t raw;          // Raw register value
} SGM5860xOfc2Reg_t;      // 0x07 OFC2: Offset Calibration Byte 2
typedef union {
    struct {
        uint8_t FSC : 8;  // Bits 0-7: Full-Scale Calibration Byte 0 (Least Significant Byte)
    } bits;               // Bit-field structure
    uint8_t raw;          // Raw register value
} SGM5860xFsc0Reg_t;      // 0x08 FSC0: Full-Scale Calibration Byte 0
typedef union {
    struct {
        uint8_t FSC : 8;  // Bits 0-7: Full-Scale Calibration Byte 2 (Most Significant Byte)
    } bits;               // Bit-field structure
    uint8_t raw;          // Raw register value
} SGM5860xFsc2Reg_t;      // 0x0A FSC2: Full-Scale Calibration Byte 2

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

// define gain codes
#define SGM58601_GAIN_1   0x00
#define SGM58601_GAIN_2   0x01
#define SGM58601_GAIN_4   0x02
#define SGM58601_GAIN_8   0x03
#define SGM58601_GAIN_16  0x04
#define SGM58601_GAIN_32  0x05
#define SGM58601_GAIN_64  0x06
#define SGM58601_GAIN_128 0x07
#define SGM58601_GAIN_MAX 0x08
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
#define SGM58601_MUXP_AIN1   0x01
#define SGM58601_MUXP_AIN2   0x02
#define SGM58601_MUXP_AIN3   0x03
#define SGM58601_MUXP_AIN4   0x04
#define SGM58601_MUXP_AIN5   0x05
#define SGM58601_MUXP_AIN6   0x06
#define SGM58601_MUXP_AIN7   0x07
#define SGM58601_MUXP_AINCOM 0x08

#define SGM58601_MUXN_AIN0   0x00
#define SGM58601_MUXN_AIN1   0x01
#define SGM58601_MUXN_AIN2   0x02
#define SGM58601_MUXN_AIN3   0x03
#define SGM58601_MUXN_AIN4   0x04
#define SGM58601_MUXN_AIN5   0x05
#define SGM58601_MUXN_AIN6   0x06
#define SGM58601_MUXN_AIN7   0x07
#define SGM58601_MUXN_AINCOM 0x08

#define ORDER_MSB_FIRST 0  // Most significant bit first (default)
#define ORDER_LSB_FIRST 1  // Least significant bit first

int DevSgm5860xInit(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle);
void DevSgm5860xReset(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle);
int DevSgm5860xReadRegister(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle, uint8_t reg,
                            uint8_t *data);
int DevSgm5860xWriteRegister(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle, uint8_t reg,
                             uint8_t data);
int DevSgm5860xConfig(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle);
int DevGetADCData(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle, double *last_voltage,
                  uint8_t *last_channel, uint8_t channel, uint8_t gain);
#endif  // __DEV_Sgm5860x_H