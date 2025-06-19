
#include "sgm5860x.h"
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
#include "dev_pin.h"
#include "dev_spi.h"
#include "dev_uart.h"
#include "elog.h"
#include "os_server.h"
// #include "app_event.h"

#define TAG            "sgm5860x"
#define sgm5860xLogLvl ELOG_LVL_INFO

#define sgm5860xPriority PriorityNormalEventGroup0
#ifndef sgm5860xChannelMax
#define sgm5860xChannelMax 1
#endif
#ifndef CONFIG_sgm5860x_CYCLE_TIMER_MS
#define CONFIG_sgm5860x_CYCLE_TIMER_MS 100
#endif

#define SPI_CS1_ENABLE  DevPinWrite(&sgm5860xBspCfg.spi_cfg.nss, 0)  // Set NEST pin low
#define SPI_CS1_DISABLE DevPinWrite(&sgm5860xBspCfg.spi_cfg.nss, 1)  // Set NEST pin high
#define SGM58601_DRDY   DevPinRead(&sgm5860xBspCfg.drdy)             // Read DRDY pin status

/* ===================================================================================== */
typedef struct {
    uint32_t ch;
    DevPinHandleStruct drdy;
    DevPinHandleStruct nest;
    DevPinHandleStruct sync;
    DevSpiHandleStruct spi_cfg;  // SPI 配置

} Typdefsgm5860xBSPCfg;

const Typdefsgm5860xBSPCfg sgm5860xBspCfg = {

    .ch = 0,
    .drdy =
        {
            .device_name = "sgm5860x_drdy",
            .base        = GPIOD,
            .af          = 0,
            .pin         = GPIO_PIN_9,
            .pin_mode    = DevPinModeInput,
            .bit_value   = 1,  // Active low
        },
    .nest =
        {
            .device_name = "sgm5860x_nest",
            .base        = GPIOD,
            .af          = 0,
            .pin         = GPIO_PIN_10,
            .pin_mode    = DevPinModeOutput,
            .bit_value   = 1,  // Active low
        },
    .sync =
        {
            .device_name = "sgm5860x_sync",
            .base        = GPIOD,
            .af          = 0,
            .pin         = GPIO_PIN_11,
            .pin_mode    = DevPinModeOutput,
            .bit_value   = 1,  // Active low
        },
    .spi_cfg =
        {
            .base = SPI1,
            // SPI configuration
            .nss =
                {
                    .device_name = "sgm5860x_nss",
                    .base        = GPIOB,
                    .af          = 0,
                    .pin         = GPIO_PIN_12,
                    .pin_mode    = DevPinModeOutput,
                    .bit_value   = 1,  // Active low
                },
            .clk =
                {
                    .device_name = "sgm5860x_clk",
                    .base        = GPIOB,
                    .af          = GPIO_AF_5,
                    .pin         = GPIO_PIN_13,
                    .pin_mode    = DevPinModeAF,
                },
            .mosi =
                {
                    .device_name = "sgm5860x_mosi",
                    .base        = GPIOB,
                    .af          = GPIO_AF_5,
                    .pin         = GPIO_PIN_15,
                    .pin_mode    = DevPinModeAF,
                },
            .miso =
                {
                    .device_name = "sgm5860x_miso",
                    .base        = GPIOB,
                    .af          = GPIO_AF_5,
                    .pin         = GPIO_PIN_14,
                    .pin_mode    = DevPinModeAF,
                },
            .spi_cfg =
                {
                    .device_mode          = SPI_MASTER,
                    .trans_mode           = SPI_TRANSMODE_FULLDUPLEX,
                    .data_size            = SPI_DATASIZE_8BIT,
                    .clock_polarity_phase = SPI_CK_PL_LOW_PH_2EDGE,
                    .nss                  = SPI_NSS_SOFT,
                    .prescale             = SPI_PSC_256,
                    .endian               = SPI_ENDIAN_MSB,
                },

        },

};
static void __sgm5860xCreateTaskHandle(void);
static void __sgm5860xRcvHandle(void *msg);
static void __sgm5860xCycHandle(void);
static void __sgm5860xInitHandle(void *msg);
void sgm5860xWaitForDRDY(void);
void sgm5860xRest(void);
void sgm5860xWriteReg(unsigned char regaddr, unsigned char databyte);
void sgm5860xInit(void);
void sgm5860xReadReg(unsigned char regaddr, unsigned char databyte);
// void sgm5860xsend(uint8_t ch, uint16_t data);
int sgm5860xReadSingleData(unsigned char channel);

typedef struct {
    char device_name[DEVICE_NAME_MAX];
    uint32_t id;  // ID
    Typdefsgm5860xBSPCfg cfg;

} Typdefsgm5860xStatus;
Typdefsgm5860xStatus sgm5860xStatus[sgm5860xChannelMax] = {0};

/* ===================================================================================== */

static const vfb_event_t sgm5860xEventList[] = {

    sgm5860xStart,
    sgm5860xStop,
    sgm5860xGet,
    sgm5860xSet,

};

static const VFBTaskStruct sgm5860x_task_cfg = {
    .name         = "VFBTasksgm5860x",  // Task name
    .pvParameters = NULL,
    // .uxPriority = 10,
    // // Task parameters
    .queue_num  = 8,                                                // Number of queues to subscribe
    .event_list = sgm5860xEventList,                                // Event list to subscribe
    .event_num  = sizeof(sgm5860xEventList) / sizeof(vfb_event_t),  // Number of events to subscribe
    .startup_wait_event_list = NULL,                                // Events to wait for at startup
    .startup_wait_event_num  = 0,  // Number of startup events to wait for
    .xTicksToWait            = pdMS_TO_TICKS(CONFIG_sgm5860x_CYCLE_TIMER_MS),  // Wait indefinitely
    .init_msg_cb             = __sgm5860xInitHandle,  // Callback for initialization messages
    .rcv_msg_cb              = __sgm5860xRcvHandle,   // Callback for received messages
    .rcv_timeout_cb          = __sgm5860xCycHandle,   // Callback for timeout
};

/* ===================================================================================== */

void sgm5860xDeviceInit(void) {
    elog_i(TAG, "sgm5860xDeviceInit");

    for (size_t i = 0; i < sgm5860xChannelMax; i++) {
        Typdefsgm5860xStatus *sgm5860xStatusHandle = &sgm5860xStatus[i];
        sgm5860xStatusHandle->id                   = i;
        memset(sgm5860xStatusHandle->device_name, 0, sizeof(sgm5860xStatusHandle->device_name));
        snprintf(sgm5860xStatusHandle->device_name, sizeof(sgm5860xStatusHandle->device_name),
                 "sgm5860x%d", i);
        sgm5860xStatusHandle->cfg = sgm5860xBspCfg;  // Copy configuration
        // Initialize the BSP configuration
        DevPinInit(&sgm5860xStatusHandle->cfg.drdy);
        DevPinInit(&sgm5860xStatusHandle->cfg.nest);
        DevPinInit(&sgm5860xStatusHandle->cfg.sync);
        DevSpiInit(&sgm5860xStatusHandle->cfg.spi_cfg);
        elog_i(TAG, "sgm5860x[%d] device_name: %s, spi_base: 0x%08X", i,
               sgm5860xStatusHandle->device_name, sgm5860xStatusHandle->cfg.spi_cfg.base);
    }
}
SYSTEM_REGISTER_INIT(MCUInitStage, sgm5860xPriority, sgm5860xDeviceInit, sgm5860xDeviceInit);

static void __sgm5860xCreateTaskHandle(void) {
    for (size_t i = 0; i < sgm5860xChannelMax; i++) {
    }
    xTaskCreate(VFBTaskFrame, "VFBTasksgm5860x", configMINIMAL_STACK_SIZE * 2,
                (void *)&sgm5860x_task_cfg, PriorityNormalEventGroup0, NULL);
}
SYSTEM_REGISTER_INIT(BoardInitStage, sgm5860xPriority, __sgm5860xCreateTaskHandle,
                     __sgm5860xCreateTaskHandle init);

static void __sgm5860xInitHandle(void *msg) {
    elog_i(TAG, "__sgm5860xInitHandle");
    elog_set_filter_tag_lvl(TAG, sgm5860xLogLvl);
    // TODO init ADC
    vfb_send(sgm5860xStart, 0, NULL, 0);
}
// 接收消息的回调函数

static void __sgm5860xRcvHandle(void *msg) {
    TaskHandle_t curTaskHandle = xTaskGetCurrentTaskHandle();
    // Typdefsgm5860xStatus *sgm5860xStatusTmp = (Typdefsgm5860xStatus *)&sgm5860xStatus[0];
    char *taskName        = pcTaskGetName(curTaskHandle);
    vfb_message_t tmp_msg = (vfb_message_t)msg;
    switch (tmp_msg->frame->head.event) {
        case sgm5860xStart: {
            elog_i(TAG, "sgm5860xStartTask %d", tmp_msg->frame->head.data);
        } break;
        case sgm5860xSet: {
            // Set ADC data for a specific channel
            uint8_t ch    = (uint8_t)(tmp_msg->frame->head.data & 0xFF);
            uint16_t data = (uint16_t)((tmp_msg->frame->head.data >> 8) & 0xFFFF);
            if (ch < sgm5860xChannelMax) {
                elog_i(TAG, "Set ADC channel %d to data 0x%04X", ch, data);
                // sgm5860xsend(ch, data);
            } else {
                elog_e(TAG, "Invalid channel: %d", ch);
            }
        } break;
        default:
            elog_e(TAG, "TASK %s RCV: unknown event: %d", taskName, tmp_msg->frame->head.event);
            break;
    }
}

static void __sgm5860xCycHandle(void) {
    Typdefsgm5860xStatus *sgm5860xStatusHandle = &sgm5860xStatus[0];
    if (sgm5860xStatusHandle == NULL) {
        elog_e(TAG, "[ERROR]sgm5860xStatusHandle NULL");
        return;
    }
}

#endif

static void Cmdsgm5860xHelp(void) {
    printf("Usage: sgm5860x <command>\r\n");
    printf("Commands:\r\n");
    printf("  help          Show this help message\r\n");
    printf("  test          Test SPI send/receive functionality\r\n");
    printf("  data <ch> <data>  Set ADC data for channel <ch> in hex format (e.g., 0x1234)\r\n");
    printf("  reset         Reset the SGM5860x device\r\n");
    printf("  init          Initialize the SGM5860x device\r\n");
}
#define sgm5860x_NEW_ARRAYSIZE 10
uint8_t sgm5860x_send_buffer_test[sgm5860x_NEW_ARRAYSIZE] = {0xA1, 0xA2, 0xA3, 0xA4, 0xA5,
                                                             0xA6, 0xA7, 0xA8, 0xA9, 0xAA};
uint8_t sgm5860x_recv_buffer_test[sgm5860x_NEW_ARRAYSIZE] = {0xC1, 0xC2, 0xC3, 0xC4, 0xC5,
                                                             0xC6, 0xC7, 0xC8, 0xC9, 0xCA};

static int Cmdsgm5860xHandle(int argc, char *argv[]) {
    if (argc < 2) {
        Cmdsgm5860xHelp();
        return 0;
    }
    if (strcmp(argv[1], "help") == 0) {
        Cmdsgm5860xHelp();
        return 0;
    }
    if (strcmp(argv[1], "test") == 0) {
        // spi_rcv_send_test();
        printf("Testing SPI send/receive\r\n");
        static uint8_t counter = 0;
        for (size_t i = 0; i < sgm5860x_NEW_ARRAYSIZE; i++) {
            sgm5860x_send_buffer_test[i] = counter++;
        }
        DevSpiWriteRead(&sgm5860xBspCfg.spi_cfg, sgm5860x_send_buffer_test,
                        sgm5860x_recv_buffer_test, sgm5860x_NEW_ARRAYSIZE);
        elog_hexdump(TAG, 16, sgm5860x_send_buffer_test, sgm5860x_NEW_ARRAYSIZE);
        elog_hexdump(TAG, 16, sgm5860x_recv_buffer_test, sgm5860x_NEW_ARRAYSIZE);
        return 0;
    }
    if (strcmp(argv[1], "data") == 0) {
        if (argc < 4) {
            elog_e(TAG, "Usage: sgm5860x data <ch> <data>");
            return 0;
        }
        printf("\r\nSetting ADC data for channel %s with data %s\r\n", argv[2], argv[3]);
        uint8_t ch    = atoi(argv[2]);
        uint16_t data = (uint16_t)strtol(argv[3], NULL, 16);  // Convert hex string to uint16_t
        if (ch >= sgm5860xChannelMax) {
            elog_e(TAG, "Invalid channel: %d", ch);
            return 0;
        }
        // sgm5860xsend(ch, data);
        return 0;
    }
    // reset
    if (strcmp(argv[1], "reset") == 0) {
        sgm5860xRest();
        elog_i(TAG, "SGM5860x reset completed");
        return 0;
    }
    // sgm5860xInit
    if (strcmp(argv[1], "init") == 0) {
        sgm5860xInit();
        elog_i(TAG, "SGM5860x initialized");
        return 0;
    }
    // read
    if (strcmp(argv[1], "read") == 0) {
        if (argc < 3) {
            elog_e(TAG, "Usage: sgm5860x read <regaddr>");
            return 0;
        }
        unsigned char regaddr = (unsigned char)strtol(argv[2], NULL, 16);
        elog_i(TAG, "Read register 0x%02X", regaddr);
        return 0;
    }
    // value
    if (strcmp(argv[1], "value") == 0) {
        if (argc < 3) {
            elog_e(TAG, "Usage: sgm5860x value <regaddr>");
            return 0;
        }
        sgm5860xReadSingleData(0);
        sgm5860xReadSingleData(2);
        sgm5860xReadSingleData(4);
        sgm5860xReadSingleData(6);

        return 0;
    }

    Cmdsgm5860xHelp();
    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), sgm5860x,
                 Cmdsgm5860xHandle, sgm5860x command);

// 0h STATUS: Status Register x1h ID[3:0] ORDER ACAL BUFEN nDRD
/* ORDER */
#define ORDER_MSB_FIRST 0  // Most significant bit first (default)
#define ORDER_LSB_FIRST 1  // Least significant bit first

typedef struct {
    uint8_t NSEL : 4;  // Bits 0-3: Negative Input Selection
    uint8_t PSEL : 4;  // Bits 4-7: Positive Input Selection
} SGM5860xMuxReg_t;

typedef struct {
    uint8_t PGA : 3;       // Bits 0-2: Programmable Gain Amplifier (PGA)
    uint8_t SDCS : 2;      // Bits 3-4: Sensor Detection Current Source (SDCS)
    uint8_t CLK : 2;       // Bits 5-6: Clock Source Selection
    uint8_t RESERVED : 1;  // Bit 7: Reserved
} SGM5860xAdconReg_t;

typedef union {
    struct {
        uint8_t nDRDY : 1;  // Bit 0: Data Ready (Active Low)
        uint8_t BUFEN : 1;  // Bit 1: Buffer Enable
        uint8_t ACAL : 1;   // Bit 2: Auto Calibration Enable
        uint8_t ORDER : 1;  // Bit 3: Data Order (0: MSB First, 1: LSB First)
        uint8_t ID : 4;     // Bits 4-7: Device ID
    } bits;                 // 位域结构体
    uint8_t raw;            // 原始寄存器值
} SGM5860xStatusReg_t;
void sgm5860xWaitForDRDY(void) {
    while (!DevPinRead(&sgm5860xBspCfg.drdy)) {
        // Do nothing, just wait
    }
}
void sgm5860xRest(void) {
    elog_i(TAG, "sgm5860xRest");
    DevPinWrite(&sgm5860xBspCfg.nest, 0);  // Set NEST pin low
    vTaskDelay(pdMS_TO_TICKS(1));          // Wait for 10 ms
    DevPinWrite(&sgm5860xBspCfg.nest, 1);  // Set NEST pin high
    sgm5860xWaitForDRDY();                 // Wait for DRDY to go low
}

void sgm5860xWriteReg(unsigned char regaddr, unsigned char databyte)  // A
{
    SPI_CS1_ENABLE;
    while (SGM58601_DRDY);  // 当SGM58601_DRDY为低时才能写寄存器
    // 向寄存器写入数据地址
    // SGM58601_CMD_WREG | (regaddr & 0x0F)  // 0x40 | regaddr
    // 0x40 = SGM58601_CMD_WREG
    uint8_t snd_data[3] = {SGM58601_CMD_WREG | (regaddr & 0x0F), 0, databyte};
    uint8_t rcv_data[3] = {0, 0, 0};
    DevSpiWriteRead(&sgm5860xBspCfg.spi_cfg, snd_data, rcv_data, 3);

    uint8_t read_snd_data[3] = {SGM58601_CMD_RREG | (regaddr & 0x0F), 0, 0};
    DevSpiWriteRead(&sgm5860xBspCfg.spi_cfg, read_snd_data, rcv_data, 3);
    // SPI0_ReadWriteByte(SGM58601_CMD_WREG | (regaddr & 0x0F));
    // 写入数据的个数n-1
    // SPI0_ReadWriteByte(0x00);
    // 向regaddr地址指向的寄存器写入数据databyte
    // SPI0_ReadWriteByte(databyte);
    SPI_CS1_DISABLE;
    elog_d(TAG, "sgm5860xWriteReg regaddr: 0x%02X databyte: 0x%02X", regaddr, databyte);
    elog_d(TAG, "sgm5860xWriteReg snd_data: 0x%02X 0x%02X 0x%02X", snd_data[0], snd_data[1],
           snd_data[2]);
    elog_d(TAG, "sgm5860xWriteReg rcv_data: 0x%02X 0x%02X 0x%02X", rcv_data[0], rcv_data[1],
           rcv_data[2]);
}
void sgm5860xReadReg(unsigned char regaddr, unsigned char databyte)  // A
{
    SPI_CS1_ENABLE;
    while (SGM58601_DRDY);  // 当SGM58601_DRDY为低时才能写寄存器
    // 向寄存器写入数据地址
    // SGM58601_CMD_WREG | (regaddr & 0x0F)  // 0x40 | regaddr
    // 0x40 = SGM58601_CMD_WREG
    uint8_t snd_data[3] = {SGM58601_CMD_RREG | (regaddr & 0x0F), 0, 0};
    uint8_t rcv_data[3] = {0, 0, 0};
    DevSpiWriteRead(&sgm5860xBspCfg.spi_cfg, snd_data, rcv_data, 3);

    //

    // SPI0_ReadWriteByte(SGM58601_CMD_WREG | (regaddr & 0x0F));
    // 写入数据的个数n-1
    // SPI0_ReadWriteByte(0x00);
    // 向regaddr地址指向的寄存器写入数据databyte
    // SPI0_ReadWriteByte(databyte);
    SPI_CS1_DISABLE;
    elog_d(TAG, "sgm5860xWriteReg regaddr: 0x%02X databyte: 0x%02X", regaddr, databyte);
    elog_d(TAG, "sgm5860xWriteReg snd_data: 0x%02X 0x%02X 0x%02X", snd_data[0], snd_data[1],
           snd_data[2]);
    elog_d(TAG, "sgm5860xWriteReg rcv_data: 0x%02X 0x%02X 0x%02X", rcv_data[0], rcv_data[1],
           rcv_data[2]);
}

// 初始化SGM58601  //  单端采集
void sgm5860xInit(void)  // A
{
    //*************自校准****************
    //  while(SGM58601_DRDY);
    //	SPI_CS1_ENABLE();
    //	SPI0_ReadWriteByte(SGM58601_CMD_SELFCAL);
    //	while(SGM58601_DRDY);
    //	SPI_CS1_DISABLE;
    //**********************************

    SGM5860xStatusReg_t status_reg;
    status_reg.bits.nDRDY = 0;                // Data Ready (Active Low)
    status_reg.bits.BUFEN = 1;                // Buffer Enable
    status_reg.bits.ACAL  = 1;                // Auto Calibration Enable
    status_reg.bits.ORDER = ORDER_MSB_FIRST;  // Data Order (0: MSB First, 1: LSB First)
    status_reg.bits.ID    = 0;                // Device ID (0-15)

    uint8_t regaddr     = SGM58601_STATUS;
    uint8_t databyte    = status_reg.raw;  // Convert to raw byte
    uint8_t snd_data[3] = {SGM58601_CMD_WREG | (regaddr & 0x0F), 0, databyte};
    uint8_t rcv_data[3] = {0, 0, 0};
    // write

    SPI_CS1_ENABLE;
    while (SGM58601_DRDY);
    DevSpiWriteRead(&sgm5860xBspCfg.spi_cfg, snd_data, rcv_data, 3);
    SPI_CS1_DISABLE;
    elog_i(TAG, "Write nDRDY: %d, BUFEN: %d, ACAL: %d, ORDER: %d, ID: %d", status_reg.bits.nDRDY,
           status_reg.bits.BUFEN, status_reg.bits.ACAL, status_reg.bits.ORDER, status_reg.bits.ID);

    SPI_CS1_ENABLE;
    while (SGM58601_DRDY);
    uint8_t read_snd_data[3] = {SGM58601_CMD_RREG | (regaddr & 0x0F), 0, 0};
    uint8_t read_rcv_data[3] = {0, 0, 0};
    uint8_t read_regaddr     = SGM58601_STATUS;
    DevSpiWriteRead(&sgm5860xBspCfg.spi_cfg, read_snd_data, read_rcv_data, 3);
    SPI_CS1_DISABLE;
    SGM5860xStatusReg_t read_status_reg;
    read_status_reg.raw = read_rcv_data[2];  // Read the status register value
    elog_i(TAG, "Read nDRDY: %d, BUFEN: %d, ACAL: %d, ORDER: %d, ID: %d",
           read_status_reg.bits.nDRDY, read_status_reg.bits.BUFEN, read_status_reg.bits.ACAL,
           read_status_reg.bits.ORDER, read_status_reg.bits.ID);

    snd_data[0] = SGM58601_CMD_WREG | (0x01 & 0x0F);
    snd_data[1] = 0;  // Number of bytes to write - 1

    read_snd_data[0] = SGM58601_CMD_RREG | (0x01 & 0x0F);
    read_snd_data[1] = 0;  // Number of bytes to read
    read_snd_data[2] = 0;

    for (size_t i = 0; i < 255; i++) {
        memset(read_rcv_data, 0, sizeof(read_rcv_data));
        elog_w(TAG, "Clear read_rcv_data %d: 0x%02X 0x%02X 0x%02X",
              i, read_rcv_data[0], read_rcv_data[1], read_rcv_data[2]);
        snd_data[2] = i;  // Example data to write
        SPI_CS1_ENABLE;
        while (SGM58601_DRDY);
        DevSpiWriteRead(&sgm5860xBspCfg.spi_cfg, snd_data, rcv_data, 3);
        SPI_CS1_DISABLE;

        SPI_CS1_ENABLE;
        while (SGM58601_DRDY);
        DevSpiWriteRead(&sgm5860xBspCfg.spi_cfg, read_snd_data, read_rcv_data, 3);
        SPI_CS1_DISABLE;
        elog_i(TAG, "Index %d: Write 0x%02X, Read 0x%02X %s",
               i, snd_data[2], read_rcv_data[2],
               (read_rcv_data[2] == snd_data[2]) ? "OK" : "Mismatch");
    }

    // sgm5860xReadReg(SGM58601_STATUS, 0x06);
    //	sgm5860xWriteReg(SGM58601_STATUS,0x04); //
    // 高位在前、不使用缓冲

    // sgm5860xWriteReg(SGM58601_MUX, 0xF7);                    // 初始化端口A0为‘+’，AINCOM位‘-’
    // sgm5860xWriteReg(SGM58601_ADCON, SGM58601_GAIN_1);       // 放大倍数1
    // sgm5860xWriteReg(SGM58601_DRATE, SGM58601_DRATE_50SPS);  // 数据10sps
    // sgm5860xWriteReg(SGM58601_IO, 0x00);

    //*************自校准****************
    //	while(SGM58601_DRDY);
    //	SPI_CS1_ENABLE();
    //	SPI0_ReadWriteByte(SGM58601_CMD_SELFCAL);
    //	while(SGM58601_DRDY);
    //	SPI_CS1_DISABLE;
    //**********************************
}

int sgm5860xReadSingleData(unsigned char channel) {
    uint8_t data = (channel << 4) | SGM58601_MUXN_AINCOM;  // 设置通道

    unsigned int sum = 0;

    while (SGM58601_DRDY);                 // 当SGM58601_DRDY为低时才能写寄存器
    sgm5860xWriteReg(SGM58601_MUX, data);  // 设置通道
    while (SGM58601_DRDY);
    SPI_CS1_ENABLE;
    uint8_t snd_data[6] = {SGM58601_CMD_SYNC, SGM58601_CMD_WAKEUP, SGM58601_CMD_RDATA};
    uint8_t rcv_data[6] = {0, 0, 0};
    DevSpiWriteRead(&sgm5860xBspCfg.spi_cfg, snd_data, rcv_data, 6);
    // SPI0_ReadWriteByte(SGM58601_CMD_SYNC);
    // SPI0_ReadWriteByte(SGM58601_CMD_WAKEUP);
    // SPI0_ReadWriteByte(SGM58601_CMD_RDATA);
    // sum |= (SPI0_ReadWriteByte(0xff) << 16);
    // sum |= (SPI0_ReadWriteByte(0xff) << 8);
    // sum |= SPI0_ReadWriteByte(0xff);
    SPI_CS1_DISABLE;
    sum |= (rcv_data[3] << 16);
    sum |= (rcv_data[4] << 8);
    sum |= rcv_data[5];
    if (sum > 0x7FFFFF)  // if MSB=1,
    {
        sum -= 0x1000000;  // do 2's complement
    }
    elog_d(TAG, "sgm5860xReadSingleData channel: %d, sum: 0x%08X", channel, sum);
    elog_d(TAG, "sgm5860xReadSingleData snd_data: 0x%02X 0x%02X 0x%02X", snd_data[0], snd_data[1],
           snd_data[2]);
    elog_d(TAG, "sgm5860xReadSingleData rcv_data: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",
           rcv_data[0], rcv_data[1], rcv_data[2], rcv_data[3], rcv_data[4], rcv_data[5]);
    elog_i(TAG, "sgm5860xReadSingleData channel: %d, sum: %d", channel, sum);
    return sum;
}
#if 0


// 初始化SGM58601  //  单端采集
void sgm5860xInit(void)  // A
{
    //*************自校准****************
    //  while(SGM58601_DRDY);
    //	SPI_CS1_ENABLE();
    //	SPI0_ReadWriteByte(SGM58601_CMD_SELFCAL);
    //	while(SGM58601_DRDY);
    //	SPI_CS1_DISABLE;
    //**********************************

    sgm5860xWriteReg(SGM58601_STATUS, 0x06);  // 高位在前、使用缓冲
                                              //	sgm5860xWriteReg(SGM58601_STATUS,0x04);               // 高位在前、不使用缓冲

    //	sgm5860xWriteReg(SGM58601_MUX,0x08);                  // 初始化端口A0为‘+’，AINCOM位‘-’
    sgm5860xWriteReg(SGM58601_ADCON, SGM58601_GAIN_1);         // 放大倍数1
    sgm5860xWriteReg(SGM58601_DRATE, SGM58601_DRATE_7500SPS);  // 数据10sps
    sgm5860xWriteReg(SGM58601_IO, 0x00);

    //*************自校准****************
    //	while(SGM58601_DRDY);
    //	SPI_CS1_ENABLE();
    //	SPI0_ReadWriteByte(SGM58601_CMD_SELFCAL);
    //	while(SGM58601_DRDY);
    //	SPI_CS1_DISABLE;
    //**********************************
}

// 读取单端AD值
signed int sgm5860xReadReg(unsigned char channel)  // A
{
    unsigned int sum = 0;

    while (SGM58601_DRDY);                    // 当SGM58601_DRDY为低时才能写寄存器
    sgm5860xWriteReg(SGM58601_MUX, channel);  // 设置通道
    SPI_CS1_ENABLE();
    SPI0_ReadWriteByte(SGM58601_CMD_SYNC);
    SPI0_ReadWriteByte(SGM58601_CMD_WAKEUP);
    SPI0_ReadWriteByte(SGM58601_CMD_RDATA);
    sum |= (SPI0_ReadWriteByte(0xff) << 16);
    sum |= (SPI0_ReadWriteByte(0xff) << 8);
    sum |= SPI0_ReadWriteByte(0xff);
    SPI_CS1_DISABLE;

    if (sum > 0x7FFFFF)  // if MSB=1,
    {
        sum -= 0x1000000;  // do 2's complement
    }
    return sum;
}

int sgm5860xReadSingleData(unsigned char channel) {
    return sgm5860xReadReg((channel << 4) | SGM58601_MUXN_AINCOM);
}

int sgm5860xReadDiffData(void) {
    return SGM58601BReadData(SGM58601_MUXP_AIN0 | SGM58601_MUXN_AIN1);
}

#endif