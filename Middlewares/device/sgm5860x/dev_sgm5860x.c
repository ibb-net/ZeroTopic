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
// #ifndef sgm5860xChannelMax
// #define sgm5860xChannelMax 8
// #endif
#endif

/* =====================================================================================
 */
#define VREF_VOLTAGE         (5.0)      // Reference voltage for ADC 2*2.5V
#define PGA_GAIN             1.0        // Gain for PGA (Programmable Gain Amplifier)
#define ADC_24BIT_MAX_INT    8388608    // 2^23 整数版本
#define ADC_24BIT_MAX_DOUBLE 8388608.0  // 浮点版本，仅用于最终计算
#define SGM5860_WAIT_TIME_US 1000

const double gaim_map[SGM58601_GAIN_MAX] = {1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0, 128.0};

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
    DevPinIsStartISR(&ptrDevSgm5860xHandle->drdy, 0);
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
    uint32_t timeout = SGM5860_WAIT_TIME_US;  // SGM5860_WAIT_TIME_US;
    while (DevPinRead(&ptrDevSgm5860xHandle->drdy)) {
        // DevDelayUs(1);  // Wait for DRDY to go low
        vTaskDelay(pdMS_TO_TICKS(1));  // Wait for 1 millisecond
        timeout--;
        if (timeout == 0) {
            // elog_e(TAG, "__DevSgm5860xWaitForDRDY: Timeout waiting for DRDY");
            return -1;
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
    uint32_t timeout = SGM5860_WAIT_TIME_US;
    DevPinWrite(&ptrDevSgm5860xHandle->nest, 0);  // Set NEST pin low
    DevDelayUs(100);                              // Wait for 100 microseconds
    DevPinWrite(&ptrDevSgm5860xHandle->nest, 1);  // Set NEST pin high
    while (DevPinRead(&ptrDevSgm5860xHandle->drdy) == 0) {
        DevDelayUs(1);
        timeout--;
        if (timeout == 0) {
            break;
        }
    }
    if (timeout == 0) {
        elog_e(TAG, "DevSgm5860xReset: Timeout waiting for DRDY after reset");
    } else {
        elog_i(TAG, "DevSgm5860xReset: Device reset completed");
    }
}
int DevSgm5860xCMDReg(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle, uint8_t regaddr) {
    uint8_t snd_data[16] = {0};
    uint8_t rcv_data[16] = {0};
    memset(snd_data, 0, sizeof(snd_data));
    memset(rcv_data, 0, sizeof(rcv_data));
    snd_data[0] = (regaddr & 0x0F);                  // Command to write register
    DevPinWrite(&ptrDevSgm5860xHandle->spi.nss, 0);  // Set NEST pin low
    if (__DevSgm5860xWaitforDRDY(ptrDevSgm5860xHandle) < 0) {
        elog_e(TAG, "DevSgm5860xCMDReg: Wait for DRDY failed");
        DevPinWrite(&ptrDevSgm5860xHandle->spi.nss, 1);
        return -1;
    }
    DevSpiWriteRead(&ptrDevSgm5860xHandle->spi, snd_data, rcv_data, 1);
    DevPinWrite(&ptrDevSgm5860xHandle->spi.nss, 1);  // Set NEST pin high
    // elog_d(TAG, "DevSgm5860xCMDReg regaddr: 0x%02X reglen: %d", regaddr, reglen);
    // char log_msg[128];
    // snprintf(log_msg, sizeof(log_msg),
    //          "DevSgm5860xCMDReg regaddr: 0x%02X, reglen: %d, data: ", regaddr, reglen);
    // for (size_t i = 0; i < reglen; i++) {
    //     snprintf(log_msg + strlen(log_msg), sizeof(log_msg) - strlen(log_msg), "0x%02X ",
    //              snd_data[i + 2]);
    // }
    // elog_i(TAG, "%s", log_msg);
    return 1;  // Success
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
    snd_data[0] = SGM58601_CMD_WREG | (regaddr & 0x0F);
    snd_data[1] = reglen - 1;
    memcpy(&snd_data[2], regvalue, reglen);
    DevPinWrite(&ptrDevSgm5860xHandle->spi.nss, 0);
    if (__DevSgm5860xWaitforDRDY(ptrDevSgm5860xHandle) < 0) {
        elog_e(TAG, "DevSgm5860xWriteReg: Wait for DRDY failed");
        DevPinWrite(&ptrDevSgm5860xHandle->spi.nss, 1);
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
    elog_d(TAG, "%s", log_msg);
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
        DevPinWrite(&ptrDevSgm5860xHandle->spi.nss, 1);
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
    elog_d(TAG, "%s", log_msg);
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
    SGM5860xStatusReg_t read_status_reg;
    SGM5860xMuxReg_t mux_reg = {

        .bits.PSEL = SGM58601_MUXP_AIN6,    // Positive Input Channel Selection
        .bits.NSEL = SGM58601_MUXN_AINCOM,  // Negative Input Channel Selection
    };
    SGM5860xMuxReg_t read_mux_reg;
    SGM5860xAdconReg_t adcon_reg = {
        .bits.CLK      = 0,                 // Clock Output Frequency
        .bits.SDCS     = 0,                 // Sensor Detection Current Source
        .bits.PGA      = SGM58601_GAIN_64,  // Programmable Gain Amplifier
        .bits.reserved = 0                  // Reserved
    };
    SGM5860xAdconReg_t read_adcon_reg;
    SGM5860xDrateReg_t drate_reg = {
        .bits.DR = SGM58601_DRATE_100SPS  // Data Rate
    };
    SGM5860xDrateReg_t read_drate_reg;

    /*     SGM5860xIoReg_t io_reg = {
            .bits.DIO = 0,  // Digital Input/Output State
            .bits.DIR = 0   // Digital Input/Output Direction
        }; */
    elog_i(TAG, "DevSgm5860xConfig: Configuring SGM58601 registers");
    elog_d(TAG, "Config Status Register: 0x%02X", status_reg.raw);
    DevSgm5860xWriteReg(ptrDevSgm5860xHandle, SGM58601_STATUS, (uint8_t *)&status_reg,
                        sizeof(status_reg));

    DevSgm5860xReadReg(ptrDevSgm5860xHandle, SGM58601_STATUS, (uint8_t *)&read_status_reg,
                       sizeof(status_reg));
    elog_d(TAG, "Read Status Register: 0x%02X", status_reg.raw);
    if (read_status_reg.raw != status_reg.raw) {
        elog_e(TAG, "Read Status Register: 0x%02X Read 0x%02X", status_reg.raw,
               read_status_reg.raw);
    }
    elog_d(TAG, "Config MUX Register: 0x%02X", mux_reg.raw);
    DevSgm5860xWriteReg(ptrDevSgm5860xHandle, SGM58601_MUX, (uint8_t *)&mux_reg, sizeof(mux_reg));
    DevSgm5860xReadReg(ptrDevSgm5860xHandle, SGM58601_MUX, (uint8_t *)&read_mux_reg,
                       sizeof(mux_reg));
    if (mux_reg.raw != read_mux_reg.raw) {
        elog_e(TAG, "Read SGM58601_MUX Register: 0x%02X Read 0x%02X", read_mux_reg.raw,
               read_mux_reg.raw);
    }
    elog_d(TAG, "Read MUX Register: 0x%02X", mux_reg.raw);
    elog_d(TAG, "Config ADCON Register: 0x%02X", adcon_reg.raw);
    DevSgm5860xWriteReg(ptrDevSgm5860xHandle, SGM58601_ADCON, (uint8_t *)&adcon_reg,
                        sizeof(adcon_reg));
    DevSgm5860xReadReg(ptrDevSgm5860xHandle, SGM58601_ADCON, (uint8_t *)&read_adcon_reg,
                       sizeof(adcon_reg));
    if (adcon_reg.raw != read_adcon_reg.raw) {
        elog_e(TAG, "Read SGM58601_ADCON Register: 0x%02X Read 0x%02X", adcon_reg.raw,
               read_adcon_reg.raw);
    }
    elog_d(TAG, "Read ADCON Register: 0x%02X", adcon_reg.raw);
    elog_d(TAG, "Config DRATE Register: 0x%02X", drate_reg.raw);
    DevSgm5860xWriteReg(ptrDevSgm5860xHandle, SGM58601_DRATE, (uint8_t *)&drate_reg,
                        sizeof(drate_reg));
    DevSgm5860xReadReg(ptrDevSgm5860xHandle, SGM58601_DRATE, (uint8_t *)&read_drate_reg,
                       sizeof(drate_reg));
    if (read_drate_reg.raw != drate_reg.raw) {
        elog_e(TAG, "Read SGM58601_ADCON Register: 0x%02X Read 0x%02X", drate_reg.raw,
               read_drate_reg.raw);
    }
    elog_i(TAG, "SGM5860 Config Done");

    return 1;
}
void DevSgm5860xStart(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle) {
    if (ptrDevSgm5860xHandle == NULL) {
        elog_e(TAG, "DevSgm5860xStart: ptrDevSgm5860xHandle is NULL");
        return;
    }
    DevPinIsStartISR(&ptrDevSgm5860xHandle->drdy, 1);
}
void DevSgm5860xStop(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle) {
    if (ptrDevSgm5860xHandle == NULL) {
        elog_e(TAG, "DevSgm5860xStop: ptrDevSgm5860xHandle is NULL");
        return;
    }
    DevPinIsStartISR(&ptrDevSgm5860xHandle->drdy, 0);
}
int DevGetADCData(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle, double *last_voltage,
                  uint8_t *last_channel, uint8_t channel, uint8_t gain) {
    if (ptrDevSgm5860xHandle == NULL || last_voltage == NULL) {
        elog_e(TAG, "DevGetADCData: ptrDevSgm5860xHandle or last_voltage is NULL");
        return -1;
    }
    static uint32_t cyc  = 0;
    uint8_t snd_data[16] = {0};
    uint8_t rcv_data[16] = {0};
    memset(snd_data, 0, sizeof(snd_data));
    memset(rcv_data, 0, sizeof(rcv_data));
    // uint8_t channel              = 0;
    SGM5860xMuxReg_t mux_reg     = {0};
    SGM5860xAdconReg_t adcon_reg = {
        .bits.CLK      = 0,                // Clock Output Frequency
        .bits.SDCS     = 0,                // Sensor Detection Current Source
        .bits.PGA      = SGM58601_GAIN_1,  // Programmable Gain Amplifier
        .bits.reserved = 0                 // Reserved
    };
    if (channel < 0 || channel > SGM58601_MUXP_AIN7) {
        elog_e(TAG, "DevGetADCData: Invalid channel %d", channel);
        return -1;
    }
    if (gain >= SGM58601_GAIN_MAX) {
        elog_e(TAG, "DevGetADCData: Invalid gain %d", gain);
        return -1;
    }
    mux_reg.bits.NSEL  = SGM58601_MUXN_AINCOM;
    mux_reg.bits.PSEL  = channel;
    adcon_reg.bits.PGA = gain;
    if (adcon_reg.bits.PGA > 1) {
        elog_d(TAG, "DevGetADCData[%u]:Set channel %d PGA gain %d ,mux raw 0x%02x ", cyc, channel,
               adcon_reg.bits.PGA, mux_reg.raw);
    }

    SGM5860xMuxReg_t tmp_mux_reg;
    SGM5860xAdconReg_t tmp_adcon_reg;
    // DevSgm5860xReadReg(ptrDevSgm5860xHandle, SGM58601_MUX, (uint8_t *)&tmp_mux_reg,
    //                    sizeof(tmp_mux_reg));  // Read MUX register

    // DevSgm5860xReadReg(ptrDevSgm5860xHandle, SGM58601_ADCON, (uint8_t *)&tmp_adcon_reg,
    //                    sizeof(tmp_adcon_reg));  // Read ADCON register
    uint8_t tmp_channel = tmp_mux_reg.bits.PSEL & 0x0F;
    double tmp_gain     = gaim_map[tmp_adcon_reg.bits.PGA];  // Get the gain from ADCON register
    if (tmp_gain > 1) {
        elog_d(TAG, "Last MUX Register: Channel %d Read Gain %.2f", tmp_channel, tmp_gain);
    }

    DevSgm5860xWriteReg(ptrDevSgm5860xHandle, SGM58601_MUX, (uint8_t *)&mux_reg,
                        sizeof(mux_reg));  // Write MUX register
    // DevSgm5860xWriteReg(ptrDevSgm5860xHandle, SGM58601_ADCON, (uint8_t *)&adcon_reg,
    //                     sizeof(adcon_reg));  // Write ADCON register    //
    // Write data to register
    elog_d(TAG, "DevGetADCData: MUX Register get to 0x%02X", mux_reg.raw);
    // snd_data[0] = SGM58601_CMD_SYNC;
    // DevPinWrite(&ptrDevSgm5860xHandle->spi.nss, 1);  // Set NEST pin high
    // while (DevPinRead(&ptrDevSgm5860xHandle->drdy) == 0) {
    // }
    // DevPinWrite(&ptrDevSgm5860xHandle->spi.nss, 0);  // Set NEST pin low
    // DevSpiWriteRead(&ptrDevSgm5860xHandle->spi, snd_data, rcv_data, 1);
    // DevPinWrite(&ptrDevSgm5860xHandle->spi.nss, 1);  // Set NEST pin high

    // snd_data[0] = SGM58601_CMD_WAKEUP;
    // DevPinWrite(&ptrDevSgm5860xHandle->spi.nss, 1);  // Set NEST pin high
    // while (DevPinRead(&ptrDevSgm5860xHandle->drdy) == 0) {
    // }
    // DevPinWrite(&ptrDevSgm5860xHandle->spi.nss, 0);  // Set NEST pin low
    // DevSpiWriteRead(&ptrDevSgm5860xHandle->spi, snd_data, rcv_data, 1);
    // DevPinWrite(&ptrDevSgm5860xHandle->spi.nss, 1);  // Set NEST pin high

    snd_data[0] = SGM58601_CMD_SYNC;                 // Command to read data
    snd_data[1] = SGM58601_CMD_WAKEUP;               // Command to read data
    snd_data[2] = SGM58601_CMD_RDATA;                // Command to read data
    DevPinWrite(&ptrDevSgm5860xHandle->spi.nss, 1);  // Set NEST pin high
    elog_d(TAG, "Last MUX Register: Channel %d", tmp_channel);
    while (DevPinRead(&ptrDevSgm5860xHandle->drdy) == 0) {
    }
    DevPinWrite(&ptrDevSgm5860xHandle->spi.nss, 0);  // Set NEST pin low
    DevSpiWriteRead(&ptrDevSgm5860xHandle->spi, snd_data, rcv_data, 3 + 3);
    DevPinWrite(&ptrDevSgm5860xHandle->spi.nss, 1);  // Set NEST pin high

    int hex_data =
        (rcv_data[3] << 16) | (rcv_data[4] << 8) | rcv_data[5];  // Combine the received data
    elog_d(TAG, "DevGetADCData: Received data: 0x%06X", hex_data);
    double voltage = 0.0;
    if (hex_data & 0x800000) {                                 // Check if the sign bit is set
        hex_data = ADC_24BIT_MAX_INT - (hex_data - 0x800000);  // Convert to positive value
        double hex_data_double = (double)hex_data;             // Convert to double for calculation
        voltage                = (-1) * hex_data_double * VREF_VOLTAGE / (ADC_24BIT_MAX_DOUBLE);
    } else {
        double hex_data_double = (double)hex_data;  // Convert to double for calculation
        voltage = hex_data_double * VREF_VOLTAGE / (ADC_24BIT_MAX_DOUBLE);  // Calculate voltage
    }

    // DevSgm5860xWriteReg(ptrDevSgm5860xHandle, SGM58601_MUX, (uint8_t *)&mux_reg,
    //                     sizeof(mux_reg));  // Write MUX register
    // DevSgm5860xWriteReg(ptrDevSgm5860xHandle, SGM58601_ADCON, (uint8_t *)&adcon_reg,
    //                     sizeof(adcon_reg));  // Write ADCON register    //
    elog_d(TAG, "DevGetADCData: Calculated voltage: %.10f V %.4f mv  Gain: %d", voltage,
           voltage * 1000.0, gain);
    elog_d(TAG,
           "rcv_data 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X "
           "0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",
           rcv_data[0], rcv_data[1], rcv_data[2], rcv_data[3], rcv_data[4], rcv_data[5],
           rcv_data[6], rcv_data[7], rcv_data[8], rcv_data[9], rcv_data[10], rcv_data[11],
           rcv_data[12], rcv_data[13], rcv_data[14], rcv_data[15]);

    *last_channel = tmp_mux_reg.bits.PSEL & 0x0F;  // Update last channel
    *last_channel = 6;
    *last_voltage = voltage;  // Update last voltage
    elog_d(TAG, "DevGetADCData[%u]:Get channel %d, last voltage %.10f V", cyc, *last_channel,
           *last_voltage);
    cyc++;
    return 1;  // Success
}