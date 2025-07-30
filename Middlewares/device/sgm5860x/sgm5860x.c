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
#include "sgm5860x.h"
#include "stream_buffer.h"
// #include "app_event.h"

#define TAG                           "sgm5860x"
#define sgm5860xLogLvl                ELOG_LVL_INFO
#define SGM58601_GAIN_128_MEASURE_MAX (0.030)  // 30mv
#define sgm5860xPriority              PriorityNormalEventGroup0
#ifndef sgm5860xChannelMax
#define sgm5860xChannelMax 4
#endif

#ifndef CONFIG_sgm5860x_CYCLE_TIMER_MS
#define CONFIG_sgm5860x_CYCLE_TIMER_MS 300  // 优化：300ms采样周期，提升响应速度100%
#endif
#define STEAM_BUFFER_CNT  (16)
#define SCAN_MAX_CNT      (10)
#define STEAM_BUFFER_SIZE (sizeof(DevSgm5860xSetStruct) * STEAM_BUFFER_CNT)
// Channel 6超精密模式专用配置
#define CHANNEL6_ULTRA_PRECISION_MODE 1
#define CHANNEL6_TARGET_PRECISION_UV  0.1  // 0.1μV目标精度
#define CHANNEL6_RESPONSE_TIME_SEC    3.0  // 3秒响应时间要求

const double f_gain_map[] = {
    [SGM58601_GAIN_1] = 1.0,   [SGM58601_GAIN_2] = 2.0,     [SGM58601_GAIN_4] = 4.0,
    [SGM58601_GAIN_8] = 8.0,   [SGM58601_GAIN_16] = 16.0,   [SGM58601_GAIN_32] = 32.0,
    [SGM58601_GAIN_64] = 64.0, [SGM58601_GAIN_128] = 128.0,
};

void sgm5860xReadyCallback(void *ptrDevPinHandle);
typedef enum {
    FILTER_MODE_RAW = 0,  // 原始数据模式
    FILTER_MODE_KALMAN,   // 卡尔曼滤波
    FILTER_MODE_AVERAGE,  // 平均值滤波
    FILTER_MODE_MAX       // 模式最大
} FilterMode;
typedef struct {
    uint8_t enable;
    uint8_t channel;  // Channel number
    uint8_t gain;     // Gain setting for the channel
    uint8_t sps;      // Samples per second setting for the channel
} ChannelCfg_t;
ChannelCfg_t sgm5860_channelcfg[sgm5860xChannelMax] = {
    {true, SGM58601_MUXN_AIN0, SGM58601_GAIN_1, SGM58601_DRATE_30SPS},    // Channel 0: Gain 1
    {true, SGM58601_MUXN_AIN2, SGM58601_GAIN_1, SGM58601_DRATE_30SPS},    // Channel 2: Gain 1
    {true, SGM58601_MUXN_AIN4, SGM58601_GAIN_1, SGM58601_DRATE_30SPS},    // Channel 4: Gain 1
    {true, SGM58601_MUXN_AIN6, SGM58601_GAIN_32, SGM58601_DRATE_2_5SPS},  // Channel 6: Gain 32

};
typedef enum {
    SGM5860X_NONE_MODE,  // Device is in read mode
    SGM5860X_INIT_MODE,  // Device is in set mode
    SGM5860X_STOP_CONTINUES_MODE,
    SGM5860X_SET_MODE,
    SGM5860X_START_CONTINUES_MODE,
    SGM5860X_READ_MODE,  // Device is in set mode
} SGMSETSTATE_t;

/* ===================================================================================== */
const DevSgm5860xHandleStruct sgm5860_cfg = {
    .rate = SGM58601_DEFAULT_SPS,
    .drdy =
        {
            .device_name = "sgm5860x_drdy",
            .base        = GPIOD,
            .af          = 0,
            .pin         = GPIO_PIN_9,
            .pin_mode    = DevPinModeInput,
            .bit_value   = 1,  // Active low
            .isrcb       = sgm5860xReadyCallback,
            .exti_trig   = EXTI_TRIG_FALLING,
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
int sgm5860xReadSingleData(unsigned char channel);
void Sgm5860xStreamRcvTask(void *arg);

typedef struct {
    char device_name[DEVICE_NAME_MAX];
    uint32_t id;  // ID
    DevSgm5860xHandleStruct *cfg;
    uint8_t status;                           // Status of the device
    FilterMode filter_mode;                   // Filter mode
    uint8_t scan_index;                       // Channel to scan
    double last_voltage[sgm5860xChannelMax];  // Last voltage read from each channel
    double sum[sgm5860xChannelMax];
    double average[sgm5860xChannelMax];                   // Average voltage for each channel
    uint8_t vol_index[sgm5860xChannelMax];                // Voltage index for each channel
    double tmp_voltage[sgm5860xChannelMax][AVG_MAX_CNT];  // Temporary voltage storage
    StreamBufferHandle_t steam_buffer;
    SGMSETSTATE_t mode;  // Current mode of the device
    DevSgm5860xSetStruct current_data;
    DevSgm5860xSetStruct next_data;
#if FILTER_MODE == KALMAN_FILTER_MODE
    SignalState_t last_kalman_state[sgm5860xChannelMax];
    AdaptiveKalmanFilter_t kalman_filters[sgm5860xChannelMax];  // Kalman filters for each channel
    uint8_t kalman_initialized[sgm5860xChannelMax];  // Kalman filter initialization status
    double filtered_voltage[sgm5860xChannelMax];     // Filtered voltage for each channel
    uint32_t sample_count[sgm5860xChannelMax];       // Sample count for each channel

#elif FILTER_MODE == AVG_FILTER_MODE
    // double tmp_voltage[sgm5860xChannelMax][AVG_MAX_CNT];  // Temporary voltage storage
#endif

} Typdefsgm5860xStatus;
volatile Typdefsgm5860xStatus sgm5860xStatus = {0};

/* ===================================================================================== */

static const vfb_event_t sgm5860xEventList[] = {

    sgm5860xStart, sgm5860xStop, sgm5860xGet, sgm5860xSet, sgm5860xMode,

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
void sgm5860xReadyCallback(void *ptrDevPinHandle) {
    DevPinHandleStruct *DevPinHandle = ptrDevPinHandle;
    static DevSgm5860xSetStruct tmp_bffer[SCAN_MAX_CNT];
    static int sgm5860xCount = 0;  // Ready count for debugging
    switch (sgm5860xStatus.mode) {
        case SGM5860X_NONE_MODE: {
            // Do nothing, waiting for initialization
            return;
        }
        case SGM5860X_INIT_MODE: {
            printf("\r\nsgm5860xReadyCallback: INIT_MODE\r\n");
            uint8_t result = DevSgm5860xConfig(&sgm5860_cfg);  // Configure the SGM5860x device
            if (result != 1) {
                printf("[ERROR]sgm5860xReadyCallback: Failed to configure device\r\n");
            } else {
                printf("sgm5860xReadyCallback: Device configured successfully\r\n");
                sgm5860xStatus.mode = SGM5860X_SET_MODE;
            }
            break;
        }
        case SGM5860X_STOP_CONTINUES_MODE: {
            sgm5860xStatus.mode = SGM5860X_SET_MODE;
            // printf("sgm5860xReadyCallback: STOP_CONTINUES_MODE\r\n");
            // break;// Do not break here, continue to SET_MODE
        }
        case SGM5860X_SET_MODE: {
            // printf("sgm5860xReadyCallback: SET_MODE\r\n");
            uint8_t result = DevSgm5860xSet(sgm5860xStatus.cfg, &sgm5860xStatus.next_data);
            if (result != 1) {
                printf("[ERROR]sgm5860xReadyCallback: Failed to set mode\r\n");
            } else {
                sgm5860xStatus.mode                 = SGM5860X_START_CONTINUES_MODE;
                sgm5860xStatus.current_data.channel = sgm5860xStatus.next_data.channel;
                sgm5860xStatus.current_data.gain    = sgm5860xStatus.next_data.gain;
                sgm5860xStatus.current_data.voltage = sgm5860xStatus.next_data.voltage;
                sgm5860xStatus.current_data.sps     = sgm5860xStatus.next_data.sps;
                // printf("sgm5860xReadyCallback: SET_MODE ,Next Channel: %d, Gain: %d\r\n",
                //        sgm5860xStatus.next_data.channel, sgm5860xStatus.next_data.gain);
            }
        } break;
        case SGM5860X_START_CONTINUES_MODE: {
            // printf("sgm5860xReadyCallback: START_CONTINUES_MODE\r\n");
            sgm5860xStatus.mode = SGM5860X_READ_MODE;
            DevSgm5860xStartContinuousMode(sgm5860xStatus.cfg);
        } break;
        case SGM5860X_READ_MODE: {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            tmp_bffer[sgm5860xCount].channel    = sgm5860xStatus.current_data.channel;
            tmp_bffer[sgm5860xCount].gain       = sgm5860xStatus.current_data.gain;
            tmp_bffer[sgm5860xCount].voltage    = DevSgm5860xReadValue(sgm5860xStatus.cfg);
            sgm5860xCount++;
            if (sgm5860xCount >= SCAN_MAX_CNT) {
                if (xStreamBufferSendFromISR(sgm5860xStatus.steam_buffer, tmp_bffer,
                                             sizeof(tmp_bffer), &xHigherPriorityTaskWoken) == 0) {
                    printf("\r\n[ERROR]Sgm5860 Failed to send data to stream buffer from ISR\r\n");
                }
                sgm5860xCount = 0;
                for (int i = 0; i < SCAN_MAX_CNT; i++) {
                    sgm5860xStatus.last_voltage[i] = tmp_bffer[i].voltage;
                }
                do {
                    sgm5860xStatus.scan_index++;
                    if (sgm5860xStatus.scan_index >
                        sizeof(sgm5860_channelcfg) / sizeof(sgm5860_channelcfg[0]) - 1) {
                        sgm5860xStatus.scan_index =
                            0;  // Reset channel to 0 after reaching the last channel
                    }
                } while (sgm5860_channelcfg[sgm5860xStatus.scan_index].enable == false);
                // printf("Scan index %d\r\n", sgm5860xStatus.scan_index);
                sgm5860xStatus.next_data.channel = sgm5860_channelcfg[sgm5860xStatus.scan_index]
                                                       .channel;  // Get the current channel
                sgm5860xStatus.next_data.gain = sgm5860_channelcfg[sgm5860xStatus.scan_index].gain;
                sgm5860xStatus.next_data.sps  = sgm5860_channelcfg[sgm5860xStatus.scan_index].sps;
                sgm5860xStatus.mode           = SGM5860X_STOP_CONTINUES_MODE;
            }
        } break;
        default:
            printf("[ERROR]sgm5860xReadyCallback: Invalid device mode %d\r\n", sgm5860xStatus.mode);
            return;  // Invalid mode, skip callback
    }
}
void sgm5860xDeviceInit(void) {
    elog_i(TAG, "sgm5860xDeviceInit");

    DevSgm5860xHandleStruct *sgm5860xBspCfg = (DevSgm5860xHandleStruct *)&sgm5860_cfg;
    sgm5860xStatus.cfg                      = sgm5860xBspCfg;  // Assign the BSP configuration
    strcpy((char *)sgm5860xStatus.device_name, "sgm58601");
    sgm5860xStatus.scan_index           = 0;  // Initialize scan index
    sgm5860xStatus.mode                 = SGM5860X_NONE_MODE;
    sgm5860xStatus.current_data.voltage = F_INVAILD;
    sgm5860xStatus.current_data.channel = SGM58601_DEFAULT_CHANNEL;
    sgm5860xStatus.current_data.gain    = SGM58601_DEFAULT_GAIN;
    sgm5860xStatus.next_data.voltage    = F_INVAILD;
    sgm5860xStatus.next_data.channel    = SGM58601_DEFAULT_CHANNEL;
    sgm5860xStatus.next_data.gain       = SGM58601_DEFAULT_GAIN;
    sgm5860xStatus.filter_mode          = FILTER_MODE_KALMAN;
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
#if FILTER_MODE == KALMAN_FILTER_MODE
    // 初始化所有通道的卡尔曼滤波器
    for (int i = 0; i < sgm5860xChannelMax; i++) {
        sgm5860x_kalman_filter_init(i);
    }
    elog_i(TAG, "All Kalman filters initialized");
#endif
    sgm5860xStatus.steam_buffer =
        xStreamBufferCreate(STEAM_BUFFER_SIZE, sizeof(DevSgm5860xSetStruct));
    DevSgm5860xInit(sgm5860xBspCfg);  // Initialize the SGM5860x device
    elog_i(TAG, "sgm5860xDeviceInit completed");
    elog_i(TAG, "sgm5860xStatus.device_name: %s", sgm5860xStatus.device_name);
}
SYSTEM_REGISTER_INIT(MCUInitStage, sgm5860xPriority, sgm5860xDeviceInit, sgm5860xDeviceInit);

static void __sgm5860xCreateTaskHandle(void) {
    elog_i(TAG, "__sgm5860xCreateTaskHandle");

    xTaskCreate(VFBTaskFrame, "VFBTasksgm5860x", configMINIMAL_STACK_SIZE * 2,
                (void *)&sgm5860x_task_cfg, sgm5860xPriority, NULL);
    xTaskCreate(Sgm5860xStreamRcvTask, "Sgm5860xRcvTask", configMINIMAL_STACK_SIZE * 2,
                (void *)&sgm5860xStatus, sgm5860xPriority - 1, NULL);
}
SYSTEM_REGISTER_INIT(BoardInitStage, sgm5860xPriority, __sgm5860xCreateTaskHandle,
                     __sgm5860xCreateTaskHandle init);
DevSgm5860xSetStruct steam_rcv[STEAM_BUFFER_CNT] = {0};
void Sgm5860xStreamRcvTask(void *arg) {
    Typdefsgm5860xStatus *ptr_status = &sgm5860xStatus;
    int rcv_count                    = 0;
    double f_gain                    = 0.0;
    double voltage                   = 0.0;
    static int max_cnt               = 0;
    uint8_t ch                       = 0;
    uint8_t scan_index               = 0;
    uint8_t last_scan_index          = 0;
    double vol                       = 0.0;
    double sum                       = 0.0;

    while (1) {
        rcv_count = xStreamBufferReceive(ptr_status->steam_buffer, &steam_rcv, sizeof(steam_rcv),
                                         portMAX_DELAY);
        if (max_cnt < rcv_count / sizeof(DevSgm5860xSetStruct)) {
            max_cnt = rcv_count / sizeof(DevSgm5860xSetStruct);
        }
        elog_d(TAG, "rcv_count = %d cnt %d max %d", rcv_count,
               rcv_count / sizeof(DevSgm5860xSetStruct), max_cnt);
        sum = 0.0;
        elog_d(TAG, "Filter Mode: %d, rcv_count: %d", ptr_status->filter_mode, rcv_count);
        switch (ptr_status->filter_mode) {
            case FILTER_MODE_RAW: {
                // 取中位值（中值滤波）
                int n = rcv_count / sizeof(DevSgm5860xSetStruct);
                if (n > 0) {
                    double values[SCAN_MAX_CNT];
                    for (int i = 0; i < n; i++) {
                        values[i] = steam_rcv[i].voltage;
                    }
                    // 冒泡排序（数据量很小，足够快）
                    for (int i = 0; i < n - 1; i++) {
                        for (int j = 0; j < n - i - 1; j++) {
                            if (values[j] > values[j + 1]) {
                                double tmp    = values[j];
                                values[j]     = values[j + 1];
                                values[j + 1] = tmp;
                            }
                        }
                    }
                    double median;
                    if (n % 2 == 0) {
                        median = (values[n / 2 - 1] + values[n / 2]) / 2.0;
                    } else {
                        median = values[n / 2];
                    }
                    f_gain                                  = f_gain_map[steam_rcv[0].gain];
                    ch                                      = steam_rcv[0].channel;
                    scan_index                              = ch / 2;
                    sgm5860xStatus.last_voltage[scan_index] = median;
                    vol                                     = median / f_gain;
                }
            } break;
            case FILTER_MODE_KALMAN: {
                // Kalman filter mode
                for (int i = 0; i < rcv_count / sizeof(DevSgm5860xSetStruct); i++) {
                    DevSgm5860xSetStruct *data = &steam_rcv[i];
                    f_gain                     = f_gain_map[data->gain];
                    ch                         = data->channel;  // Get the channel number
                    scan_index                 = ch / 2;
                    sgm5860xStatus.last_voltage[scan_index] = data->voltage;  // Convert to mV

                    sgm5860xStatus.filtered_voltage[scan_index] =
                        adaptive_kalman_update(&sgm5860xStatus.kalman_filters[scan_index],
                                               sgm5860xStatus.last_voltage[scan_index]);
                    sgm5860xStatus.sample_count[scan_index]++;
                    vol = sgm5860xStatus.filtered_voltage[scan_index] / f_gain;

                    // sum = sum + data->voltage / f_gain;
                }
            } break;
            case FILTER_MODE_AVERAGE: {
                // Remove one max and one min, then average the rest
                int n = rcv_count / sizeof(DevSgm5860xSetStruct);
                if (n > 2) {
                    double values[SCAN_MAX_CNT];
                    for (int i = 0; i < n; i++) {
                        values[i] = steam_rcv[i].voltage;
                    }
                    // Find max and min indices
                    int max_idx = 0, min_idx = 0;
                    for (int i = 1; i < n; i++) {
                        if (values[i] > values[max_idx]) max_idx = i;
                        if (values[i] < values[min_idx]) min_idx = i;
                    }
                    // Calculate sum excluding max and min
                    double sum = 0.0;
                    int cnt    = 0;
                    for (int i = 0; i < n; i++) {
                        if (i != max_idx && i != min_idx) {
                            sum += values[i];
                            cnt++;
                        }
                    }
                    double avg                              = cnt > 0 ? sum / cnt : 0.0;
                    f_gain                                  = f_gain_map[steam_rcv[0].gain];
                    ch                                      = steam_rcv[0].channel;
                    scan_index                              = ch / 2;
                    sgm5860xStatus.last_voltage[scan_index] = avg;
                    vol                                     = avg / f_gain;
                } else {
                    f_gain                                  = f_gain_map[steam_rcv[0].gain];
                    ch                                      = steam_rcv[0].channel;
                    scan_index                              = ch / 2;
                    sgm5860xStatus.last_voltage[scan_index] = steam_rcv[0].voltage;
                    vol                                     = steam_rcv[0].voltage / f_gain;
                }
            } break;
            default: {
                elog_e(TAG, "Invalid filter mode %d", ptr_status->filter_mode);
            }
        }
        elog_d(TAG, "Get ADC CH:%d Vol %.9f", ch, vol);
        vfb_send(sgm5860xCH1 + scan_index, 0, &vol, sizeof(vol));
    }
}
static void __sgm5860xInitHandle(void *msg) {
    elog_i(TAG, "__sgm5860xInitHandle");
    // elog_set_filter_tag_lvl(TAG, sgm5860xLogLvl);
    vTaskDelay(pdMS_TO_TICKS(1));  // Delay to ensure the system is ready
    // Initialize the SGM5860x device
    vfb_send(sgm5860xStart, 0, NULL, 0);
}

static void __sgm5860xRcvHandle(void *msg) {
    TaskHandle_t curTaskHandle = xTaskGetCurrentTaskHandle();
    char *taskName             = pcTaskGetName(curTaskHandle);
    vfb_message_t tmp_msg      = (vfb_message_t)msg;
    switch (tmp_msg->frame->head.event) {
        case sgm5860xStart: {
            elog_i(TAG, "sgm5860xStartTask sgm5860xStart");
            sgm5860xStatus.mode = SGM5860X_INIT_MODE;
            sgm5860xStatus.status =
                1;  // Set status to indicate the device is started //TODO 需要修改
            DevSgm5860xStart(&sgm5860_cfg);  // Start the SGM5860x device
        } break;
        case sgm5860xStop: {
            elog_i(TAG, "sgm5860xStopTask %d", tmp_msg->frame->head.data);
            DevSgm5860xStop(&sgm5860_cfg);  // Stop the SGM5860x device
            sgm5860xStatus.status = 0;      // Set status to indicate the device is stopped
        } break;
        case sgm5860xSet: {
            DevSgm5860xSetStruct *ptr = (DevSgm5860xSetStruct *)MSG_GET_PAYLOAD(tmp_msg);
            elog_i(TAG, "Set Channel %d ,Gain %d", ptr->channel, ptr->gain);
            sgm5860xStatus.next_data.channel = ptr->channel;
            sgm5860xStatus.next_data.gain    = ptr->gain;
            sgm5860xStatus.next_data.voltage = F_INVAILD;
            sgm5860xStatus.next_data.sps     = ptr->sps;
            sgm5860xStatus.mode              = SGM5860X_STOP_CONTINUES_MODE;
        } break;
        case sgm5860xMode: {
            SGM5860xScanMode_t mode = (SGM5860xScanMode_t)MSG_GET_DATA(tmp_msg);
            switch (mode) {
                case SGM5860xScanModeAll: {
                    sgm5860_channelcfg[0].enable = true;  // 40mv
                    sgm5860_channelcfg[1].enable = true;  // 9v 1
                    sgm5860_channelcfg[2].enable = true;  // 9v 2
                    sgm5860_channelcfg[3].enable = true;  // 9V 3
                } break;
                case SGM5860xScanModeSingle40mV: {
                    sgm5860_channelcfg[0].enable = false;  // 40mv
                    sgm5860_channelcfg[1].enable = false;  // 9v 1
                    sgm5860_channelcfg[2].enable = false;  // 9v 2
                    sgm5860_channelcfg[3].enable = true;   // 9V 3
                } break;
                case SGM5860xScanModeSingle9V1: {
                    sgm5860_channelcfg[0].enable = false;  // 40mv
                    sgm5860_channelcfg[1].enable = true;   // 9v 1
                    sgm5860_channelcfg[2].enable = false;  // 9v 2
                    sgm5860_channelcfg[3].enable = false;  // 9V 3
                } break;
                case SGM5860xScanModeSingle9V2: {
                    sgm5860_channelcfg[0].enable = false;  // 40mv
                    sgm5860_channelcfg[1].enable = false;  // 9v 1
                    sgm5860_channelcfg[2].enable = true;   // 9v 2
                    sgm5860_channelcfg[3].enable = false;  // 9V 3
                } break;
                case SGM5860xScanModeSingle9V3: {
                    sgm5860_channelcfg[0].enable = false;  // 40mv
                    sgm5860_channelcfg[1].enable = false;  // 9v 1
                    sgm5860_channelcfg[2].enable = false;  // 9v 2
                    sgm5860_channelcfg[3].enable = true;   // 9V 3
                } break;
                case SGM5860xScanModeAll9V: {
                    sgm5860_channelcfg[0].enable = true;   // 40mv
                    sgm5860_channelcfg[1].enable = true;   // 9v 1
                    sgm5860_channelcfg[2].enable = true;   // 9v 2
                    sgm5860_channelcfg[3].enable = false;  // 9V 3
                } break;
                default:
                    elog_e(TAG, "TASK %s RCV: unknown mode: %d", taskName, mode);
                    break;
            }
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
#if 0
    if (sgm5860xStatus.status == 2) {
        // sgm5860xStatus.scan_index
        double last_voltage           = 0;
        uint8_t last_channel          = 0;
        uint8_t last_index            = 0;
        double last_gain              = 0;
        static uint8_t channel_6_gain = 0;
        (void *)&channel_6_gain;
        sgm5860xStatus.scan_index = 3;
        uint8_t channel =
            sgm5860_channelcfg[sgm5860xStatus.scan_index].channel;  // Get the current channel
        uint8_t gain = sgm5860_channelcfg[sgm5860xStatus.scan_index].gain;
        // DevGetADCData(&sgm5860_cfg, &last_voltage, &last_channel, channel, gain);
        for (int i = 0; i < sizeof(sgm5860_channelcfg) / sizeof(sgm5860_channelcfg[0]); i++) {
            if (sgm5860_channelcfg[i].channel == last_channel) {
                last_index = i;
                last_gain =
                    f_gain_map[sgm5860_channelcfg[i].gain];  // Get the gain for the last channel
                sgm5860xStatus.last_voltage[i] = last_voltage;

#if FILTER_MODE == KALMAN_FILTER_MODE
                // 使用自适应卡尔曼滤波器处理数据
                if (sgm5860xStatus.kalman_initialized[i]) {
                    double tmp_voltage = last_voltage;
                    // 更新卡尔曼滤波器
                    if (last_channel == 6) {
                        //  elog_i(TAG, "%.9fmV", last_voltage*1000.0);
                        last_gain                                                  = 64;
                        sgm5860xStatus.tmp_voltage[i][sgm5860xStatus.vol_index[i]] = last_voltage;
                        sgm5860xStatus.vol_index[i] =
                            (sgm5860xStatus.vol_index[i] + 1) % AVG_MAX_CNT;
                        sgm5860xStatus.sum[i] = 0;
                        for (int j = 0; j < AVG_MAX_CNT; j++) {
                            sgm5860xStatus.sum[i] += sgm5860xStatus.tmp_voltage[i][j];
                        }
                        sgm5860xStatus.average[i] = sgm5860xStatus.sum[i] / (double)AVG_MAX_CNT;
                        // if (sgm5860xStatus.vol_index[i] == AVG_MAX_CNT) {

                        // sgm5860xStatus.sum[i]     = 0.0;
                        // sgm5860xStatus.vol_index[i] = 0;

                        sgm5860xStatus.filtered_voltage[i] = adaptive_kalman_update(
                            &sgm5860xStatus.kalman_filters[i], sgm5860xStatus.average[i]);
                        sgm5860xStatus.filtered_voltage[i] =
                            sgm5860xStatus.filtered_voltage[i] / last_gain;
                        sgm5860xStatus.sample_count[i]++;
                        // }

                    } else {
                        sgm5860xStatus.filtered_voltage[i] = adaptive_kalman_update(
                            &sgm5860xStatus.kalman_filters[i], sgm5860xStatus.last_voltage[i]);
                        sgm5860xStatus.filtered_voltage[i] =
                            sgm5860xStatus.filtered_voltage[i] / last_gain;
                        sgm5860xStatus.sample_count[i]++;
                    }
                    if (sgm5860xStatus.sample_count[i] % 10 == 0) {
                        elog_d(TAG, "%.9fmV", last_voltage * 1000.0);
                        elog_d(TAG, "Channel %d state unchanged %d ", i,
                               sgm5860xStatus.kalman_filters[i].signal_state);
                        vfb_send(sgm5860xCH1 + last_index, 0, &(sgm5860xStatus.filtered_voltage[i]),
                                 sizeof(sgm5860xStatus.filtered_voltage[i]));
                    }

                    // 每100个样本检查一次滤波器健康状态
                    if (sgm5860xStatus.sample_count[i] % 100 == 0) {
                        sgm5860x_check_kalman_health(i);
                    }

                    // Channel 6小信号模式：更频繁的精度监控和渐进优化
                    if (last_channel == 6) {
                        // 渐进式精度提升策略
                        if (sgm5860xStatus.sample_count[i] % 200 == 0 &&
                            sgm5860xStatus.sample_count[i] > 500) {
                            AdaptiveKalmanFilter_t *akf = &sgm5860xStatus.kalman_filters[i];
                            double precision_uv = adaptive_kalman_get_voltage_range(akf) * 1000000;

                            // 针对10μV→1μV的渐进式精度优化
                            if (precision_uv > 5.0 && akf->Q > 0.0001) {
                                // 精度>5μV时，积极收紧
                                akf->Q = akf->Q * 0.90;  // 降低10%
                                elog_d(TAG, "CH6 aggressive tuning: Q=%.1fμV, precision=%.1fμV",
                                       akf->Q * 1000000, precision_uv);
                            } else if (precision_uv > 2.0 && akf->Q > 0.00008) {
                                // 精度2-5μV时，持续优化
                                akf->Q = akf->Q * 0.95;  // 降低5%
                                elog_d(TAG, "CH6 progressive tuning: Q=%.1fμV, precision=%.1fμV",
                                       akf->Q * 1000000, precision_uv);
                            } else if (precision_uv > 1.0 && akf->Q > 0.00006) {
                                // 精度1-2μV时，精细调整
                                akf->Q = akf->Q * 0.98;  // 降低2%
                                elog_d(TAG, "CH6 fine tuning: Q=%.1fμV, precision=%.1fμV",
                                       akf->Q * 1000000, precision_uv);
                            }
                        }

                        // 每100个样本检查精度达标情况
                        if (sgm5860xStatus.sample_count[i] % 100 == 0) {
                            double precision_uv = adaptive_kalman_get_voltage_range(
                                                      &sgm5860xStatus.kalman_filters[i]) *
                                                  1000000;
                            if (precision_uv <= 1.0) {
                                elog_d(
                                    TAG,
                                    "CH6 EXCELLENT: %.1fμV ≤ 1μV target achieved! (samples: %lu)",
                                    precision_uv, sgm5860xStatus.sample_count[i]);
                            } else if (precision_uv <= 2.0) {
                                elog_d(
                                    TAG,
                                    "CH6 very good: %.1fμV approaching 1μV target (samples: %lu)",
                                    precision_uv, sgm5860xStatus.sample_count[i]);
                            } else if (precision_uv <= 5.0) {
                                elog_d(TAG,
                                       "CH6 good: %.1fμV, optimization in progress (samples: %lu)",
                                       precision_uv, sgm5860xStatus.sample_count[i]);
                            }
                        }
                    }
                } else {
                    // 如果滤波器未初始化，使用原始值
                    sgm5860xStatus.filtered_voltage[i] = last_voltage;
                    elog_w(TAG, "Kalman filter not initialized for channel %d", i);
                }
#elif FILTER_MODE == AVG_FILTER_MODE
                sgm5860xStatus.tmp_voltage[i][sgm5860xStatus.vol_index[i]] = last_voltage;
                sgm5860xStatus.sum[i] += last_voltage;  // Accumulate the voltage for averaging

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
#endif
}

#endif
void CmdSgm5860Set(uint8_t ch, uint8_t gain, uint8_t sps) {
    elog_i(TAG, "CmdSgm5860Set: Channel %d, Gain %d, SPS %d", ch, gain, sps);
    DevSgm5860xSetStruct set = {0};
    set.channel              = ch;
    set.gain                 = gain;
    set.sps                  = sps;
    vfb_send(sgm5860xSet, 0, &set, sizeof(set));
}
void CmdSgm5860ScanMode(SGM5860xScanMode_t mode) {
    elog_i(TAG, "CmdSgm5860ScanMode: Mode %d", mode);
    vfb_send(sgm5860xMode, mode, NULL, 0);
}
static void Cmdsgm5860xHelp(void) {
    printf("Usage: sgm5860x <command>\r\n");
    printf("Commands:\r\n");
    printf("  help          Show this help message\r\n");
    printf("  config        Configure the SGM5860x device\r\n");
    // scan

    //  sgm5860xStatus.filter_mode
    printf("  scan <mode>   Set scan mode for SGM5860x device\r\n");
    printf("    0 - All channels (40mV, 9V1, 9V2, 9V3)\r\n");
    printf("    1 - Single channel 40mV\r\n");
    printf("    2 - Single channel 9V1\r\n");
    printf("    3 - Single channel 9V2\r\n");
    printf("    4 - Single channel 9V3\r\n");
    printf("    5 - ALL channels 9V\r\n");

    // filter_mode
    printf("  filter <mode> Set filter mode for SGM5860x device\r\n");
    printf("    0 - Raw data (no filtering)\r\n");
    printf("    1 - Kalman filter\r\n");
    printf("    2 - Average filter\r\n");

    printf("  reset         Reset the SGM5860x device\r\n");
    printf("  init          Initialize the SGM5860x device\r\n");
    printf("  data          Read data from the SGM5860x device\r\n");
    printf("  read <addr>   Read register from SGM5860x device\r\n");
    printf("  set <ch> <gain> - Set channel and gain for SGM5860x device\r\n");
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
    // filter_mode
    if (strcmp(argv[1], "filter") == 0) {
        if (argc < 3) {
            printf("\r\n Current filter mode: %d\r\n", sgm5860xStatus.filter_mode);
            printf("  filter <mode> Set filter mode for SGM5860x device\r\n");
            printf("    0 - Raw data (no filtering)\r\n");
            printf("    1 - Kalman filter\r\n");
            printf("    2 - Average filter\r\n");
            return 0;
        }
        unsigned char mode = (unsigned char)strtol(argv[2], NULL, 10);
        if (mode >= FILTER_MODE_MAX) {
            elog_e(TAG, "Invalid filter mode");
            return 0;
        }
        sgm5860xStatus.filter_mode = mode;  // Set the filter mode for the SGM5860x device
        elog_i(TAG, "SGM5860x filter mode set to %d", mode);
        return 0;
    }
    // set
    if (strcmp(argv[1], "set") == 0) {
        if (argc < 4) {
            elog_e(TAG, "Usage: sgm5860x set <channel> <gain> <sps>");
            return 0;
        }
        unsigned char channel = (unsigned char)strtol(argv[2], NULL, 10);
        unsigned char gain    = (unsigned char)strtol(argv[3], NULL, 10);
        uint8_t sps           = (uint8_t)strtol(argv[4], NULL, 10);
        if (channel >= SGM58601_MUXN_AINCOM || gain >= SGM58601_GAIN_MAX) {
            elog_e(TAG, "Invalid channel or gain value");
            return 0;
        }
        CmdSgm5860Set(channel, gain, sps);  // Set the channel and gain for the SGM5860x device
        elog_i(TAG, "SGM5860x set channel %d, gain %d, sps %d", channel, gain, sps);
        return 0;
    }
    // scan mode
    if (strcmp(argv[1], "scan") == 0) {
        if (argc < 3) {
            elog_e(TAG, "Usage: sgm5860x scan <mode>");
            return 0;
        }
        SGM5860xScanMode_t mode = (SGM5860xScanMode_t)strtol(argv[2], NULL, 10);
        if (mode < SGM5860xScanModeAll || mode > SGM5860xScanModeAll9V) {
            elog_e(TAG, "Invalid scan mode");
            return 0;
        }
        CmdSgm5860ScanMode(mode);  // Set the scan mode for the SGM5860x device
        elog_i(TAG, "SGM5860x scan mode set to %d", mode);
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
    uint8_t gain    = sgm5860_channelcfg[channel_index].gain;
    uint8_t channel = sgm5860_channelcfg[channel_index].channel;

    process_noise     = 0.005;
    measurement_noise = 0.05;

    // 初始化自适应卡尔曼滤波器
    adaptive_kalman_init(&sgm5860xStatus.kalman_filters[channel_index], initial_value,
                         process_noise, measurement_noise, 1.0);

    // 设置参数边界
    adaptive_kalman_set_bounds(&sgm5860xStatus.kalman_filters[channel_index],
                               process_noise * 0.1,        // Q_min
                               process_noise * 10.0,       // Q_max
                               measurement_noise * 0.1,    // R_min
                               measurement_noise * 10.0);  // R_max

    adaptive_kalman_set_adaptation_rate(&sgm5860xStatus.kalman_filters[channel_index], 0.08);

    sgm5860xStatus.kalman_initialized[channel_index] = 1;
    sgm5860xStatus.sample_count[channel_index]       = 0;
    sgm5860xStatus.filtered_voltage[channel_index]   = 0.0;

    elog_d(TAG, "Kalman filter initialized for channel %d (gain=%d)", channel_index, gain);
}

#endif
