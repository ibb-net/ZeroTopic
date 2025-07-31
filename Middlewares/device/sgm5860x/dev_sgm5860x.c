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
volatile SGM5860xAllReg_t write_regs = {0};
volatile SGM5860xAllReg_t read_regs  = {0};
int DevSgm5860xInit(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle) {
    if (ptrDevSgm5860xHandle == NULL) {
        printf("         [ERROR] DevSgm5860xInit: ptrDevSgm5860xHandle is NULL");
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
        printf("         [ERROR] __DevSgm5860xWaitForDRDY: ptrDevSgm5860xHandle is NULL");
        return 0;
    }
    uint32_t timeout = SGM5860_WAIT_TIME_US;  // SGM5860_WAIT_TIME_US;
    while (DevPinRead(&ptrDevSgm5860xHandle->drdy)) {
        // DevDelayUs(1);  // Wait for DRDY to go low
        vTaskDelay(pdMS_TO_TICKS(1));  // Wait for 1 millisecond
        timeout--;
        if (timeout == 0) {
            //  printf("         [ERROR] __DevSgm5860xWaitForDRDY: Timeout waiting for DRDY");
            return -1;
        }
    }
    return 1;
}

void DevSgm5860xReset(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle) {
    if (ptrDevSgm5860xHandle == NULL) {
        printf("         [ERROR] DevSgm5860xReset: ptrDevSgm5860xHandle is NULL");
        return;
    }
    printf("         DevSgm5860xReset: Device reset Started");
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
        printf("         [ERROR] DevSgm5860xWriteReg: ptrDevSgm5860xHandle or regvalue is NULL");
        return -1;
    }
    if (reglen < 1 || reglen > 0x0b) {
        printf("         [ERROR] DevSgm5860xWriteReg: Invalid reglen %d", reglen);
        return -1;  // Invalid register length
    }
    if (regaddr > 0x0F) {
        printf("         [ERROR] DevSgm5860xWriteReg: Invalid regaddr 0x%02X", regaddr);
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
    // elog_d(TAG, "%s", log_msg);
    return 1;  // Success
}
int DevSgm5860xReadReg(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle, uint8_t regaddr,
                       uint8_t *regvalue, uint8_t reglen) {
    if (ptrDevSgm5860xHandle == NULL || regvalue == NULL) {
        printf("         [ERROR] DevSgm5860xReadReg: ptrDevSgm5860xHandle or regvalue is NULL");
        return -1;
    }
    if (reglen < 1 || reglen > 0x0b) {
        printf("         [ERROR] DevSgm5860xReadReg: Invalid reglen %d", reglen);
        return -1;  // Invalid register length
    }
    if (regaddr > 0x0F) {
        printf("         [ERROR] DevSgm5860xWriteReg: Invalid regaddr 0x%02X", regaddr);
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
    // elog_d(TAG, "DevGetADCData: Received data: 0x%06X", hex_data);
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
        printf("         [ERROR] DevSgm5860xStart: ptrDevSgm5860xHandle is NULL");
        return;
    }
    DevPinIsStartISR(&ptrDevSgm5860xHandle->drdy, 1);
}
void DevSgm5860xStop(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle) {
    if (ptrDevSgm5860xHandle == NULL) {
        printf("         [ERROR] DevSgm5860xStop: ptrDevSgm5860xHandle is NULL");
        return;
    }
    DevPinIsStartISR(&ptrDevSgm5860xHandle->drdy, 0);
}

// 需要再中断回调中调用,不能使用elog和阻塞性的函数
uint8_t DevSgm5860xSet(const DevSgm5860xHandleStruct *ptrDevPinHandle,
                       DevSgm5860xSetStruct *ptrSet) {
    if (ptrDevPinHandle == NULL) {
        printf("[ERROR]DevSgm5860xSet: ptrDevPinHandle is NULL\r\n");
        return 0;
    }
    if (ptrSet == NULL) {
        printf("[ERROR]DevSgm5860xSet: ptrSet is NULL\r\n");
        return 0;
    }
    uint8_t channel                               = ptrSet->channel;
    uint8_t gain                                  = ptrSet->gain;
    uint8_t sps                                   = ptrSet->sps;
    DevSgm5860xHandleStruct *ptrDevSgm5860xHandle = (DevSgm5860xHandleStruct *)ptrDevPinHandle;
    static uint8_t channel_last                   = -1;
    static uint8_t gain_last                      = -1;
    static uint8_t sps_last                       = 0xFF;

    /* status */
    write_regs.regs.status.bits.BUFEN = 1;
    write_regs.regs.status.bits.ACAL  = 0;
    write_regs.regs.status.bits.ORDER = 0;
    write_regs.regs.status.bits.ID    = 0x00;

    write_regs.regs.mux.bits.NSEL = SGM58601_MUXN_AINCOM;  // Set NSEL to AINCOM
    write_regs.regs.mux.bits.PSEL = (uint8_t)(channel & 0xFF);

    write_regs.regs.adcon.bits.PGA      = (uint8_t)(gain & 0x07);  // Gain code
    write_regs.regs.adcon.bits.CLK      = 0;
    write_regs.regs.adcon.bits.SDCS     = 0;  // Sensor Detection Current Source
    write_regs.regs.adcon.bits.reserved = 0;  // Reserved bit

    write_regs.regs.drate.bits.DR = sps;  // Set the data rate

    write_regs.regs.io.raw = 0;  // Reserved byte SGM58601_IO
    if (ptrSet->offset != NULL) {
        // printf("[INFO] Set Offset: 0x%02X 0x%02X 0x%02X\r\n", ptrSet->offset[0], ptrSet->offset[1], ptrSet->offset[2]);
        write_regs.regs.ofc0.raw = ptrSet->offset[0];
        write_regs.regs.ofc1.raw = ptrSet->offset[1];
        write_regs.regs.ofc2.raw = ptrSet->offset[2];
    }
 
    if (ptrSet->gain_offset != NULL) {
        // printf("[INFO] Set Gain Offset: 0x%02X 0x%02X 0x%02X\r\n", ptrSet->gain_offset[0], ptrSet->gain_offset[1], ptrSet->gain_offset[2]);
        write_regs.regs.fsc0.raw = ptrSet->gain_offset[0];
        write_regs.regs.fsc1.raw = ptrSet->gain_offset[1];
        write_regs.regs.fsc2.raw = ptrSet->gain_offset[2];
    }
    if ((ptrSet->offset != NULL) || (ptrSet->gain != NULL)) {
        DevSgm5860xWriteReg(ptrDevSgm5860xHandle, SGM58601_STATUS, write_regs.raws, 11);
    } else {
        printf("[INFO] Set Default Configuration\r\n");
        DevSgm5860xWriteReg(ptrDevSgm5860xHandle, SGM58601_STATUS, write_regs.raws, 4);
    }
    return 1;
}
int DevSgm5860xSetRead(const DevSgm5860xHandleStruct *ptrDevPinHandle,DevSgm5860xSetStruct *ptrSet)
{
   int result = DevSgm5860xReadReg(ptrDevPinHandle, SGM58601_STATUS, read_regs.raws, 11);
   if(result != 1)
       return result;
    // 对比 read_regs 和 ptrSet 的设置是否一致
    int mismatch = 0;
    if (read_regs.regs.mux.bits.PSEL != ptrSet->channel) {
        printf("[ERROR] Channel mismatch: read=0x%02X, expected=0x%02X\r\n", read_regs.regs.mux.bits.PSEL, ptrSet->channel);
        mismatch = 1;
    }
    if (read_regs.regs.adcon.bits.PGA != ptrSet->gain) {
        printf("[ERROR] Gain mismatch: read=0x%02X, expected=0x%02X\r\n", read_regs.regs.adcon.bits.PGA, ptrSet->gain);
        mismatch = 1;
    }
    if (read_regs.regs.drate.bits.DR != ptrSet->sps) {
        printf("[ERROR] SPS mismatch: read=0x%02X, expected=0x%02X\r\n", read_regs.regs.drate.bits.DR, ptrSet->sps);
        mismatch = 1;
    }
    if (ptrSet->offset != NULL &&
        (read_regs.regs.ofc0.raw != ptrSet->offset[0] ||
         read_regs.regs.ofc1.raw != ptrSet->offset[1] ||
         read_regs.regs.ofc2.raw != ptrSet->offset[2])) {
        printf("[ERROR] Offset mismatch: read=0x%02X%02X%02X, expected=0x%02X%02X%02X\r\n",
            read_regs.regs.ofc0.raw, read_regs.regs.ofc1.raw, read_regs.regs.ofc2.raw,
            ptrSet->offset[0], ptrSet->offset[1], ptrSet->offset[2]);
        mismatch = 1;
    }
    if (ptrSet->gain_offset != NULL &&
        (read_regs.regs.fsc0.raw != ptrSet->gain_offset[0] ||
         read_regs.regs.fsc1.raw != ptrSet->gain_offset[1] ||
         read_regs.regs.fsc2.raw != ptrSet->gain_offset[2])) {
        printf("[ERROR] Gain offset mismatch: read=0x%02X%02X%02X, expected=0x%02X%02X%02X\r\n",
            read_regs.regs.fsc0.raw, read_regs.regs.fsc1.raw, read_regs.regs.fsc2.raw,
            ptrSet->gain_offset[0], ptrSet->gain_offset[1], ptrSet->gain_offset[2]);
        mismatch = 1;
    }
    if (!mismatch) {
        // printf("[PASS] read_regs and ptrSet configuration match.\r\n");
    }
  
   return 1;
}
uint8_t DevSgm5860xStartContinuousMode(const DevSgm5860xHandleStruct *ptrDevPinHandle) {
    DevSgm5860xCommand(ptrDevPinHandle, SGM58601_CMD_WAKEUP);
    DevSgm5860xCommand(ptrDevPinHandle, SGM58601_CMD_RDATAC);
    return 1;  // Success
}

/**
 * SGM5860x Offset Self-Calibration (SELFOCAL)
 */
int DevSgm5860xSelfOffsetCal(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle) {
    if (!ptrDevSgm5860xHandle) return -1;
    // DevSgm5860xCommand(ptrDevSgm5860xHandle, SGM58601_CMD_SDATAC);   // 停止连续采集
    // DevSgm5860xCommand(ptrDevSgm5860xHandle, SGM58601_CMD_STANDBY);  // 停止连续采集

    // 等待 nDRDY 低电平（数据空闲）
    while (DevPinRead(&ptrDevSgm5860xHandle->drdy));
    // 发送自偏移校准命令
    DevSgm5860xCommand(ptrDevSgm5860xHandle, SGM58601_CMD_SELFCAL);
    // 等待 nDRDY 高电平（校准开始）
    while (!DevPinRead(&ptrDevSgm5860xHandle->drdy));
    // 等待 nDRDY 低电平（校准完成）
    while (DevPinRead(&ptrDevSgm5860xHandle->drdy));
    return 1;
}
int DevSgm5860xSelfOffsetCal2(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle) {
    if (!ptrDevSgm5860xHandle) return -1;
    // DevSgm5860xCommand(ptrDevSgm5860xHandle, SGM58601_CMD_SELFOCAL);
    return 1;
}
int DevSgm5860xSelfOffsetCalRead(const DevSgm5860xHandleStruct *ptrDevSgm5860xHandle,
                                 uint8_t *offset, uint8_t *gain_offset) {
    if (!ptrDevSgm5860xHandle) return -1;

    if (DevSgm5860xReadReg(ptrDevSgm5860xHandle, SGM58601_OFC0, (uint8_t *)offset, 3) != 1) {
        return -1;
    }
    if (DevSgm5860xReadReg(ptrDevSgm5860xHandle, SGM58601_FSC0, (uint8_t *)gain_offset, 3) != 1) {
        return -1;
    }
    return 1;
}
