
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
#include "dev_sgm5860x.h"
#include "dev_spi.h"
#include "dev_uart.h"
#include "elog.h"
#include "os_server.h"
// #include "app_event.h"

#define TAG            "sgm5860x"
#define sgm5860xLogLvl ELOG_LVL_INFO

#define sgm5860xPriority PriorityNormalEventGroup0
#ifndef sgm5860xChannelMax
#define sgm5860xChannelMax 4
#endif
#ifndef CONFIG_sgm5860x_CYCLE_TIMER_MS
#define CONFIG_sgm5860x_CYCLE_TIMER_MS 100
#endif

#define SPI_CS1_ENABLE  DevPinWrite(&sgm5860xBspCfg.spi_cfg.nss, 0)  // Set NEST pin low
#define SPI_CS1_DISABLE DevPinWrite(&sgm5860xBspCfg.spi_cfg.nss, 1)  // Set NEST pin high
#define SGM58601_DRDY   DevPinRead(&sgm5860xBspCfg.drdy)             // Read DRDY pin status

/* ===================================================================================== */
const DevSgm5860xHandleStruct sgm5860_cfg = {
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
    .spi =
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
    DevSgm5860xHandleStruct *cfg;

} Typdefsgm5860xStatus;
Typdefsgm5860xStatus sgm5860xStatus = {0};

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

    DevSgm5860xHandleStruct *sgm5860xBspCfg = (DevSgm5860xHandleStruct *)&sgm5860_cfg;
    sgm5860xStatus.cfg                      = sgm5860xBspCfg;  // Assign the BSP configuration
    strcpy(sgm5860xStatus.device_name, "sgm58601");
    DevSgm5860xInit(sgm5860xBspCfg);  // Initialize the SGM5860x device
    elog_i(TAG, "sgm5860xDeviceInit completed");
}
SYSTEM_REGISTER_INIT(MCUInitStage, sgm5860xPriority, sgm5860xDeviceInit, sgm5860xDeviceInit);

static void __sgm5860xCreateTaskHandle(void) {
    elog_i(TAG, "__sgm5860xCreateTaskHandle");
    xTaskCreate(VFBTaskFrame, "VFBTasksgm5860x", configMINIMAL_STACK_SIZE * 2, (void *)&sgm5860x_task_cfg,
                PriorityNormalEventGroup0, NULL);
}
SYSTEM_REGISTER_INIT(BoardInitStage, sgm5860xPriority, __sgm5860xCreateTaskHandle,__sgm5860xCreateTaskHandle init);

static void __sgm5860xInitHandle(void *msg) {
    elog_i(TAG, "__sgm5860xInitHandle");
    elog_set_filter_tag_lvl(TAG, sgm5860xLogLvl);
    vTaskDelay(pdMS_TO_TICKS(1));  // Delay to ensure the system is ready
    // Initialize the SGM5860x device
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
             DevSgm5860xConfig(&sgm5860_cfg);  // Configure the SGM5860x device

        } break;
        case sgm5860xSet: {
            elog_i(TAG, "sgm5860xSetTask %d", tmp_msg->frame->head.data);
        } break;
        default:
            elog_e(TAG, "TASK %s RCV: unknown event: %d", taskName, tmp_msg->frame->head.event);
            break;
    }
}

static void __sgm5860xCycHandle(void) {
    Typdefsgm5860xStatus *sgm5860xStatusHandle = &sgm5860xStatus;
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
    printf("  config        Configure the SGM5860x device\r\n");
    printf("  reset         Reset the SGM5860x device\r\n");
    printf("  init          Initialize the SGM5860x device\r\n");
}

static int Cmdsgm5860xHandle(int argc, char *argv[]) {
    if (argc < 2) {
        Cmdsgm5860xHelp();
        return 0;
    }
    if (strcmp(argv[1], "help") == 0) {
        Cmdsgm5860xHelp();
        return 0;
    }
    if (strcmp(argv[1], "config") == 0) {
        printf("Testing SPI send/receive\r\n");
        DevSgm5860xConfig(&sgm5860_cfg);  // Configure the SGM5860x device
        return 0;
    }
    if (strcmp(argv[1], "data") == 0) {
        int32_t adc_data = 0;
        uint8_t channel = 0;
        if (argc < 3) {
            elog_e(TAG, "Usage: sgm5860x data <channel>");
            return 0;
        }
        channel = (uint8_t)strtol(argv[2], NULL, 10);

        printf("Testing ADC data read\r\n");
        DevGetADCData(&sgm5860_cfg, &adc_data,channel);
        return 0;
    }
    // reset
    if (strcmp(argv[1], "reset") == 0) {
        DevSgm5860xReset(&sgm5860_cfg);  // Reset the SGM5860x device
        elog_i(TAG, "SGM5860x reset completed");
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

    Cmdsgm5860xHelp();
    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), sgm5860x,
                 Cmdsgm5860xHandle, sgm5860x command);
