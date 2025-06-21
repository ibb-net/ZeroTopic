#define CONFIG_sgm5860x_EN 1
#if CONFIG_sgm5860x_EN
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "gd32h7xx.h"
#include "string.h"

/* Device */
#include "Device.h"
#include "dev_basic.h"
#include "dev_delay.h"
#include "dev_pin.h"
#include "dev_sgm5860x.h"
#include "dev_spi.h"
#include "elog.h"
#include "os_server.h"
// #include "app_event.h"

#define TAG            "dev_sgm5860x"
#define sgm5860xLogLvl ELOG_LVL_INFO

#define sgm5860xPriority PriorityNormalEventGroup0
#ifndef sgm5860xChannelMax
#define sgm5860xChannelMax 4
#endif
#endif

/* =====================================================================================
 */
#define SET_RESET1_HIGH
#define SET_RESET1_LOW

#define SET_CS1_HIGH
#define SET_CS1_LOW

#define SGM5860_WAIT_TIME_US 1000
/*SGM58601初始化*/
static int __DevSgm5860xWaitforDRDY(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle);

int DevSgm5860xInit(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle) {
    if (ptrDevSgm5860xHandle == NULL) {
        elog_e(TAG, "DevSgm5860xInit: ptrDevSgm5860xHandle is NULL");
        return 0;
    }

    // Initialize pins
    DevPinInit(&ptrDevSgm5860xHandle->drdy);
    DevPinInit(&ptrDevSgm5860xHandle->nest);
    DevPinInit(&ptrDevSgm5860xHandle->sync);
    DevSpiInit(&ptrDevSgm5860xHandle->spi);
    DevPinWrite(&ptrDevSgm5860xHandle->nest, 1);
    DevPinWrite(&ptrDevSgm5860xHandle->sync, 1);
    DevPinWrite(&ptrDevSgm5860xHandle->spi.nss, 1);
    DevSgm5860xReset(ptrDevSgm5860xHandle);
    return 1;
}

static int __DevSgm5860xWaitforDRDY(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle) {
    if (ptrDevSgm5860xHandle == NULL) {
        elog_e(TAG, "__DevSgm5860xWaitForDRDY: ptrDevSgm5860xHandle is NULL");
        return 0;
    }
    uint32_t timeout = SGM5860_WAIT_TIME_US;  // Timeout in microseconds
    // Wait for DRDY pin to go low
    while (DevPinRead(&ptrDevSgm5860xHandle->drdy)) {
        // DevDelayUs(1);  // Wait for DRDY to go low
        vTaskDelay(pdMS_TO_TICKS(1));  // Wait for 1 millisecond
        timeout--;
        if (timeout == 0) {
            elog_e(TAG, "__DevSgm5860xWaitForDRDY: Timeout waiting for DRDY");
           return -1;  // Timeout occurred
        }
    }
    return 1;
}

void DevSgm5860xReset(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle) {
    if (ptrDevSgm5860xHandle == NULL) {
        elog_e(TAG, "DevSgm5860xReset: ptrDevSgm5860xHandle is NULL");
        return;
    }
    elog_i(TAG, "DevSgm5860xReset: Device reset Started");

    DevPinWrite(&ptrDevSgm5860xHandle->nest, 0);  // Set NEST pin low
    DevDelayUs(100);  // Wait for 100 microseconds
    DevPinWrite(&ptrDevSgm5860xHandle->nest, 1);  // Set NEST pin high
    while (DevPinRead(&ptrDevSgm5860xHandle->drdy)==0) ;
    elog_i(TAG, "DevSgm5860xReset: Device reset completed");
}

int DevSgm5860xWriteReg(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle, uint8_t regaddr,
                        uint8_t *regvalue, uint8_t reglen) {
    if (ptrDevSgm5860xHandle == NULL || regvalue == NULL) {
        elog_e(TAG, "DevSgm5860xWriteReg: ptrDevSgm5860xHandle or regvalue is NULL");
        return -1;
    }
    if (reglen < 1 || reglen > 0x0b) {
        elog_e(TAG, "DevSgm5860xWriteReg: Invalid reglen %d", reglen);
        return -1;  // Invalid register length
    }
    if (regaddr > 0x0F) {
        elog_e(TAG, "DevSgm5860xWriteReg: Invalid regaddr 0x%02X", regaddr);
        return -1;  // Invalid register address
    }
    uint8_t snd_data[16] = {0};
    uint8_t rcv_data[16] = {0};
    memset(snd_data, 0, sizeof(snd_data));
    memset(rcv_data, 0, sizeof(rcv_data));
    snd_data[0] = SGM58601_CMD_WREG | (regaddr & 0x0F);  // Command to write register
    snd_data[1] = reglen - 1;                            // Number of bytes to
    memcpy(&snd_data[2], regvalue, reglen);              // Write data to register
    DevPinWrite(&ptrDevSgm5860xHandle->spi.nss, 0);      // Set NEST pin low
    if (__DevSgm5860xWaitforDRDY(ptrDevSgm5860xHandle) < 0) {
        elog_e(TAG, "DevSgm5860xWriteReg: Wait for DRDY failed");
        return -1;
    }
    DevSpiWriteRead(&ptrDevSgm5860xHandle->spi, snd_data, rcv_data, reglen + 2);
    DevPinWrite(&ptrDevSgm5860xHandle->spi.nss, 1);  // Set NEST pin high
    // elog_d(TAG, "DevSgm5860xWriteReg regaddr: 0x%02X reglen: %d", regaddr, reglen);
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), "Write regaddr: 0x%02X, reglen: %d, data: ", regaddr,
             reglen);
    for (size_t i = 0; i < reglen; i++) {
        snprintf(log_msg + strlen(log_msg), sizeof(log_msg) - strlen(log_msg), "0x%02X ",
                 snd_data[i + 2]);
    }
    elog_i(TAG, "%s", log_msg);
    return 1;  // Success
}
int DevSgm5860xReadReg(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle, uint8_t regaddr,
                       uint8_t *regvalue, uint8_t reglen) {
    if (ptrDevSgm5860xHandle == NULL || regvalue == NULL) {
        elog_e(TAG, "DevSgm5860xReadReg: ptrDevSgm5860xHandle or regvalue is NULL");
        return -1;
    }
    if (reglen < 1 || reglen > 0x0b) {
        elog_e(TAG, "DevSgm5860xReadReg: Invalid reglen %d", reglen);
        return -1;  // Invalid register length
    }
    if (regaddr > 0x0F) {
        elog_e(TAG, "DevSgm5860xWriteReg: Invalid regaddr 0x%02X", regaddr);
        return -1;  // Invalid register address
    }
    uint8_t snd_data[16] = {0};
    uint8_t rcv_data[16] = {0};
    memset(snd_data, 0, sizeof(snd_data));
    memset(rcv_data, 0, sizeof(rcv_data));
    snd_data[0] = SGM58601_CMD_RREG | (regaddr & 0x0F);  // Command to read register
    snd_data[1] = reglen - 1;                            // Number of bytes to read
    DevPinWrite(&ptrDevSgm5860xHandle->spi.nss, 0);      // Set NEST pin low
    if (__DevSgm5860xWaitforDRDY(ptrDevSgm5860xHandle) < 0) {
        elog_e(TAG, "DevSgm5860xReadReg: Wait for DRDY failed");
        return -1;
    }
    DevSpiWriteRead(&ptrDevSgm5860xHandle->spi, snd_data, rcv_data, reglen + 2);
    DevPinWrite(&ptrDevSgm5860xHandle->spi.nss, 1);  // Set NEST pin high
    memcpy(regvalue, &rcv_data[2], reglen);          // Copy read data to output buffer
    // elog_d(TAG, "DevSgm5860xReadReg regaddr: 0x%02X reglen: %d", regaddr, reglen);
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), "Read regaddr: 0x%02X, reglen: %d, data: ", regaddr, reglen);

    for (size_t i = 0; i < reglen; i++) {
        snprintf(log_msg + strlen(log_msg), sizeof(log_msg) - strlen(log_msg), "0x%02X ",
                 rcv_data[i + 2]);
    }
    elog_i(TAG, "%s", log_msg);
    return 1;  // Success
}
int DevSgm5860xConfig(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle) {
    SGM5860xStatusReg_t status_reg = {
        .bits.nDRDY = 0,    // Data Ready (Active Low)
        .bits.BUFEN = 1,    // Buffer Enable
        .bits.ACAL  = 1,    // Auto Calibration Enable
        .bits.ORDER = 0,    // Data Order (0: MSB First, 1: LSB First)
        .bits.ID    = 0x00  // Device ID
    };
    SGM5860xMuxReg_t mux_reg = {
        .bits.NSEL = SGM58601_MUXP_AIN0,   // Negative Input Channel Selection
        .bits.PSEL = SGM58601_MUXN_AINCOM  // Positive Input Channel Selection
    };

    SGM5860xAdconReg_t adcon_reg = {
        .bits.CLK      = 0,                // Clock Output Frequency
        .bits.SDCS     = 0,                // Sensor Detection Current Source
        .bits.PGA      = SGM58601_GAIN_1,  // Programmable Gain Amplifier
        .bits.reserved = 0                 // Reserved
    };
    SGM5860xDrateReg_t drate_reg = {
        .bits.DR = SGM58601_DRATE_100SPS  // Data Rate
    };

    SGM5860xIoReg_t io_reg = {
        .bits.DIO = 0,  // Digital Input/Output State
        .bits.DIR = 0   // Digital Input/Output Direction
    };
    elog_i(TAG, "DevSgm5860xConfig: Configuring SGM58601 registers");
    elog_i(TAG, "Config Status Register: 0x%02X", status_reg.raw);
    DevSgm5860xWriteReg(ptrDevSgm5860xHandle, SGM58601_STATUS, (uint8_t *)&status_reg,
                        sizeof(status_reg));
    DevSgm5860xReadReg(ptrDevSgm5860xHandle, SGM58601_STATUS, (uint8_t *)&status_reg,
                       sizeof(status_reg));
    elog_i(TAG, "Read Status Register: 0x%02X", status_reg.raw);

    elog_i(TAG, "Config MUX Register: 0x%02X", mux_reg.raw);
    DevSgm5860xWriteReg(ptrDevSgm5860xHandle, SGM58601_MUX, (uint8_t *)&mux_reg, sizeof(mux_reg));
    DevSgm5860xReadReg(ptrDevSgm5860xHandle, SGM58601_MUX, (uint8_t *)&mux_reg, sizeof(mux_reg));
    elog_i(TAG, "Read MUX Register: 0x%02X", mux_reg.raw);
    elog_i(TAG, "Config ADCON Register: 0x%02X", adcon_reg.raw);
    DevSgm5860xWriteReg(ptrDevSgm5860xHandle, SGM58601_ADCON, (uint8_t *)&adcon_reg,
                        sizeof(adcon_reg));
    DevSgm5860xReadReg(ptrDevSgm5860xHandle, SGM58601_ADCON, (uint8_t *)&adcon_reg,
                       sizeof(adcon_reg));
    elog_i(TAG, "Read ADCON Register: 0x%02X", adcon_reg.raw);
    elog_i(TAG, "Config DRATE Register: 0x%02X", drate_reg.raw);
    DevSgm5860xWriteReg(ptrDevSgm5860xHandle, SGM58601_DRATE, (uint8_t *)&drate_reg,
                        sizeof(drate_reg));
    DevSgm5860xReadReg(ptrDevSgm5860xHandle, SGM58601_DRATE, (uint8_t *)&drate_reg,
                       sizeof(drate_reg));
    elog_i(TAG, "Read DRATE Register: 0x%02X", drate_reg.raw);
}
