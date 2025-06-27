
#include "DACSgm3533Cfg.h"
#define CONFIG_DACSgm3533_EN 1
#if CONFIG_DACSgm3533_EN
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
#include "timer_decoder.h"
// #include "app_event.h"

#define TAG              "DAC"
#define DACSgm3533LogLvl ELOG_LVL_INFO

#define DACSgm3533Priority PriorityNormalEventGroup0
#ifndef DACSgm3533ChannelMax
#define DACSgm3533ChannelMax 2
#endif
#ifndef CONFIG_DACSgm3533_CYCLE_TIMER_MS
#define CONFIG_DACSgm3533_CYCLE_TIMER_MS 100
#endif
/* ===================================================================================== */
#define ARRAYSIZE 2
uint8_t spi_send_buffer[DACSgm3533ChannelMax][ARRAYSIZE] = {
    {0xA5, 0xA5},  // Channel 0
    {0x5a, 0x5a},  // Channel 1
};
typedef struct {
    uint32_t ch;
    DevSpiHandleStruct spi_cfg;  // SPI 配置

} TypdefDACSgm3533BSPCfg;

static void __DACSgm3533CreateTaskHandle(void);
static void __DACSgm3533RcvHandle(void *msg);
static void __DACSgm3533CycHandle(void);
static void __DACSgm3533InitHandle(void *msg);
void DACSgm3533DSPISend(uint8_t ch, uint16_t data);

const TypdefDACSgm3533BSPCfg DACSgm3533BspCfg[DACSgm3533ChannelMax] = {
    /* SPI2  */

    {
        .ch      = 0,
        .spi_cfg = {.base = SPI2,
                    .nss =
                        {
                            .device_name = "DACSgm3533_NSS",
                            .base        = GPIOA,
                            .af          = 0,
                            .pin         = GPIO_PIN_15,
                            .pin_mode    = DevPinModeOutput,
                            .bit_value   = 1,  // Active low
                        },
                    .clk =
                        {
                            .device_name = "DACSgm3533_SCK",
                            .base        = GPIOC,
                            .af          = GPIO_AF_6,
                            .pin         = GPIO_PIN_10,
                            .pin_mode    = DevPinModeAF,
                        },
                    .mosi =
                        {
                            .device_name = "DACSgm3533_MOSI",
                            .base        = GPIOC,
                            .af          = GPIO_AF_6,
                            .pin         = GPIO_PIN_12,
                            .pin_mode    = DevPinModeAF,
                        },
                    .miso =
                        {
                            .device_name = "DACSgm3533_MISO",
                            .base        = GPIOC,
                            .af          = GPIO_AF_6,
                            .pin         = GPIO_PIN_11,
                            .pin_mode    = DevPinModeAF,
                        },
                    .spi_cfg =
                        {
                            .device_mode          = SPI_MASTER,
                            .trans_mode           = SPI_TRANSMODE_FULLDUPLEX,
                            .data_size            = SPI_DATASIZE_8BIT,
                            .clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE,
                            .nss                  = SPI_NSS_SOFT,
                            .prescale             = SPI_PSC_256,
                            .endian               = SPI_ENDIAN_MSB,
                        },
                    .dam_tx =
                        {
                            .base    = DMA0,
                            .channel = DMA_CH2,

                        }},

    },
    {
        .ch      = 1,
        .spi_cfg = {.base = SPI0,
                    .nss =
                        {
                            .device_name = "DACSgm3533_NSS",
                            .base        = GPIOG,
                            .af          = 0,
                            .pin         = GPIO_PIN_10,
                            .pin_mode    = DevPinModeOutput,
                            .bit_value   = 1,  // Active low
                        },
                    .clk =
                        {
                            .device_name = "DACSgm3533_SCK",
                            .base        = GPIOG,
                            .af          = GPIO_AF_5,
                            .pin         = GPIO_PIN_11,
                            .pin_mode    = DevPinModeAF,
                        },
                    .mosi =
                        {
                            .device_name = "DACSgm3533_MOSI",
                            .base        = GPIOD,
                            .af          = GPIO_AF_5,
                            .pin         = GPIO_PIN_7,
                            .pin_mode    = DevPinModeAF,
                        },
                    .miso =
                        {
                            .device_name = "DACSgm3533_MISO",
                            .base        = GPIOG,
                            .af          = GPIO_AF_5,
                            .pin         = GPIO_PIN_9,
                            .pin_mode    = DevPinModeAF,
                        },
                    .spi_cfg =
                        {
                            .device_mode          = SPI_MASTER,
                            .trans_mode           = SPI_TRANSMODE_FULLDUPLEX,
                            .data_size            = SPI_DATASIZE_8BIT,
                            .clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE,
                            .nss                  = SPI_NSS_SOFT,
                            .prescale             = SPI_PSC_256,
                            .endian               = SPI_ENDIAN_MSB,
                        },
                    .dam_tx =
                        {
                            .base    = DMA1,
                            .channel = DMA_CH2,

                        }},

    },
};

typedef struct {
    char device_name[DEVICE_NAME_MAX];
    uint32_t id;  // ID
    TypdefDACSgm3533BSPCfg cfg;
    uint16_t dac_data;  // DAC data

} TypdefDACSgm3533Status;
TypdefDACSgm3533Status DACSgm3533Status[DACSgm3533ChannelMax] = {0};

/* ===================================================================================== */

static const vfb_event_t DACSgm3533EventList[] = {

    DACSgm3533Start,
    DACSgm3533Stop,
    DACSgm3533Get,
    DACSgm3533Set,

};

static const VFBTaskStruct DACSgm3533_task_cfg = {
    .name         = "VFBTaskDACSgm3533",  // Task name
    .pvParameters = NULL,
    // .uxPriority = 10,
    // // Task parameters
    .queue_num  = 8,                    // Number of queues to subscribe
    .event_list = DACSgm3533EventList,  // Event list to subscribe
    .event_num =
        sizeof(DACSgm3533EventList) / sizeof(vfb_event_t),  // Number of events to subscribe
    .startup_wait_event_list = NULL,                        // Events to wait for at startup
    .startup_wait_event_num  = 0,                           // Number of startup events to wait for
    .xTicksToWait   = pdMS_TO_TICKS(CONFIG_DACSgm3533_CYCLE_TIMER_MS),  // Wait indefinitely
    .init_msg_cb    = __DACSgm3533InitHandle,  // Callback for initialization messages
    .rcv_msg_cb     = __DACSgm3533RcvHandle,   // Callback for received messages
    .rcv_timeout_cb = __DACSgm3533CycHandle,   // Callback for timeout
};

/* ===================================================================================== */

void DACSgm3533DeviceInit(void) {
    elog_i(TAG, "DACSgm3533DeviceInit");

    for (size_t i = 0; i < DACSgm3533ChannelMax; i++) {
        TypdefDACSgm3533Status *DACSgm3533StatusHandle = &DACSgm3533Status[i];
        DevSpiHandleStruct *spicfg = (DevSpiHandleStruct *)&DACSgm3533BspCfg[i].spi_cfg;
        DACSgm3533StatusHandle->id = i;
        memset(DACSgm3533StatusHandle->device_name, 0, sizeof(DACSgm3533StatusHandle->device_name));
        snprintf(DACSgm3533StatusHandle->device_name, sizeof(DACSgm3533StatusHandle->device_name),
                 "DACSgm3533%d", i);
        DACSgm3533StatusHandle->cfg = DACSgm3533BspCfg[i];  // Copy configuration

        DevSpiInit(spicfg);  // Initialize SPI

        DACSgm3533DSPISend(i, 0x0000);  // Send initial data to clear the buffer
        elog_i(TAG, "DACSgm3533[%d] device_name: %s, spi_base: 0x%08X", i,
               DACSgm3533StatusHandle->device_name, DACSgm3533StatusHandle->cfg.spi_cfg.base);
    }
}
SYSTEM_REGISTER_INIT(MCUInitStage, DACSgm3533Priority, DACSgm3533DeviceInit, DACSgm3533DeviceInit);

static void __DACSgm3533CreateTaskHandle(void) {
    for (size_t i = 0; i < DACSgm3533ChannelMax; i++) {
    }
    xTaskCreate(VFBTaskFrame, "VFBTaskDACSgm3533", configMINIMAL_STACK_SIZE * 2,
                (void *)&DACSgm3533_task_cfg, PriorityNormalEventGroup0, NULL);
}
SYSTEM_REGISTER_INIT(ServerInitStage, DACSgm3533Priority, __DACSgm3533CreateTaskHandle,
                     __DACSgm3533CreateTaskHandle init);

static void __DACSgm3533InitHandle(void *msg) {
    elog_i(TAG, "__DACSgm3533InitHandle");
    elog_set_filter_tag_lvl(TAG, DACSgm3533LogLvl);
    vfb_send(DACSgm3533Start, 0, NULL, 0);
}
// 接收消息的回调函数
static void __DACSgm3533RcvHandle(void *msg) {
    TaskHandle_t curTaskHandle = xTaskGetCurrentTaskHandle();
    // TypdefDACSgm3533Status *DACSgm3533StatusTmp = (TypdefDACSgm3533Status *)&DACSgm3533Status[0];
    char *taskName        = pcTaskGetName(curTaskHandle);
    vfb_message_t tmp_msg = (vfb_message_t)msg;
    switch (tmp_msg->frame->head.event) {
        case DACSgm3533Start: {
            elog_i(TAG, "DACSgm3533StartTask %d", tmp_msg->frame->head.data);
        } break;
        case DACSgm3533Set: {
            // Set DAC data for a specific channel
            uint8_t ch    = (uint8_t)(MSG_GET_DATA(tmp_msg) & 0xFF);
            uint16_t *hex = (uint16_t *)MSG_GET_PAYLOAD(tmp_msg);
            if (DACSgm3533Status[ch].dac_data != *hex) {
                DACSgm3533Status[ch].dac_data = *hex;  // Update the DAC data
                DACSgm3533DSPISend(ch, *hex);
                elog_d(TAG, "DACSgm3533Set: Channel %d, Data 0x%04X", ch, *hex);
            }

        } break;

        default:
            elog_e(TAG, "TASK %s RCV: unknown event: %d", taskName, tmp_msg->frame->head.event);
            break;
    }
}

static void __DACSgm3533CycHandle(void) {
    TypdefDACSgm3533Status *DACSgm3533StatusHandle = &DACSgm3533Status[0];
    if (DACSgm3533StatusHandle == NULL) {
        elog_e(TAG, "[ERROR]DACSgm3533StatusHandle NULL");
        return;
    }
    // DACSgm3533DSPISend(0, DACSgm3533Status[0].dac_data);
    // DACSgm3533DSPISend(1, DACSgm3533Status[1].dac_data);
}

#endif
void DACSgm3533DSPISend(uint8_t ch, uint16_t data) {
    // elog_i(TAG, "DACSgm3533DSPISend: ch=%d, data=0x%04X %d", ch, data, data);

    uint8_t i = ch;
    if (i >= DACSgm3533ChannelMax) {
        elog_e(TAG, "Invalid channel: %d", i);
        return;
    }
    // Prepare the data to send
    spi_send_buffer[i][0] = (data >> 8) & 0xFF;  // High byte
    spi_send_buffer[i][1] = data & 0xFF;         // Low byte
    DevSpiWriteRead(&DACSgm3533BspCfg[i].spi_cfg, &(spi_send_buffer[i][0]), NULL, ARRAYSIZE);
    // elog_i(TAG, "spi_send_buffer[%d][0]: 0x%02X, spi_send_buffer[%d][1]: 0x%02X", i,
    // spi_send_buffer[i][0], i, spi_send_buffer[i][1]);
    elog_d(TAG, "sgm3533 send data: 0x%04X to channel%d", data, ch);
    // elog_i(TAG, "Send done");
}

static void CmdDACSgm3533Help(void) {
    printf("Usage: sgm3533 <command>\r\n");
    printf("Commands:\r\n");
    printf("  help          Show this help message\r\n");
    printf("  data <ch> <data>  Set DAC data for channel <ch> in hex format (e.g., 0x1234)\r\n");
}
#define NEW_ARRAYSIZE 10
int send_n = 0, receive_n = 0;
uint8_t spi0_send_array[NEW_ARRAYSIZE]    = {0xA1, 0xA2, 0xA3, 0xA4, 0xA5,
                                             0xA6, 0xA7, 0xA8, 0xA9, 0xAA};
uint8_t spi0_receive_array[NEW_ARRAYSIZE] = {0xC1, 0xC2, 0xC3, 0xC4, 0xC5,
                                             0xC6, 0xC7, 0xC8, 0xC9, 0xCA};

static int CmdDACSgm3533Handle(int argc, char *argv[]) {
    if (argc < 2) {
        CmdDACSgm3533Help();
        return 0;
    }
    if (strcmp(argv[1], "help") == 0) {
        CmdDACSgm3533Help();
        return 0;
    }
    if (strcmp(argv[1], "test") == 0) {
        printf("Testing SPI send/receive\r\n");
        DevSpiWriteRead(&DACSgm3533BspCfg[0].spi_cfg, spi0_send_array, spi0_receive_array,
                        NEW_ARRAYSIZE);
        elog_hexdump(TAG, 16, spi0_send_array, NEW_ARRAYSIZE);
        elog_hexdump(TAG, 16, spi0_receive_array, NEW_ARRAYSIZE);
        return 0;
    }
    if (strcmp(argv[1], "data") == 0) {
        if (argc < 4) {
            elog_e(TAG, "Usage: sgm3533 data <ch> <data>");
            return 0;
        }
        printf("\r\nSetting DAC data for channel %s with data %s\r\n", argv[2], argv[3]);
        uint8_t ch    = atoi(argv[2]);
        uint16_t data = (uint16_t)strtol(argv[3], NULL, 16);  // Convert hex string to uint16_t
        if (ch >= DACSgm3533ChannelMax) {
            elog_e(TAG, "Invalid channel: %d", ch);
            return 0;
        }
        DACSgm3533DSPISend(ch, data);
        return 0;
    }
    CmdDACSgm3533Help();
    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), sgm3533,
                 CmdDACSgm3533Handle, DACSgm3533 command);