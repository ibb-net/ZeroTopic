
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
#define CONFIG_sgm5860x_CYCLE_TIMER_MS 10
#endif
#define AVG_MAX_CNT (10)

const double f_gain_map[] = {
    [SGM58601_GAIN_1] = 1.0f,   [SGM58601_GAIN_2] = 2.0f,     [SGM58601_GAIN_4] = 4.0f,
    [SGM58601_GAIN_8] = 8.0f,   [SGM58601_GAIN_16] = 16.0f,   [SGM58601_GAIN_32] = 32.0f,
    [SGM58601_GAIN_64] = 64.0f, [SGM58601_GAIN_128] = 128.0f,
};
typedef struct {
    uint8_t channel;  // Channel number
    uint8_t gain;     // Gain setting for the channel
} ChannelCfg_t;
ChannelCfg_t sgm5860_channelcfg[sgm5860xChannelMax] = {
    {0, SGM58601_GAIN_1},    // Channel 0: Gain 1
    {2, SGM58601_GAIN_1},    // Channel 2: Gain 1
    {4, SGM58601_GAIN_1},    // Channel 4: Gain 1
    {6, SGM58601_GAIN_128},  // Channel 6: Gain 128

};
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
    uint8_t status;                          // Status of the device
    uint8_t scan_index;                      // Channel to scan
    double last_voltage[sgm5860xChannelMax];  // Last voltage read from each channel
    double sum[sgm5860xChannelMax];
    double average[sgm5860xChannelMax];                   // Average voltage for each channel
    uint8_t vol_index[sgm5860xChannelMax];               // Voltage index for each channel
    double tmp_voltage[sgm5860xChannelMax][AVG_MAX_CNT];  // Temporary voltage storage

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
    xTaskCreate(VFBTaskFrame, "VFBTasksgm5860x", configMINIMAL_STACK_SIZE * 2,
                (void *)&sgm5860x_task_cfg, PriorityNormalEventGroup0, NULL);
}
SYSTEM_REGISTER_INIT(BoardInitStage, sgm5860xPriority, __sgm5860xCreateTaskHandle,
                     __sgm5860xCreateTaskHandle init);

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
    char *taskName             = pcTaskGetName(curTaskHandle);
    vfb_message_t tmp_msg      = (vfb_message_t)msg;
    switch (tmp_msg->frame->head.event) {
        case sgm5860xStart: {
            elog_i(TAG, "sgm5860xStartTask %d", tmp_msg->frame->head.data);
            DevSgm5860xConfig(&sgm5860_cfg);  // Configure the SGM5860x device
            sgm5860xStatus.status = 1;        // Set status to indicate the device is started

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
    if (sgm5860xStatus.status == 1) {
        // sgm5860xStatus.scan_index
        double last_voltage   = 0;
        uint8_t last_channel = 0;
        uint8_t last_index   = 0;
        double last_gain      = 0;
        uint8_t channel =
            sgm5860_channelcfg[sgm5860xStatus.scan_index].channel;  // Get the current channel
        uint8_t gain = sgm5860_channelcfg[sgm5860xStatus.scan_index]
                           .gain;  // Get the gain for the current channel
        DevGetADCData(&sgm5860_cfg, &last_voltage, &last_channel, channel, gain);

        for (int i = 0; i < sizeof(sgm5860_channelcfg) / sizeof(sgm5860_channelcfg[0]); i++) {
            if (sgm5860_channelcfg[i].channel == last_channel) {
                last_index = i;
                last_gain =
                    f_gain_map[sgm5860_channelcfg[i].gain];  // Get the gain for the last channel
                sgm5860xStatus.last_voltage[i] = last_voltage;
                sgm5860xStatus.sum[i] += last_voltage;  // Accumulate the voltage for averaging
                sgm5860xStatus.tmp_voltage[i][sgm5860xStatus.vol_index[i]] = last_voltage;

                sgm5860xStatus.vol_index[i]++;
                if (sgm5860xStatus.vol_index[i] >= AVG_MAX_CNT) {
                    double max1   = sgm5860xStatus.tmp_voltage[i][0],
                          max2   = sgm5860xStatus.tmp_voltage[i][0];
                    double min1   = sgm5860xStatus.tmp_voltage[i][0],
                          min2   = sgm5860xStatus.tmp_voltage[i][0];
                    int max1_idx = 0, max2_idx = 0, min1_idx = 0, min2_idx = 0;

                    // 找到最大和次大，最小和次小的索引
                    for (int j = 1; j < AVG_MAX_CNT; j++) {
                        double v = sgm5860xStatus.tmp_voltage[i][j];
                        if (v > max1) {
                            max2     = max1;
                            max2_idx = max1_idx;
                            max1     = v;
                            max1_idx = j;
                        } else if (v > max2 || max2_idx == max1_idx) {
                            max2     = v;
                            max2_idx = j;
                        }
                        if (v < min1) {
                            min2     = min1;
                            min2_idx = min1_idx;
                            min1     = v;
                            min1_idx = j;
                        } else if (v < min2 || min2_idx == min1_idx) {
                            min2     = v;
                            min2_idx = j;
                        }
                    }

                    // 计算剩余6个的平均值
                    double sum = 0.0f;
                    int cnt   = 0;
                    for (int j = 0; j < AVG_MAX_CNT; j++) {
                        if (j != max1_idx && j != max2_idx && j != min1_idx && j != min2_idx) {
                            sum += sgm5860xStatus.tmp_voltage[i][j];
                            cnt++;
                        }
                    }
                    if (cnt > 0) {
                        sgm5860xStatus.average[i] = sum / cnt;
                    } else {
                        sgm5860xStatus.average[i] = 0.0f;
                    }
                    elog_d(TAG,
                           "Channel %d cnt %d ,tmp_voltage: %.7f, %.7f, %.7f, %.7f, %.7f, %.7f, "
                           "%.7f, %.7f",
                           i, cnt, sgm5860xStatus.tmp_voltage[i][0], sgm5860xStatus.tmp_voltage[i][1],
                           sgm5860xStatus.tmp_voltage[i][2], sgm5860xStatus.tmp_voltage[i][3],
                           sgm5860xStatus.tmp_voltage[i][4], sgm5860xStatus.tmp_voltage[i][5],
                           sgm5860xStatus.tmp_voltage[i][6], sgm5860xStatus.tmp_voltage[i][7]);
                    // sgm5860xStatus.average[i] = sgm5860xStatus.sum[i] /
                    // sgm5860xStatus.vol_index[i];
                    sgm5860xStatus.vol_index[i] = 0;  // Reset the index after averaging
                    sgm5860xStatus.sum[i]       = 0;  // Reset the sum after averaging
                    vfb_send(sgm5860xCH1 + last_index, 0, &(sgm5860xStatus.average[i]),
                             sizeof(last_voltage));
                    elog_d(TAG, " Index %d Channel %d, Gain %.0f, Voltage: %.7f V", last_index,
                           last_channel, last_gain, sgm5860xStatus.average[i]);
                }
                break;
            }
        }

        sgm5860xStatus.scan_index++;
        if (sgm5860xStatus.scan_index >
            sizeof(sgm5860_channelcfg) / sizeof(sgm5860_channelcfg[0]) - 1) {
            sgm5860xStatus.scan_index = 0;  // Reset channel to 0 after reaching the last channel
        }
    } else {
        // elog_e(TAG, "[ERROR]sgm5860xCycHandle: Device not started");
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
    printf("  data          Read data from the SGM5860x device\r\n");
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
        elog_i(TAG, "SGM5860x data readout\r\n");
        for (int i = 0; i < sgm5860xChannelMax; i++) {
            elog_i(TAG, "Index %d Channel %d: Voltage = %.7f V %.4f mV, %.1f uV", i,
                   sgm5860_channelcfg[i].channel, sgm5860xStatus.average[i],
                   sgm5860xStatus.average[i] * 1000.0f, sgm5860xStatus.average[i] * 1000000.0f);
        }
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
