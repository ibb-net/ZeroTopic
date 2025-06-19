
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
// void sgm5860xsend(uint8_t ch, uint16_t data);

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
        // sgm5860xStatusHandle->cfg = sgm5860xBspCfg;  // Copy configuration
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
    printf("  data <ch> <data>  Set ADC data for channel <ch> in hex format (e.g., 0x1234)\r\n");
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
        // DevSpiWriteRead(&sgm5860xBspCfg.spi_cfg, sgm5860x_send_buffer_test,
        // sgm5860x_recv_buffer_test, sgm5860x_NEW_ARRAYSIZE);
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
    Cmdsgm5860xHelp();
    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), sgm5860x,
                 Cmdsgm5860xHandle, sgm5860x command);
#if 0
void sgm5860xWriteReg(unsigned char regaddr, unsigned char databyte)  // A
{
    SPI_CS1_ENABLE();
    while (SGM58601_DRDY);  // 当SGM58601_DRDY为低时才能写寄存器
    // 向寄存器写入数据地址
    SPI0_ReadWriteByte(SGM58601_CMD_WREG | (regaddr & 0x0F));
    // 写入数据的个数n-1
    SPI0_ReadWriteByte(0x00);
    // 向regaddr地址指向的寄存器写入数据databyte
    SPI0_ReadWriteByte(databyte);
    SPI_CS1_DISABLE();
}

// 初始化SGM58601  //  单端采集
void sgm5860xInit(void)  // A
{
    //*************自校准****************
    //  while(SGM58601_DRDY);
    //	SPI_CS1_ENABLE();
    //	SPI0_ReadWriteByte(SGM58601_CMD_SELFCAL);
    //	while(SGM58601_DRDY);
    //	SPI_CS1_DISABLE();
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
    //	SPI_CS1_DISABLE();
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
    SPI_CS1_DISABLE();

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