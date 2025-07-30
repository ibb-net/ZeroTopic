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
#define SGM5860_WAIT_TIME_US (1000 * 1000 * 1000)
const double gain_map[SGM58601_GAIN_MAX] = {1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0, 128.0};
volatile int ready_trig_flg              = 0;
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
    // DevPinInit(&ptrDevSgm5860xHandle->sync);
    DevSpiInit(&ptrDevSgm5860xHandle->spi);
    // DevPinIsStartISR(&ptrDevSgm5860xHandle->drdy, 0);
    DevPinWrite(&ptrDevSgm5860xHandle->nest, 1);
    // DevPinWrite(&ptrDevSgm5860xHandle->sync, 1);
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
    vTaskDelay(pdMS_TO_TICKS(1));                 // Wait for 1 millisecond
    DevPinWrite(&ptrDevSgm5860xHandle->nest, 1);  // Set NEST pin high
}
int DevSgm5860xCommand(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle, uint8_t command) {
    uint8_t snd_data[16] = {0};
    uint8_t rcv_data[16] = {0};
    memset(snd_data, 0, sizeof(snd_data));
    memset(rcv_data, 0, sizeof(rcv_data));
    snd_data[0] = (command & 0x0F);  // Command to write register
    DevSpiWriteRead(&ptrDevSgm5860xHandle->spi, snd_data, rcv_data, 1);
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
    DevSpiWriteRead(&ptrDevSgm5860xHandle->spi, snd_data, rcv_data, reglen + 2);
    memcpy(regvalue, &rcv_data[2], reglen);  // Copy read data to output buffer
#if 0
    // elog_d(TAG, "DevSgm5860xReadReg regaddr: 0x%02X reglen: %d", regaddr, reglen);
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), "Read regaddr: 0x%02X, reglen: %d, data: ", regaddr, reglen);

    for (size_t i = 0; i < reglen; i++) {
        snprintf(log_msg + strlen(log_msg), sizeof(log_msg) - strlen(log_msg), "0x%02X ",
                 rcv_data[i + 2]);
    }
    elog_d(TAG, "%s", log_msg);
#endif
    return 1;  // Success
}
double DevSgm5860xReadValue(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle) {
    if (ptrDevSgm5860xHandle == NULL) {
        printf("[ERROR]DevSgm5860xReadValue: ptrDevSgm5860xHandle is NULL");
        return F_INVAILD;
    }
    double voltage       = 0.0;
    uint8_t snd_data[16] = {0};
    uint8_t rcv_data[16] = {0};
    memset(snd_data, 0, sizeof(snd_data));
    memset(rcv_data, 0, sizeof(rcv_data));
    DevSpiWriteRead(&ptrDevSgm5860xHandle->spi, snd_data, rcv_data, 3);
    int hex_data =
        (rcv_data[0] << 16) | (rcv_data[1] << 8) | rcv_data[2];  // Combine the received data
    elog_d(TAG, "DevGetADCData: Received data: 0x%06X", hex_data);
    if (hex_data & 0x800000) {                                 // Check if the sign bit is set
        hex_data = ADC_24BIT_MAX_INT - (hex_data - 0x800000);  // Convert to positive value
        double hex_data_double = (double)hex_data;             // Convert to double for calculation
        voltage                = (-1) * hex_data_double * VREF_VOLTAGE / (ADC_24BIT_MAX_DOUBLE);
    } else {
        double hex_data_double = (double)hex_data;  // Convert to double for calculation
        voltage = hex_data_double * VREF_VOLTAGE / (ADC_24BIT_MAX_DOUBLE);  // Calculate voltage
    }
    return voltage;  // Success
}

int DevSgm5860xConfig(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle) {
    SGM5860xStatusReg_t status_reg = {
        .bits.nDRDY = 0,    // Data Ready (Active Low)
        .bits.BUFEN = 1,    // Buffer Enable
        .bits.ACAL  = 0,    // Auto Calibration Enable
        .bits.ORDER = 0,    // Data Order (0: MSB First, 1: LSB First)
        .bits.ID    = 0x00  // Device ID
    };
    SGM5860xStatusReg_t read_status_reg;
    SGM5860xDrateReg_t drate_reg;
    drate_reg.bits.DR = ptrDevSgm5860xHandle->rate;
    SGM5860xDrateReg_t read_drate_reg;

    DevSgm5860xWriteReg(ptrDevSgm5860xHandle, SGM58601_STATUS, (uint8_t *)&status_reg,
                        sizeof(status_reg));
    DevSgm5860xReadReg(ptrDevSgm5860xHandle, SGM58601_STATUS, (uint8_t *)&read_status_reg,
                       sizeof(status_reg));
    if ((read_status_reg.raw | 0x01) != (status_reg.raw | 0x01)) {
        printf("[ERROR] Status Register: 0x%02X Read 0x%02X", status_reg.raw, read_status_reg.raw);
        return 0;  // Error in reading status register
    }
    DevSgm5860xWriteReg(ptrDevSgm5860xHandle, SGM58601_DRATE, (uint8_t *)&drate_reg,
                        sizeof(drate_reg));
    DevSgm5860xReadReg(ptrDevSgm5860xHandle, SGM58601_DRATE, (uint8_t *)&read_drate_reg,
                       sizeof(drate_reg));
    if (read_drate_reg.raw != drate_reg.raw) {
        printf("[ERROR] Read SGM58601_DRATE Register: 0x%02X Read 0x%02X", drate_reg.raw,
               read_drate_reg.raw);
        return 0;
    }
    printf("[PASS] SGM5860 Config Done");

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

// 需要再中断回调中调用,不能使用elog和阻塞性的函数
uint8_t DevSgm5860xSet(const DevSgm5860xHandleStruct *ptrDevPinHandle,
                       DevSgm5860xSetStruct *ptrSet) {
    if (ptrDevPinHandle == NULL) {
        printf("[ERROR]DevSgm5860xISRSetCallback: ptrDevPinHandle is NULL\r\n");
        return 0;
    }
    if (ptrSet == NULL) {
        printf("[ERROR]DevSgm5860xISRSetCallback: ptrSet is NULL\r\n");
        return 0;
    }
    uint8_t channel = ptrSet->channel;
    uint8_t gain    = ptrSet->gain;
    uint8_t sps     = ptrSet->sps;

    static uint8_t channel_last = -1;
    static uint8_t gain_last    = -1;
    static uint8_t sps_last     = 0xFF;

    SGM5860xMuxReg_t mux_reg                      = {0};
    mux_reg.raw                                   = 0;
    SGM5860xAdconReg_t adcon_reg                  = {0};
    adcon_reg.raw                                 = 0;
    SGM5860xMuxReg_t read_mux_reg                 = {0};
    read_mux_reg.raw                              = 0;
    SGM5860xAdconReg_t read_adcon_reg             = {0};
    read_adcon_reg.raw                            = 0;
    DevSgm5860xHandleStruct *ptrDevSgm5860xHandle = (DevSgm5860xHandleStruct *)ptrDevPinHandle;
    DevSgm5860xCommand(ptrDevSgm5860xHandle, SGM58601_CMD_SDATAC);
    if (channel_last != channel) {
        mux_reg.bits.NSEL = SGM58601_MUXN_AINCOM;
        mux_reg.bits.PSEL = (uint8_t)(channel & 0xFF);
        DevSgm5860xWriteReg(ptrDevSgm5860xHandle, SGM58601_MUX, (uint8_t *)&mux_reg,
                            sizeof(mux_reg));
        DevSgm5860xReadReg(ptrDevSgm5860xHandle, SGM58601_MUX, (uint8_t *)&read_mux_reg,
                           sizeof(mux_reg));
        if (mux_reg.raw != read_mux_reg.raw) {
            printf("[ERROR]MUX: 0x%08X, Expected: 0x%08X\r\n", read_mux_reg.raw, mux_reg.raw);
            return 0;
        }
        channel_last = channel;
    }
    if (gain_last != gain) {
        adcon_reg.bits.PGA  = (uint8_t)(gain & 0x07);  // Gain code
        adcon_reg.bits.SDCS = 0;                       // Sensor Detection Current Source
        adcon_reg.bits.CLK  = 0;                       // Clock Output Frequency
        DevSgm5860xWriteReg(ptrDevSgm5860xHandle, SGM58601_ADCON, (uint8_t *)&adcon_reg,
                            sizeof(adcon_reg));

        DevSgm5860xReadReg(ptrDevSgm5860xHandle, SGM58601_ADCON, (uint8_t *)&read_adcon_reg,
                           sizeof(adcon_reg));

        if (adcon_reg.raw != read_adcon_reg.raw) {
            printf("[ERROR]ADCON: 0x%08X, Expected: 0x%08X\r\n", read_adcon_reg.raw, adcon_reg.raw);
            return 0;
        }
        gain_last = gain;
    }
    if (sps_last != sps) {
        SGM5860xDrateReg_t drate_reg = {0};
        drate_reg.bits.DR            = sps;  // Set the data rate
        DevSgm5860xWriteReg(ptrDevSgm5860xHandle, SGM58601_DRATE, (uint8_t *)&drate_reg,
                            sizeof(drate_reg));
        SGM5860xDrateReg_t read_drate_reg = {0};
        DevSgm5860xReadReg(ptrDevSgm5860xHandle, SGM58601_DRATE, (uint8_t *)&read_drate_reg,
                           sizeof(drate_reg));
        if (drate_reg.raw != read_drate_reg.raw) {
            printf("[ERROR]DRATE: 0x%08X, Expected: 0x%08X\r\n", read_drate_reg.raw, drate_reg.raw);
            return 0;
        } else {
            // printf("[PASS]DRATE: 0x%08X\r\n", read_drate_reg.raw);
        }
        sps_last = sps;
    }

    // printf("[PASS]DevSgm5860xSet: Channel %d Gain %d MUX 0x%02X ADCON 0x%02X\r\n", channel, gain,
    //        mux_reg.raw, adcon_reg.raw);
    return 1;
}
uint8_t DevSgm5860xStartContinuousMode(const DevSgm5860xHandleStruct *ptrDevPinHandle) {
    DevSgm5860xCommand(ptrDevPinHandle, SGM58601_CMD_WAKEUP);
    DevSgm5860xCommand(ptrDevPinHandle, SGM58601_CMD_RDATAC);
    return 1;  // Success
}

void DevSgm5860xISRSetCallback(const DevSgm5860xHandleStruct *ptrDevPinHandle,
                               DevSgm5860xSetStruct *ptrCurr, DevSgm5860xSetStruct *ptrNext) {
    if (ptrDevPinHandle == NULL) {
        printf("[ERROR]DevSgm5860xISRSetCallback: ptrDevPinHandle is NULL\r\n");
        return;
    }
    SGM5860xMuxReg_t mux_reg          = {0};
    SGM5860xAdconReg_t adcon_reg      = {0};
    SGM5860xMuxReg_t read_mux_reg     = {0};
    SGM5860xAdconReg_t read_adcon_reg = {0};
    if (DevPinRead(&ptrDevPinHandle->drdy)) {
        printf("DevSgm5860xISRSetCallback1: nDRDY is high, skipping ISR callback\r\n");
        return;  // nDRDY is high, skip the callback
    }
    DevSgm5860xHandleStruct *ptrDevSgm5860xHandle = (DevSgm5860xHandleStruct *)ptrDevPinHandle;
    /* STEP2 Write Next Channel config */
    mux_reg.bits.NSEL  = SGM58601_MUXN_AINCOM;
    mux_reg.bits.PSEL  = ptrNext->channel;
    adcon_reg.bits.PGA = ptrNext->gain;

    if (DevPinRead(&ptrDevPinHandle->drdy)) {
        printf("DevSgm5860xISRSetCallback5: nDRDY is high, skipping ISR callback\r\n");
        return;  // nDRDY is high, skip the callback
    }

    DevSgm5860xWriteReg(ptrDevSgm5860xHandle, SGM58601_MUX, (uint8_t *)&mux_reg, sizeof(mux_reg));
    if (DevPinRead(&ptrDevPinHandle->drdy)) {
        printf("DevSgm5860xISRSetCallback6: nDRDY is high, skipping ISR callback\r\n");
        return;  // nDRDY is high, skip the callback
    }
    DevSgm5860xWriteReg(ptrDevSgm5860xHandle, SGM58601_ADCON, (uint8_t *)&adcon_reg,
                        sizeof(adcon_reg));
    return;
}

/**
 * SGM5860x Offset Self-Calibration (SELFOCAL)
 */
int DevSgm5860xSelfOffsetCal(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle) {
    elog_i(TAG, "DevSgm5860xSelfOffsetCal: Start SELFOCAL (Offset Self-Calibration)");
    if (!ptrDevSgm5860xHandle) return -1;
    DevSgm5860xCommand(ptrDevSgm5860xHandle, SGM58601_CMD_SELFOCAL);
    while (!DevPinRead(&ptrDevSgm5860xHandle->drdy)) {
    }
    elog_d(TAG, "DevSgm5860xSelfOffsetCal: nDRDY high, calibration running");
    while (DevPinRead(&ptrDevSgm5860xHandle->drdy)) {
    }
    elog_i(TAG, "DevSgm5860xSelfOffsetCal: Calibration complete, nDRDY low");
    return 1;
}
/**
 * SGM5860x Gain Self-Calibration (SELFGCAL)
 */
int DevSgm5860xSelfGainCal(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle) {
    elog_i(TAG, "DevSgm5860xSelfGainCal: Start SELFGCAL (Gain Self-Calibration)");
    if (!ptrDevSgm5860xHandle) return -1;
    DevSgm5860xCommand(ptrDevSgm5860xHandle, SGM58601_CMD_SELFGCAL);
    while (!DevPinRead(&ptrDevSgm5860xHandle->drdy)) {
    }
    elog_d(TAG, "DevSgm5860xSelfGainCal: nDRDY high, calibration running");
    while (DevPinRead(&ptrDevSgm5860xHandle->drdy)) {
    }
    elog_i(TAG, "DevSgm5860xSelfGainCal: Calibration complete, nDRDY low");
    return 1;
}
/**
 * SGM5860x System Offset Calibration (SYSOCAL)
 */
int DevSgm5860xSysOffsetCal(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle) {
    elog_i(TAG, "DevSgm5860xSysOffsetCal: Start SYSOCAL (System Offset Calibration)");
    if (!ptrDevSgm5860xHandle) return -1;
    DevSgm5860xCommand(ptrDevSgm5860xHandle, SGM58601_CMD_SYSOCAL);
    while (!DevPinRead(&ptrDevSgm5860xHandle->drdy)) {
    }
    elog_d(TAG, "DevSgm5860xSysOffsetCal: nDRDY high, calibration running");
    while (DevPinRead(&ptrDevSgm5860xHandle->drdy)) {
    }
    elog_i(TAG, "DevSgm5860xSysOffsetCal: Calibration complete, nDRDY low");
    return 1;
}
/**
 * SGM5860x System Gain Calibration (SYSGCAL)
 */
int DevSgm5860xSysGainCal(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle) {
    elog_i(TAG, "DevSgm5860xSysGainCal: Start SYSGCAL (System Gain Calibration)");
    if (!ptrDevSgm5860xHandle) return -1;
    DevSgm5860xCommand(ptrDevSgm5860xHandle, SGM58601_CMD_SYSGCAL);
    while (!DevPinRead(&ptrDevSgm5860xHandle->drdy)) {
    }
    elog_d(TAG, "DevSgm5860xSysGainCal: nDRDY high, calibration running");
    while (DevPinRead(&ptrDevSgm5860xHandle->drdy)) {
    }
    elog_i(TAG, "DevSgm5860xSysGainCal: Calibration complete, nDRDY low");
    return 1;
}
