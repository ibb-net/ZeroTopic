
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
#include "dev_uart.h"
#include "elog.h"
#include "os_server.h"
// #include "app_event.h"

#define TAG              "DACSgm3533"
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
__IO uint8_t spi_send_buffer[DACSgm3533ChannelMax][ARRAYSIZE] = {
    {0xA5, 0xA5},  // Channel 0
    {0x5a, 0x5a},  // Channel 1
};
typedef struct
{
    uint32_t ch;
    DevPinHandleStruct nss_gpio_cfg;   // NSS GPIO configuration
    DevPinHandleStruct sck_gpio_cfg;   // SCK GPIO configuration
    DevPinHandleStruct mosi_gpio_cfg;  // MOSI GPIO configuration
    // DevPinHandleStruct miso_gpio_cfg;  // MISO GPIO configuration

    uint32_t spi_base;
    uint32_t spi_rcu;
    spi_idx_enum spi_idx;          // SPI index
    spi_parameter_struct spi_cfg;  // SPI configuration
    // DMA configuration
    uint32_t dma_base_addr;  // TX DMA base address
    uint32_t dma_channel;    // TX DMA channel

    dma_single_data_parameter_struct dma_fg;
} TypdefDACSgm3533BSPCfg;

static void __DACSgm3533CreateTaskHandle(void);
static void __DACSgm3533RcvHandle(void *msg);
static void __DACSgm3533CycHandle(void);
static void __DACSgm3533InitHandle(void *msg);
void DACSgm3533DSPISend(uint8_t ch, uint16_t data);
const TypdefDACSgm3533BSPCfg DACSgm3533BspCfg[DACSgm3533ChannelMax] = {
    /* SPI5  */
    {
        .ch           = 0,
        .nss_gpio_cfg = {
            .device_name = "DACSgm3533_NSS",
            .base        = GPIOA,
            .af          = 0,
            .pin         = GPIO_PIN_15,
            .pin_mode    = DevPinModeOutput,
            .bit_value   = 1,  // Active low
        },
        .sck_gpio_cfg = {
            .device_name = "DACSgm3533_SCK",
            .base        = GPIOC,
            .af          = GPIO_AF_6,
            .pin         = GPIO_PIN_10,
            .pin_mode    = DevPinModeAF,
        },
        .mosi_gpio_cfg = {
            .device_name = "DACSgm3533_MOSI",
            .base        = GPIOC,
            .af          = GPIO_AF_6,
            .pin         = GPIO_PIN_12,
            .pin_mode    = DevPinModeAF,
        },

        .spi_base = SPI2,
        .spi_rcu  = RCU_SPI2,
        .spi_idx  = IDX_SPI2,
        .spi_cfg  = {
             .device_mode          = SPI_MASTER,
             .trans_mode           = SPI_TRANSMODE_BDTRANSMIT,
             .data_size            = SPI_DATASIZE_8BIT,
             .clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE,
             .nss                  = SPI_NSS_SOFT,
             .prescale             = SPI_PSC_256,
             .endian               = SPI_ENDIAN_MSB,

        },
        .dma_base_addr = DMA0,
        .dma_channel   = DMA_CH2,
        .dma_fg        = {
                   .request             = DMA_REQUEST_SPI2_TX,
                   .direction           = DMA_MEMORY_TO_PERIPH,
                   .memory0_addr        = (uint32_t)spi_send_buffer[0],  // To be set later
                   .memory_inc          = DMA_MEMORY_INCREASE_ENABLE,
                   .periph_memory_width = DMA_PERIPH_WIDTH_8BIT,
                   .number              = ARRAYSIZE,  // Number of bytes to transfer
                   .periph_addr         = (uint32_t)(&SPI_TDATA(SPI2)),
                   .periph_inc          = DMA_PERIPH_INCREASE_DISABLE,
                   .priority            = DMA_PRIORITY_HIGH,
                   .circular_mode       = DMA_CIRCULAR_MODE_DISABLE,
        },

    },
    // spi0

    {
        .ch           = 1,
        .nss_gpio_cfg = {
            .device_name = "DACSgm3533_NSS", .base = GPIOG, .af = 0, .pin = GPIO_PIN_10, .pin_mode = DevPinModeOutput,
            .bit_value = 1,  // Active low
        },
        .sck_gpio_cfg = {
            .device_name = "DACSgm3533_SCK",
            .base        = GPIOG,
            .af          = GPIO_AF_6,
            .pin         = GPIO_PIN_11,
            .pin_mode    = DevPinModeAF,
        },
        .mosi_gpio_cfg = {
            .device_name = "DACSgm3533_MOSI",
            .base        = GPIOD,
            .af          = GPIO_AF_6,
            .pin         = GPIO_PIN_7,
            .pin_mode    = DevPinModeAF,
        },

        .spi_base = SPI0,
        .spi_rcu  = RCU_SPI0,
        .spi_idx  = IDX_SPI0,
        .spi_cfg  = {
             .device_mode          = SPI_MASTER,
             .trans_mode           = SPI_TRANSMODE_BDTRANSMIT,
             .data_size            = SPI_DATASIZE_8BIT,
             .clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE,
             .nss                  = SPI_NSS_SOFT,
             .prescale             = SPI_PSC_256,
             .endian               = SPI_ENDIAN_MSB,

        },
        .dma_base_addr = DMA1,
        .dma_channel   = DMA_CH2,
        .dma_fg        = {
                   .request             = DMA_REQUEST_SPI0_TX,
                   .direction           = DMA_MEMORY_TO_PERIPH,
                   .memory0_addr        = (uint32_t)spi_send_buffer[1],  // To be set later
                   .memory_inc          = DMA_MEMORY_INCREASE_ENABLE,
                   .periph_memory_width = DMA_PERIPH_WIDTH_8BIT,
                   .number              = ARRAYSIZE,  // Number of bytes to transfer
                   .periph_addr         = (uint32_t)(&SPI_TDATA(SPI0)),
                   .periph_inc          = DMA_PERIPH_INCREASE_DISABLE,
                   .priority            = DMA_PRIORITY_HIGH,
                   .circular_mode       = DMA_CIRCULAR_MODE_DISABLE,
        },
    },
};

typedef struct
{
    char device_name[DEVICE_NAME_MAX];
    uint32_t id;  // ID

} TypdefDACSgm3533Status;
TypdefDACSgm3533Status DACSgm3533Status[DACSgm3533ChannelMax] = {0};

/* ===================================================================================== */

static const vfb_event_t DACSgm3533EventList[] = {

    DACSgm3533Start,
    DACSgm3533Stop,
    DACSgm3533Get,

};

static const VFBTaskStruct DACSgm3533_task_cfg = {
    .name         = "VFBTaskDACSgm3533",  // Task name
    .pvParameters = NULL,
    // .uxPriority = 10,											  // Task parameters
    .queue_num               = 8,                                                  // Number of queues to subscribe
    .event_list              = DACSgm3533EventList,                                // Event list to subscribe
    .event_num               = sizeof(DACSgm3533EventList) / sizeof(vfb_event_t),  // Number of events to subscribe
    .startup_wait_event_list = NULL,                                               // Events to wait for at startup
    .startup_wait_event_num  = 0,                                                  // Number of startup events to wait for
    .xTicksToWait            = pdMS_TO_TICKS(CONFIG_DACSgm3533_CYCLE_TIMER_MS),    // Wait indefinitely
    .init_msg_cb             = __DACSgm3533InitHandle,                             // Callback for initialization messages
    .rcv_msg_cb              = __DACSgm3533RcvHandle,                              // Callback for received messages
    .rcv_timeout_cb          = __DACSgm3533CycHandle,                              // Callback for timeout
};

/* ===================================================================================== */

void DACSgm3533DeviceInit(void) {
    elog_i(TAG, "DACSgm3533DeviceInit");

    for (size_t i = 0; i < DACSgm3533ChannelMax; i++) {
        TypdefDACSgm3533Status *DACSgm3533StatusHandle = &DACSgm3533Status[i];

        DACSgm3533StatusHandle->id = i;
        memset(DACSgm3533StatusHandle->device_name, 0, sizeof(DACSgm3533StatusHandle->device_name));
        snprintf(DACSgm3533StatusHandle->device_name, sizeof(DACSgm3533StatusHandle->device_name), "DACSgm3533%d", i);

        // Initialize GPIOs
        DevPinHandleStruct *nss_gpio_cfg  = &DACSgm3533BspCfg[i].nss_gpio_cfg;
        DevPinHandleStruct *sck_gpio_cfg  = &DACSgm3533BspCfg[i].sck_gpio_cfg;
        DevPinHandleStruct *mosi_gpio_cfg = &DACSgm3533BspCfg[i].mosi_gpio_cfg;
        DevPinInit(nss_gpio_cfg);
        DevPinInit(sck_gpio_cfg);
        DevPinInit(mosi_gpio_cfg);
        /* RCU */
        rcu_periph_clock_enable(DACSgm3533BspCfg[i].spi_rcu);
        rcu_spi_clock_config(DACSgm3533BspCfg[i].spi_idx, RCU_SPISRC_PLL0Q);
        // Initialize SPI
        spi_i2s_deinit(DACSgm3533BspCfg[i].spi_base);
        spi_parameter_struct spi_init_struct;
        spi_parameter_struct *spi_cfg = &DACSgm3533BspCfg[i].spi_cfg;

        spi_init(DACSgm3533BspCfg[i].spi_base, spi_cfg);
        spi_byte_access_enable(DACSgm3533BspCfg[i].spi_base);
        spi_nss_output_enable(DACSgm3533BspCfg[i].spi_base);

        DACSgm3533DSPISend(i, 0xA5A6);  // Send initial data to clear the buffer
        elog_i(TAG, "DACSgm3533[%d] device_name: %s, spi_base: 0x%08X", i, DACSgm3533StatusHandle->device_name, DACSgm3533BspCfg[i].spi_base);
    }
}
SYSTEM_REGISTER_INIT(MCUInitStage, DACSgm3533Priority, DACSgm3533DeviceInit, DACSgm3533DeviceInit);

static void __DACSgm3533CreateTaskHandle(void) {
    for (size_t i = 0; i < DACSgm3533ChannelMax; i++) {
    }
    xTaskCreate(VFBTaskFrame, "VFBTaskDACSgm3533", configMINIMAL_STACK_SIZE, (void *)&DACSgm3533_task_cfg, PriorityNormalEventGroup0, NULL);
}
SYSTEM_REGISTER_INIT(ServerInitStage, DACSgm3533Priority, __DACSgm3533CreateTaskHandle, __DACSgm3533CreateTaskHandle init);

static void __DACSgm3533InitHandle(void *msg) {
    elog_i(TAG, "__DACSgm3533InitHandle");
    elog_set_filter_tag_lvl(TAG, DACSgm3533LogLvl);
    vfb_send(DACSgm3533Start, 0, NULL, 0);
}
// 接收消息的回调函数
static void __DACSgm3533RcvHandle(void *msg) {
    TaskHandle_t curTaskHandle                  = xTaskGetCurrentTaskHandle();
    TypdefDACSgm3533Status *DACSgm3533StatusTmp = &DACSgm3533Status[0];
    char *taskName                              = pcTaskGetName(curTaskHandle);
    vfb_message_t tmp_msg                       = (vfb_message_t)msg;
    switch (tmp_msg->frame->head.event) {
        case DACSgm3533Start: {
            elog_i(TAG, "DACSgm3533StartTask %d", tmp_msg->frame->head.data);
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
}

#endif
void DACSgm3533DSPISend(uint8_t ch, uint16_t data) {
    elog_i(TAG, "SendDACHex: ch=%d, data=0x%04X %d", ch, data, data);

    // Set NSS high

    spi_byte_access_enable(DACSgm3533BspCfg[ch].spi_base);  // Enable byte access for SPI
    spi_nss_output_enable(DACSgm3533BspCfg[ch].spi_base);   // Enable NSS output for SPI
    SCB_CleanDCache_by_Addr((uint32_t *)spi_send_buffer[ch], ARRAYSIZE);
    uint8_t i = ch;
    if (i >= DACSgm3533ChannelMax) {
        elog_e(TAG, "Invalid channel: %d", i);
        return;
    }
    DevPinHandleStruct *nss_gpio_cfg = &DACSgm3533BspCfg[i].nss_gpio_cfg;

    memset(spi_send_buffer[i], 0, sizeof(spi_send_buffer[i]));  // Clear the buffer
    memcpy(spi_send_buffer[i], &data, sizeof(data));            // Copy data to the buffer
    elog_i(TAG, "spi_send_buffer[%d]: 0x%02X 0x%02X", i, spi_send_buffer[i][0], spi_send_buffer[i][1]);
    spi_current_data_num_config(DACSgm3533BspCfg[i].spi_base, 0);  // Clear current data number
    DevPinWrite(nss_gpio_cfg, 1);
    DevPinWrite(nss_gpio_cfg, 0);   
    TypdefDACSgm3533Status *DACSgm3533StatusHandle = &DACSgm3533Status[i];
    dma_single_data_parameter_struct *dma_fg       = (dma_single_data_parameter_struct *)&DACSgm3533BspCfg[i].dma_fg;
    elog_i(TAG, "spi dam buffer 0x%02x 0x%02x", *((uint8_t *)dma_fg->memory0_addr),
           *((uint8_t *)dma_fg->memory0_addr + 1));
    dma_deinit(DACSgm3533BspCfg[i].dma_base_addr, DACSgm3533BspCfg[i].dma_channel);
   SCB_CleanDCache_by_Addr((uint32_t *)spi_send_buffer[ch], ARRAYSIZE);
    dma_single_data_mode_init(DACSgm3533BspCfg[i].dma_base_addr, DACSgm3533BspCfg[i].dma_channel, dma_fg);
    dma_channel_enable(DACSgm3533BspCfg[i].dma_base_addr, DACSgm3533BspCfg[i].dma_channel);  // Enable DMA channel

    // Set NSS high (inactive state)
    spi_enable(DACSgm3533BspCfg[i].spi_base);                                                                         // Enable SPI
    DevPinWrite(nss_gpio_cfg, 0);                                                                                     // Set NSS low (active state)
    spi_dma_enable(DACSgm3533BspCfg[i].spi_base, SPI_DMA_TRANSMIT);                                                   // Enable DMA for SPI transmit
    spi_master_transfer_start(DACSgm3533BspCfg[i].spi_base, SPI_TRANS_START);                                         // Start SPI transfer
    while (dma_flag_get(DACSgm3533BspCfg[i].dma_base_addr, DACSgm3533BspCfg[i].dma_channel, DMA_FLAG_FTF) == RESET);  // Wait for transfer complete
    dma_flag_clear(DACSgm3533BspCfg[i].dma_base_addr, DACSgm3533BspCfg[i].dma_channel, DMA_FLAG_FTF);

    // while (RESET == spi_i2s_flag_get(DACSgm3533BspCfg[i].spi_base, SPI_FLAG_ET));

    // spi_reload_data_num_config(DACSgm3533BspCfg[i].spi_base, ARRAYSIZE );
    spi_master_transfer_start(DACSgm3533BspCfg[i].spi_base, SPI_TRANS_START);  // Clear current data number

    while (RESET == spi_i2s_flag_get(DACSgm3533BspCfg[i].spi_base, SPI_STAT_TC));  // Wait until transmit buffer is empty
    spi_i2s_data_transmit(DACSgm3533BspCfg[i].spi_base, 0x12);                     // Transmit first byte
    while (RESET == spi_i2s_flag_get(DACSgm3533BspCfg[i].spi_base, SPI_FLAG_TP));  // Wait until transmit buffer is empty
    spi_i2s_data_transmit(DACSgm3533BspCfg[i].spi_base, 0x56);                     // Transmit second byte
    while (RESET == spi_i2s_flag_get(DACSgm3533BspCfg[i].spi_base, SPI_STAT_TC));

    DevPinWrite(nss_gpio_cfg, 1);
    elog_i(TAG, "DACSgm3533[%d] device_name: %s, spi_base: 0x%08X", i, DACSgm3533StatusHandle->device_name, DACSgm3533BspCfg[i].spi_base);
    elog_i(TAG, "send data: 0x%04X to channel %d", data, ch);
    elog_i(TAG, "Send done");
}
static void SendDACHex(uint8_t ch, uint16_t data) {
    DACSgm3533DSPISend(ch, data);
}
static void CmdDACSgm3533Help(void) {
    printf("Usage: sgm3533 <command>\r\n");
    printf("Commands:\r\n");
    printf("  help          Show this help message\r\n");
    printf("  data <ch> <data>  Set DAC data for channel <ch> in hex format (e.g., 0x1234)\r\n");
}

static int CmdDACSgm3533Handle(int argc, char *argv[]) {
    if (argc < 2) {
        CmdDACSgm3533Help();
        return 0;
    }
    if (strcmp(argv[1], "help") == 0) {
        CmdDACSgm3533Help();
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
        SendDACHex(ch, data);
        return 0;
    }
    CmdDACSgm3533Help();
    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), sgm3533, CmdDACSgm3533Handle, DACSgm3533 command);