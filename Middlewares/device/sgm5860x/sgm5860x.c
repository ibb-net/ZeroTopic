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

#define TAG                           "sgm5860x"
#define sgm5860xLogLvl                ELOG_LVL_INFO
#define SGM58601_GAIN_128_MEASURE_MAX (0.030)  // 30mv
#define sgm5860xPriority              PriorityNormalEventGroup0
#ifndef sgm5860xChannelMax
#define sgm5860xChannelMax 4
#endif
#ifndef CONFIG_sgm5860x_CYCLE_TIMER_MS
#define CONFIG_sgm5860x_CYCLE_TIMER_MS 10
#endif
#define KALMAN_FILTER_MODE (1)  // 0: No filter, 1: Kalman filter
#define AVG_FILTER_MODE    (0)  // 0: No filter, 1: Average filter
#define FILTER_MODE        (KALMAN_FILTER_MODE)
#if FILTER_MODE == KALMAN_FILTER_MODE
#include "kalman_filter.h"
#elif FILTER_MODE == AVG_FILTER_MODE
#define AVG_MAX_CNT (10)
#else
#error \
    "Invalid filter mode selected. Please set FILTER_MODE to either KALMAN_FILTER_MODE or AVG_FILTER_MODE"
#endif

const double f_gain_map[] = {
    [SGM58601_GAIN_1] = 1.0,   [SGM58601_GAIN_2] = 2.0,     [SGM58601_GAIN_4] = 4.0,
    [SGM58601_GAIN_8] = 8.0,   [SGM58601_GAIN_16] = 16.0,   [SGM58601_GAIN_32] = 32.0,
    [SGM58601_GAIN_64] = 64.0, [SGM58601_GAIN_128] = 128.0,
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
static void sgm5860x_check_kalman_health(uint8_t channel_index);
static void sgm5860x_kalman_filter_init(uint8_t channel_index);
static void sgm5860x_print_kalman_stats(uint8_t channel_index);
int sgm5860xReadSingleData(unsigned char channel);

typedef struct {
    char device_name[DEVICE_NAME_MAX];
    uint32_t id;  // ID
    DevSgm5860xHandleStruct *cfg;
    uint8_t status;                           // Status of the device
    uint8_t scan_index;                       // Channel to scan
    double last_voltage[sgm5860xChannelMax];  // Last voltage read from each channel
    double sum[sgm5860xChannelMax];
    double average[sgm5860xChannelMax];     // Average voltage for each channel
    uint8_t vol_index[sgm5860xChannelMax];  // Voltage index for each channel
#if FILTER_MODE == KALMAN_FILTER_MODE
    SignalState_t last_kalman_state[sgm5860xChannelMax];
    AdaptiveKalmanFilter_t kalman_filters[sgm5860xChannelMax];  // Kalman filters for each channel
    uint8_t kalman_initialized[sgm5860xChannelMax];  // Kalman filter initialization status
    double filtered_voltage[sgm5860xChannelMax];     // Filtered voltage for each channel
    uint32_t sample_count[sgm5860xChannelMax];       // Sample count for each channel
#elif FILTER_MODE == AVG_FILTER_MODE
    double tmp_voltage[sgm5860xChannelMax][AVG_MAX_CNT];  // Temporary voltage storage
#endif

} Typdefsgm5860xStatus;
volatile Typdefsgm5860xStatus sgm5860xStatus = {0};

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
    strcpy((char *)sgm5860xStatus.device_name, "sgm58601");

    // 初始化所有通道的滤波器状态
    for (int i = 0; i < sgm5860xChannelMax; i++) {
        sgm5860xStatus.last_voltage[i] = 0.0;
        sgm5860xStatus.sum[i]          = 0.0;
        sgm5860xStatus.average[i]      = 0.0;
        sgm5860xStatus.vol_index[i]    = 0;
#if FILTER_MODE == KALMAN_FILTER_MODE
        sgm5860xStatus.kalman_initialized[i] = 0;
        sgm5860xStatus.filtered_voltage[i]   = 0.0;
        sgm5860xStatus.sample_count[i]       = 0;
#endif
    }

    DevSgm5860xInit(sgm5860xBspCfg);  // Initialize the SGM5860x device
    elog_i(TAG, "sgm5860xDeviceInit completed");
    elog_i(TAG, "sgm5860xStatus.device_name: %s", sgm5860xStatus.device_name);
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
    // elog_set_filter_tag_lvl(TAG, sgm5860xLogLvl);
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

#if FILTER_MODE == KALMAN_FILTER_MODE
            // 初始化所有通道的卡尔曼滤波器
            for (int i = 0; i < sgm5860xChannelMax; i++) {
                sgm5860x_kalman_filter_init(i);
            }
            elog_i(TAG, "All Kalman filters initialized");
#endif
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
    Typdefsgm5860xStatus *sgm5860xStatusHandle = (Typdefsgm5860xStatus *)&sgm5860xStatus;
    if (sgm5860xStatusHandle == NULL) {
        elog_e(TAG, "[ERROR]sgm5860xStatusHandle NULL");
        return;
    }
    if (sgm5860xStatus.status == 1) {
        // sgm5860xStatus.scan_index
        double last_voltage           = 0;
        uint8_t last_channel          = 0;
        uint8_t last_index            = 0;
        double last_gain              = 0;
        static uint8_t channel_6_gain = 0;
        (void *)&channel_6_gain;
        uint8_t channel =
            sgm5860_channelcfg[sgm5860xStatus.scan_index].channel;  // Get the current channel
        uint8_t gain = sgm5860_channelcfg[sgm5860xStatus.scan_index].gain;
        DevGetADCData(&sgm5860_cfg, &last_voltage, &last_channel, channel, gain);
        for (int i = 0; i < sizeof(sgm5860_channelcfg) / sizeof(sgm5860_channelcfg[0]); i++) {
            if (sgm5860_channelcfg[i].channel == last_channel) {
                last_index = i;
                last_gain =
                    f_gain_map[sgm5860_channelcfg[i].gain];  // Get the gain for the last channel
                sgm5860xStatus.last_voltage[i] = last_voltage;
                sgm5860xStatus.sum[i] += last_voltage;  // Accumulate the voltage for averaging

#if FILTER_MODE == KALMAN_FILTER_MODE
                // 使用自适应卡尔曼滤波器处理数据
                if (sgm5860xStatus.kalman_initialized[i]) {
                    double tmp_voltage = last_voltage;
                    // 更新卡尔曼滤波器
                    if (last_channel == 6) {
                        tmp_voltage = last_voltage * 250.0;
                    }
                    sgm5860xStatus.filtered_voltage[i] =
                        adaptive_kalman_update(&sgm5860xStatus.kalman_filters[i], tmp_voltage);
                    if (last_channel == 6) {
                        sgm5860xStatus.filtered_voltage[i] =
                            sgm5860xStatus.filtered_voltage[i] / 250.0;
                    }
                    sgm5860xStatus.sample_count[i]++;

                    // 每个采样都发送滤波后的数据
                    // SignalState_t signal_state =
                    // adaptive_kalman_query_signal_state(&sgm5860xStatus.kalman_filters[i]);
                    if (sgm5860xStatus.last_kalman_state[i] !=
                        sgm5860xStatus.kalman_filters[i].signal_state) {
                        sgm5860xStatus.last_kalman_state[i] =
                            sgm5860xStatus.kalman_filters[i].signal_state;
                        elog_i(TAG, "Channel %d state changed to %d", i,
                               sgm5860xStatus.kalman_filters[i].signal_state);
                        elog_i(TAG, "Index %d Channel %d, Gain %.0f, Raw: %.7f V, Filtered: %.7f V",
                               last_index, last_channel, last_gain, last_voltage,
                               sgm5860xStatus.filtered_voltage[i]);
                        vfb_send(sgm5860xCH1 + last_index, 0, &(sgm5860xStatus.filtered_voltage[i]),
                                 sizeof(sgm5860xStatus.filtered_voltage[i]));
                    }
                    else
                    {
                        if(sgm5860xStatus.sample_count[i]%10 == 0)
                        elog_d(TAG, "Channel %d state unchanged %d ", i,
                               sgm5860xStatus.kalman_filters[i].signal_state);
                                 vfb_send(sgm5860xCH1 + last_index, 0, &(sgm5860xStatus.filtered_voltage[i]),
                                 sizeof(sgm5860xStatus.filtered_voltage[i]));
                    }

                    // 每100个样本检查一次滤波器健康状态
                    if (sgm5860xStatus.sample_count[i] % 100 == 0) {
                        sgm5860x_check_kalman_health(i);
                    }

                    // 每1000个样本打印一次统计信息
                    if (sgm5860xStatus.sample_count[i] % 1000 == 0) {
                        sgm5860x_print_kalman_stats(i);
                    }
                } else {
                    // 如果滤波器未初始化，使用原始值
                    sgm5860xStatus.filtered_voltage[i] = last_voltage;
                    elog_w(TAG, "Kalman filter not initialized for channel %d", i);
                }
#elif FILTER_MODE == AVG_FILTER_MODE
                sgm5860xStatus.tmp_voltage[i][sgm5860xStatus.vol_index[i]] = last_voltage;

                sgm5860xStatus.vol_index[i]++;
                if (sgm5860xStatus.vol_index[i] >= AVG_MAX_CNT) {
                    double max1  = sgm5860xStatus.tmp_voltage[i][0],
                           max2  = sgm5860xStatus.tmp_voltage[i][0];
                    double min1  = sgm5860xStatus.tmp_voltage[i][0],
                           min2  = sgm5860xStatus.tmp_voltage[i][0];
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
                    double sum = 0.0;
                    int cnt    = 0;
                    for (int j = 0; j < AVG_MAX_CNT; j++) {
                        if (j != max1_idx && j != max2_idx && j != min1_idx && j != min2_idx) {
                            sum += sgm5860xStatus.tmp_voltage[i][j];
                            cnt++;
                        }
                    }
                    if (cnt > 0) {
                        sgm5860xStatus.average[i] = sum / (double)cnt;
                    } else {
                        sgm5860xStatus.average[i] = 0.0;
                    }
                    elog_d(TAG,
                           "Channel %d cnt %d ,tmp_voltage: %.7f, %.7f, %.7f, %.7f, %.7f, %.7f, "
                           "%.7f, %.7f",
                           i, cnt, sgm5860xStatus.tmp_voltage[i][0],
                           sgm5860xStatus.tmp_voltage[i][1], sgm5860xStatus.tmp_voltage[i][2],
                           sgm5860xStatus.tmp_voltage[i][3], sgm5860xStatus.tmp_voltage[i][4],
                           sgm5860xStatus.tmp_voltage[i][5], sgm5860xStatus.tmp_voltage[i][6],
                           sgm5860xStatus.tmp_voltage[i][7]);
                    sgm5860xStatus.vol_index[i] = 0;    // Reset the index after averaging
                    sgm5860xStatus.sum[i]       = 0.0;  // Reset the sum after averaging
                    vfb_send(sgm5860xCH1 + last_index, 0, &(sgm5860xStatus.average[i]),
                             sizeof(last_voltage));
                    elog_d(TAG, " Index %d Channel %d, Gain %.0f, Voltage: %.7f V", last_index,
                           last_channel, last_gain, sgm5860xStatus.average[i]);
                }
#endif

                break;
            }
        }
#if 0  // 自动放大倍数
        static uint8_t channel_6_gain = 0;
        for (int i = 3; i < sizeof(sgm5860_channelcfg) / sizeof(sgm5860_channelcfg[0]); i++) {
            if ((sgm5860_channelcfg[i].channel == 6) && (last_channel == 6)) {
                // printf("this2");
                uint8_t tmp_last_channel = sgm5860_channelcfg[i].channel;
                elog_d(TAG, "Index %d Last Channel %d Voltage %.7f V\r\n ", i, last_channel,
                       last_voltage);
                if (last_voltage >= SGM58601_GAIN_128_MEASURE_MAX) {
                    sgm5860_channelcfg[i].gain = SGM58601_GAIN_64;
                    if (channel_6_gain != SGM58601_GAIN_64) {
                        elog_i(
                            TAG,
                            "Index %d Channel %d Gain changed to SGM58601_GAIN_64, Voltage: %.7f V",
                            i, tmp_last_channel, last_voltage);
                        channel_6_gain = SGM58601_GAIN_64;
                    }
                } else {
                    sgm5860_channelcfg[i].gain = SGM58601_GAIN_128;
                    if (channel_6_gain != SGM58601_GAIN_128) {
                        elog_i(
                            TAG,
                            "Index %d Channel 6 Gain changed to SGM58601_GAIN_128, Voltage: %.7f V",
                            i, last_voltage);
                        channel_6_gain = SGM58601_GAIN_128;
                    }
                }
                // printf("this3\r\n");
                break;
            }
        }
#endif

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
    printf("  read <addr>   Read register from SGM5860x device\r\n");
#if FILTER_MODE == KALMAN_FILTER_MODE
    printf("  kalman        Kalman filter control commands\r\n");
    printf("    status [ch] - Show filter status\r\n");
    printf("    stats [ch]  - Show filter statistics\r\n");
    printf("    reset [ch]  - Reset filter\r\n");
    printf("    tune <ch> <resp> <stab> - Tune filter parameters\r\n");
    printf("    check [ch]  - Check filter health\r\n");
#endif
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
#if FILTER_MODE == KALMAN_FILTER_MODE
            elog_i(TAG, "Index %d Channel %d: Raw=%.7f V, Filtered=%.7f V (%.4f mV, %.1f uV)", i,
                   sgm5860_channelcfg[i].channel, sgm5860xStatus.last_voltage[i],
                   sgm5860xStatus.filtered_voltage[i], sgm5860xStatus.filtered_voltage[i] * 1000.0,
                   sgm5860xStatus.filtered_voltage[i] * 1000000.0);
#elif FILTER_MODE == AVG_FILTER_MODE
            elog_i(TAG, "Index %d Channel %d: Voltage = %.7f V %.4f mV, %.1f uV", i,
                   sgm5860_channelcfg[i].channel, sgm5860xStatus.average[i],
                   sgm5860xStatus.average[i] * 1000.0, sgm5860xStatus.average[i] * 1000000.0);
#endif
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

#if FILTER_MODE == KALMAN_FILTER_MODE
    // kalman
    if (strcmp(argv[1], "kalman") == 0) {
        if (argc < 3) {
            printf("Usage: sgm5860x kalman <command> [args]\r\n");
            printf("Commands:\r\n");
            printf("  status [channel] - Show Kalman filter status\r\n");
            printf("  stats [channel]  - Show Kalman filter statistics\r\n");
            printf("  reset [channel]  - Reset Kalman filter\r\n");
            printf("  tune <channel> <responsiveness> <stability> - Tune filter parameters\r\n");
            printf("  check [channel]  - Check filter health\r\n");
            return 0;
        }

        if (strcmp(argv[2], "status") == 0) {
            if (argc >= 4) {
                int channel = atoi(argv[3]);
                if (channel >= 0 && channel < sgm5860xChannelMax) {
                    sgm5860x_print_kalman_stats(channel);
                } else {
                    printf("Invalid channel number. Range: 0-%d\r\n", sgm5860xChannelMax - 1);
                }
            } else {
                // 显示所有通道的状态
                for (int i = 0; i < sgm5860xChannelMax; i++) {
                    sgm5860x_print_kalman_stats(i);
                }
            }
            return 0;
        }

        if (strcmp(argv[2], "stats") == 0) {
            if (argc >= 4) {
                int channel = atoi(argv[3]);
                if (channel >= 0 && channel < sgm5860xChannelMax) {
                    if (sgm5860xStatus.kalman_initialized[channel]) {
                        AdaptiveKalmanPerformance_t perf;
                        adaptive_kalman_get_performance_stats(
                            &sgm5860xStatus.kalman_filters[channel], &perf);

                        printf("Channel %d Performance Statistics:\r\n", channel);
                        printf("  Tracking Accuracy: %.4f\r\n", perf.tracking_accuracy);
                        printf("  Filter Stability: %.4f\r\n", perf.filter_stability);
                        printf("  Convergence Speed: %.4f\r\n", perf.convergence_speed);
                        printf("  Q Drift: %.4f%%\r\n", perf.Q_drift * 100);
                        printf("  R Drift: %.4f%%\r\n", perf.R_drift * 100);
                        printf("  Total Adaptations: %d\r\n", perf.total_adaptations);
                        printf("  State Changes: %d\r\n", perf.state_changes);
                        printf("  Samples: %d\r\n", (int)sgm5860xStatus.sample_count[channel]);
                    } else {
                        printf("Kalman filter not initialized for channel %d\r\n", channel);
                    }
                } else {
                    printf("Invalid channel number. Range: 0-%d\r\n", sgm5860xChannelMax - 1);
                }
            } else {
                printf("Usage: sgm5860x kalman stats <channel>\r\n");
            }
            return 0;
        }

        if (strcmp(argv[2], "reset") == 0) {
            if (argc >= 4) {
                int channel = atoi(argv[3]);
                if (channel >= 0 && channel < sgm5860xChannelMax) {
                    if (sgm5860xStatus.kalman_initialized[channel]) {
                        double current_value = sgm5860xStatus.filtered_voltage[channel];
                        adaptive_kalman_reset(&sgm5860xStatus.kalman_filters[channel],
                                              current_value);
                        sgm5860xStatus.sample_count[channel] = 0;
                        printf("Kalman filter reset for channel %d to value %.6f\r\n", channel,
                               current_value);
                    } else {
                        printf("Kalman filter not initialized for channel %d\r\n", channel);
                    }
                } else {
                    printf("Invalid channel number. Range: 0-%d\r\n", sgm5860xChannelMax - 1);
                }
            } else {
                // 重置所有通道
                for (int i = 0; i < sgm5860xChannelMax; i++) {
                    if (sgm5860xStatus.kalman_initialized[i]) {
                        double current_value = sgm5860xStatus.filtered_voltage[i];
                        adaptive_kalman_reset(&sgm5860xStatus.kalman_filters[i], current_value);
                        sgm5860xStatus.sample_count[i] = 0;
                        printf("Kalman filter reset for channel %d to value %.6f\r\n", i,
                               current_value);
                    }
                }
            }
            return 0;
        }

        if (strcmp(argv[2], "tune") == 0) {
            if (argc >= 6) {
                int channel           = atoi(argv[3]);
                double responsiveness = atof(argv[4]);
                double stability      = atof(argv[5]);

                if (channel >= 0 && channel < sgm5860xChannelMax) {
                    if (sgm5860xStatus.kalman_initialized[channel]) {
                        adaptive_kalman_tune_parameters(&sgm5860xStatus.kalman_filters[channel],
                                                        responsiveness, stability);
                        printf(
                            "Kalman filter tuned for channel %d: responsiveness=%.2f, "
                            "stability=%.2f\r\n",
                            channel, responsiveness, stability);
                    } else {
                        printf("Kalman filter not initialized for channel %d\r\n", channel);
                    }
                } else {
                    printf("Invalid channel number. Range: 0-%d\r\n", sgm5860xChannelMax - 1);
                }
            } else {
                printf("Usage: sgm5860x kalman tune <channel> <responsiveness> <stability>\r\n");
                printf("  responsiveness: 0.1-1.0 (higher = more responsive)\r\n");
                printf("  stability: 0.1-1.0 (higher = more stable)\r\n");
            }
            return 0;
        }

        if (strcmp(argv[2], "check") == 0) {
            if (argc >= 4) {
                int channel = atoi(argv[3]);
                if (channel >= 0 && channel < sgm5860xChannelMax) {
                    if (sgm5860xStatus.kalman_initialized[channel]) {
                        int check_result =
                            adaptive_kalman_self_check(&sgm5860xStatus.kalman_filters[channel]);
                        printf("Channel %d health check: ", channel);
                        if (check_result == KALMAN_CHECK_OK) {
                            printf("OK\r\n");
                        } else {
                            printf("ERROR 0x%02X", check_result);
                            if (check_result & KALMAN_CHECK_PARAM_ERROR) printf(" PARAM");
                            if (check_result & KALMAN_CHECK_BOUNDS_ERROR) printf(" BOUNDS");
                            if (check_result & KALMAN_CHECK_UNSTABLE) printf(" UNSTABLE");
                            if (check_result & KALMAN_CHECK_POOR_TRACKING) printf(" TRACKING");
                            if (check_result & KALMAN_CHECK_POOR_STABILITY) printf(" STABILITY");
                            printf("\r\n");
                        }
                    } else {
                        printf("Kalman filter not initialized for channel %d\r\n", channel);
                    }
                } else {
                    printf("Invalid channel number. Range: 0-%d\r\n", sgm5860xChannelMax - 1);
                }
            } else {
                // 检查所有通道
                for (int i = 0; i < sgm5860xChannelMax; i++) {
                    if (sgm5860xStatus.kalman_initialized[i]) {
                        int check_result =
                            adaptive_kalman_self_check(&sgm5860xStatus.kalman_filters[i]);
                        printf("Channel %d: ", i);
                        if (check_result == KALMAN_CHECK_OK) {
                            printf("OK\r\n");
                        } else {
                            printf("ERROR 0x%02X\r\n", check_result);
                        }
                    }
                }
            }
            return 0;
        }

        printf("Unknown kalman command: %s\r\n", argv[2]);
        return 0;
    }
#endif
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), sgm5860x,
                 Cmdsgm5860xHandle, sgm5860x command);

#if FILTER_MODE == KALMAN_FILTER_MODE
/**
 * @brief 初始化ADC通道的自适应卡尔曼滤波器
 * @param channel_index 通道索引
 */
static void sgm5860x_kalman_filter_init(uint8_t channel_index) {
    if (channel_index >= sgm5860xChannelMax) return;

    // 根据通道特性设置不同的初始参数
    double initial_value     = 0.0;
    double process_noise     = 0.01;  // 默认过程噪声
    double measurement_noise = 0.1;   // 默认测量噪声

    // 根据通道配置调整滤波器参数
    uint8_t gain = sgm5860_channelcfg[channel_index].gain;
    switch (gain) {
        case SGM58601_GAIN_1:
        case SGM58601_GAIN_2:
        case SGM58601_GAIN_4:
            // 低增益通道：信号较大，噪声相对较小
            process_noise     = 0.005;
            measurement_noise = 0.05;
            break;

        case SGM58601_GAIN_8:
        case SGM58601_GAIN_16:
        case SGM58601_GAIN_32:
            // 中增益通道：信号中等，噪声中等
            process_noise     = 0.01;
            measurement_noise = 0.1;
            break;

        case SGM58601_GAIN_64:
        case SGM58601_GAIN_128:
            // 高增益通道：信号较小，噪声相对较大
            process_noise     = 0.005;
            measurement_noise = 0.05;
            break;
    }

    // 初始化自适应卡尔曼滤波器
    adaptive_kalman_init(&sgm5860xStatus.kalman_filters[channel_index], initial_value,
                         process_noise, measurement_noise, 1.0);

    // 设置参数边界
    adaptive_kalman_set_bounds(&sgm5860xStatus.kalman_filters[channel_index],
                               process_noise * 0.1,        // Q_min
                               process_noise * 10.0,       // Q_max
                               measurement_noise * 0.1,    // R_min
                               measurement_noise * 10.0);  // R_max

    // 根据通道特性调整自适应速率
    if (gain >= SGM58601_GAIN_64) {
        // 高增益通道需要更快的自适应速率
        // adaptive_kalman_set_adaptation_rate(&sgm5860xStatus.kalman_filters[channel_index], 0.15);
        adaptive_kalman_set_adaptation_rate(&sgm5860xStatus.kalman_filters[channel_index], 0.08);

    } else {
        // 低增益通道使用较慢的自适应速率
        adaptive_kalman_set_adaptation_rate(&sgm5860xStatus.kalman_filters[channel_index], 0.08);
    }

    sgm5860xStatus.kalman_initialized[channel_index] = 1;
    sgm5860xStatus.sample_count[channel_index]       = 0;
    sgm5860xStatus.filtered_voltage[channel_index]   = 0.0;

    elog_d(TAG, "Kalman filter initialized for channel %d (gain=%d)", channel_index, gain);
}

/**
 * @brief 获取通道的滤波器性能统计
 * @param channel_index 通道索引
 */
static void sgm5860x_print_kalman_stats(uint8_t channel_index) {
    if (channel_index >= sgm5860xChannelMax || !sgm5860xStatus.kalman_initialized[channel_index])
        return;

    AdaptiveKalmanStatus_t status;
    adaptive_kalman_get_status(&sgm5860xStatus.kalman_filters[channel_index], &status);

    elog_i(TAG, "Channel %d Kalman Stats:", channel_index);
    elog_i(TAG, "  Value: %.6f V, Gain: %.4f", status.current_value, status.kalman_gain);
    elog_i(TAG, "  Q: %.6f, R: %.6f", status.process_noise, status.measurement_noise);
    elog_i(TAG, "  Stability: %.4f, Error: %.6f", status.stability_metric, status.tracking_error);
    elog_i(TAG, "  Signal State: %d, Adaptations: %d", status.signal_state,
           status.adaptation_counter);
}

/**
 * @brief 检查并重置异常的滤波器
 * @param channel_index 通道索引
 */
static void sgm5860x_check_kalman_health(uint8_t channel_index) {
    if (channel_index >= sgm5860xChannelMax || !sgm5860xStatus.kalman_initialized[channel_index])
        return;

    int check_result = adaptive_kalman_self_check(&sgm5860xStatus.kalman_filters[channel_index]);

    if (check_result != KALMAN_CHECK_OK) {
        elog_w(TAG, "Channel %d Kalman filter error: 0x%02X", channel_index, check_result);

        // 如果检测到严重错误，重置滤波器
        if (check_result & (KALMAN_CHECK_UNSTABLE | KALMAN_CHECK_POOR_STABILITY)) {
            double current_value = sgm5860xStatus.filtered_voltage[channel_index];
            adaptive_kalman_reset(&sgm5860xStatus.kalman_filters[channel_index], current_value);
            elog_i(TAG, "Channel %d Kalman filter reset to %.6f", channel_index, current_value);
        }
    }
}
#endif
