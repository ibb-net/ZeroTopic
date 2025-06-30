
#include "timer_decoder_cfg.h"

#if CONFIG_TIMER_DECODER_EN
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elog.h"
#include "gd32h7xx.h"
#include "os_server.h"
#include "shell.h"
#include "timer_decoder.h"
#define TAG                 "ENCODER"
#define TimerEcoderPriority PriorityOperationGroup0

typedef struct {
    uint32_t timer_rcu;
    uint32_t timer_base;
    uint32_t gpio_rcu;
    uint32_t gpio_port;
    uint32_t gpio_af;
    uint32_t gpio_pin_a;
    uint32_t gpio_pin_b;
} TypdefDecoderBSPCfg;
const float max_pyh[CONFIG_TIMER_DECODER_CHANNEL_MAX] = {
    MAX_PHY_VALUE_CH0,
    MAX_PHY_VALUE_CH1,
};
const float min_pyh[CONFIG_TIMER_DECODER_CHANNEL_MAX] = {
    MIN_PHY_VALUE_CH0,
    MIN_PHY_VALUE_CH1,
};
const float def_phy[CONFIG_TIMER_DECODER_CHANNEL_MAX] = {
    DEF_PHY_VALUE_CH0,
    DEF_PHY_VALUE_CH1,
};
const float step_phy[CONFIG_TIMER_DECODER_CHANNEL_MAX] = {
    STEP_PHY_VALUE_CH0,
    STEP_PHY_VALUE_CH1,
};
#define CONFIG_ENCODER_CH0_GAIN (2.2)
#define CONFIG_ENCODER_CH1_GAIN (1.0)
const float channel_gain[CONFIG_TIMER_DECODER_CHANNEL_MAX] = {
    CONFIG_ENCODER_CH0_GAIN,
    CONFIG_ENCODER_CH1_GAIN,
};
#define ENCODER_MAX_DIFF (100)

const TypdefDecoderBSPCfg decoder_bsp_cfg[CONFIG_TIMER_DECODER_CHANNEL_MAX] = {
    {
        .timer_rcu  = ENCODER_CH0_TIMER_RCU,
        .timer_base = ENCODER_CH0_TIMER,
        .gpio_rcu   = ENCODER_CH0_GPIO_RCU,
        .gpio_port  = ENCODER_CH0_GPIO_PORT,
        .gpio_af    = ENCODER_CH0_GPIO_AF,
        .gpio_pin_a = ENCODER_CH0_GPIO_PIN_A,
        .gpio_pin_b = ENCODER_CH0_GPIO_PIN_B,
    },
#if (CONFIG_TIMER_DECODER_CHANNEL_MAX > 1)
    {
        .timer_rcu  = ENCODER_CH1_TIMER_RCU,
        .timer_base = ENCODER_CH1_TIMER,
        .gpio_rcu   = ENCODER_CH1_GPIO_RCU,
        .gpio_port  = ENCODER_CH1_GPIO_PORT,
        .gpio_af    = ENCODER_CH1_GPIO_AF,
        .gpio_pin_a = ENCODER_CH1_GPIO_PIN_A,
        .gpio_pin_b = ENCODER_CH1_GPIO_PIN_B,
    },

#endif
};
#if CONFIG_TIMER_DECODER_CHANNEL_MAX == 1
const float decoder_step_map[1][16] = {
    {CONFIG_TIMER_DECODER_CH0_STEP0, CONFIG_TIMER_DECODER_CH0_STEP1, CONFIG_TIMER_DECODER_CH0_STEP2,
     CONFIG_TIMER_DECODER_CH0_STEP3, CONFIG_TIMER_DECODER_CH0_STEP4, CONFIG_TIMER_DECODER_CH0_STEP5,
     CONFIG_TIMER_DECODER_CH0_STEP6, CONFIG_TIMER_DECODER_CH0_STEP7, CONFIG_TIMER_DECODER_CH0_STEP8,
     CONFIG_TIMER_DECODER_CH0_STEP9, CONFIG_TIMER_DECODER_CH0_STEP10,
     CONFIG_TIMER_DECODER_CH0_STEP11, CONFIG_TIMER_DECODER_CH0_STEP12,
     CONFIG_TIMER_DECODER_CH0_STEP13, CONFIG_TIMER_DECODER_CH0_STEP14,
     CONFIG_TIMER_DECODER_CH0_STEP15}};
#elif CONFIG_TIMER_DECODER_CHANNEL_MAX == 2
const float decoder_step_map[2][16] = {
    {CONFIG_TIMER_DECODER_CH0_STEP0, CONFIG_TIMER_DECODER_CH0_STEP1, CONFIG_TIMER_DECODER_CH0_STEP2,
     CONFIG_TIMER_DECODER_CH0_STEP3, CONFIG_TIMER_DECODER_CH0_STEP4, CONFIG_TIMER_DECODER_CH0_STEP5,
     CONFIG_TIMER_DECODER_CH0_STEP6, CONFIG_TIMER_DECODER_CH0_STEP7, CONFIG_TIMER_DECODER_CH0_STEP8,
     CONFIG_TIMER_DECODER_CH0_STEP9, CONFIG_TIMER_DECODER_CH0_STEP10,
     CONFIG_TIMER_DECODER_CH0_STEP11, CONFIG_TIMER_DECODER_CH0_STEP12,
     CONFIG_TIMER_DECODER_CH0_STEP13, CONFIG_TIMER_DECODER_CH0_STEP14,
     CONFIG_TIMER_DECODER_CH0_STEP15},
    {CONFIG_TIMER_DECODER_CH1_STEP0, CONFIG_TIMER_DECODER_CH1_STEP1, CONFIG_TIMER_DECODER_CH1_STEP2,
     CONFIG_TIMER_DECODER_CH1_STEP3, CONFIG_TIMER_DECODER_CH1_STEP4, CONFIG_TIMER_DECODER_CH1_STEP5,
     CONFIG_TIMER_DECODER_CH1_STEP6, CONFIG_TIMER_DECODER_CH1_STEP7, CONFIG_TIMER_DECODER_CH1_STEP8,
     CONFIG_TIMER_DECODER_CH1_STEP9, CONFIG_TIMER_DECODER_CH1_STEP10,
     CONFIG_TIMER_DECODER_CH1_STEP11, CONFIG_TIMER_DECODER_CH1_STEP12,
     CONFIG_TIMER_DECODER_CH1_STEP13, CONFIG_TIMER_DECODER_CH1_STEP14,
     CONFIG_TIMER_DECODER_CH1_STEP15}};
#else
#error "Unsupported CONFIG_TIMER_DECODER_CHANNEL_MAX value"
#endif
typedef struct {
    uint8_t channel;  // 通道号 0:CH0 1:CH1
    uint16_t event;
    uint32_t timer_handle;
    uint8_t channel_ctr;  // 0 stop 1:start
    uint8_t mode;         // 0:连续模式 1:步进模式
    float phy_value;
    float phy_value_last;  // 上次物理值
    float phy_value_max;   // 最大物理值
    float phy_value_min;   // 最小物理值
    float phy_value_step;
    uint64_t diff;  // 上次计数值和当前计数值的差值

    uint8_t is_turning;             // 是否正在旋转
    uint64_t last_encoder_counter;  // 上次计数值
    uint64_t start_cnt;             // 旋转开始计数值
    uint8_t direction;              // 旋转方向 0:正向 1:反向
    uint32_t tmp_cnt;               // 当一次读取不足 4个计数值时，临时存储的计数值
    uint32_t active_duration;       // 通道激活持续时间
    uint32_t active_timeout;        // 通道激活超时时间
    uint32_t pluse_gain;            // 旋转速度产生的增益
    uint32_t duration_gain;         // 旋转时间产生的增益
    uint32_t pluse_cnt;             // 旋转单次触发的个数
    /* 步进模式 */
    int step_index;  // 步进模式下的步进索引
} TypdefEncoderStruct;
TypdefEncoderStruct encoder_struct[CONFIG_TIMER_DECODER_CHANNEL_MAX] = {0};
static void __decoder_handle(void);
static void __encoder_timer_clear(TypdefEncoderStruct *encoder);

void device_timer_encoder_init(void) {
    printf("device_timer_encoder_init\r\n");
    timer_parameter_struct timer_initpara;

    for (size_t i = 0; i < CONFIG_TIMER_DECODER_CHANNEL_MAX; i++) {
        rcu_periph_clock_enable(decoder_bsp_cfg[i].timer_rcu);
        rcu_periph_clock_enable(decoder_bsp_cfg[i].gpio_rcu);
        gpio_af_set(decoder_bsp_cfg[i].gpio_port, decoder_bsp_cfg[i].gpio_af,
                    decoder_bsp_cfg[i].gpio_pin_a);
        gpio_af_set(decoder_bsp_cfg[i].gpio_port, decoder_bsp_cfg[i].gpio_af,
                    decoder_bsp_cfg[i].gpio_pin_b);
        gpio_mode_set(decoder_bsp_cfg[i].gpio_port, GPIO_MODE_AF, GPIO_PUPD_NONE,
                      decoder_bsp_cfg[i].gpio_pin_a);
        gpio_mode_set(decoder_bsp_cfg[i].gpio_port, GPIO_MODE_AF, GPIO_PUPD_NONE,
                      decoder_bsp_cfg[i].gpio_pin_b);
        gpio_output_options_set(decoder_bsp_cfg[i].gpio_port, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ,
                                decoder_bsp_cfg[i].gpio_pin_a);
        gpio_output_options_set(decoder_bsp_cfg[i].gpio_port, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ,
                                decoder_bsp_cfg[i].gpio_pin_b);

        timer_deinit(decoder_bsp_cfg[i].timer_base);  // 复位计时器
        // 初始化计时器结构体
        timer_initpara.prescaler         = 0;
        timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
        timer_initpara.counterdirection  = TIMER_COUNTER_UP;
        timer_initpara.period            = 65535;
        timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
        timer_initpara.repetitioncounter = 0;
        timer_init(decoder_bsp_cfg[i].timer_base, &timer_initpara);  // 初始化计时器
        timer_quadrature_decoder_mode_config(
            decoder_bsp_cfg[i].timer_base, TIMER_QUAD_DECODER_MODE2, TIMER_IC_POLARITY_RISING,
            TIMER_IC_POLARITY_RISING);  // 配置计时器为四分频解码模式
        timer_auto_reload_shadow_enable(decoder_bsp_cfg[i].timer_base);
        timer_counter_value_config(decoder_bsp_cfg[i].timer_base, DEFAULT_TIMER_VALUE);
        timer_enable(decoder_bsp_cfg[i].timer_base);  // 使能计时器

        encoder_struct[i].timer_handle = decoder_bsp_cfg[i].timer_base;  // 设置计时器句柄
        encoder_struct[i].channel      = i;                              // 设置通道号
        encoder_struct[i].channel_ctr  = 0;                              // 初始化通道状态为停止
    }
}
SYSTEM_REGISTER_INIT(BoardInitStage, TimerEcoderPriority, device_timer_encoder_init,
                     device_timer_encoder_init);

// 函数声明
static void ENCODER_TIMER_RCV_HANDLE(void *msg);
static void ENCODER_TIMER_CYC_HANDLE(void);
static void ENCODER_TIMER_INIT_HANDLE(void *msg);

// 定义事件列表
static const vfb_event_t ENCODER_TIMER_event_list[] = {
    ENCODER_TIMER_START,
    ENCODER_TIMER_STOP,
    ENCODER_TIMER_SET_MODE,
    ENCODER_TIMER_SET_PHY,

};

// 定义任务配置结构体
static const VFBTaskStruct ENCODER_TIMER_task_cfg = {
    .name         = "VFBTaskENCODER_TIMER",  // Task name
    .pvParameters = NULL,
    .uxPriority   = 10,                        // Task parameters
    .queue_num    = 8,                         // Number of queues to subscribe
    .event_list   = ENCODER_TIMER_event_list,  // Event list to subscribe
    .event_num =
        sizeof(ENCODER_TIMER_event_list) / sizeof(vfb_event_t),  // Number of events to subscribe
    .startup_wait_event_list = NULL,                             // Events to wait for at startup
    .startup_wait_event_num  = 0,  // Number of startup events to wait for
    .xTicksToWait            = pdMS_TO_TICKS(CONFIG_ENCODER_CYCLE_TIMER_MS),  // Wait indefinitely
    .init_msg_cb             = ENCODER_TIMER_INIT_HANDLE,  // Callback for initialization messages
    .rcv_msg_cb              = ENCODER_TIMER_RCV_HANDLE,   // Callback for received messages
    .rcv_timeout_cb          = ENCODER_TIMER_CYC_HANDLE,   // Callback for timeout
};

// 任务初始化函数
void ENCODER_TIMER_CREATE_HANDLE(void) {
    for (size_t i = 0; i < CONFIG_TIMER_DECODER_CHANNEL_MAX; i++) {
        encoder_struct[i].phy_value      = def_phy[i];               // 设置默认物理值
        encoder_struct[i].phy_value_max  = max_pyh[i];               // 设置最大物理值
        encoder_struct[i].phy_value_min  = min_pyh[i];               // 设置最小物理值
        encoder_struct[i].phy_value_step = step_phy[i];              // 设置物理值步进
        encoder_struct[i].mode           = DECODER_MODE_CONTINUOUS;  // 设置默认模式为连续模式
        __encoder_timer_clear(&encoder_struct[i]);                   // 初始化每个通道的计时器
    }

    xTaskCreate(VFBTaskFrame, "Encoder", configMINIMAL_STACK_SIZE * 2,
                (void *)&ENCODER_TIMER_task_cfg, TimerEcoderPriority, NULL);
}
SYSTEM_REGISTER_INIT(ServerInitStage, TimerEcoderPriority, ENCODER_TIMER_CREATE_HANDLE,
                     timer_encoder init);

static void __encoder_timer_clear(TypdefEncoderStruct *encoder) {
    // encoder->phy_value = DEFAULT_TIMER_VALUE;
    // // 重置物理值
    encoder->diff           = 0;           // 重置差值
    encoder->phy_value_last = 0xFFFFFFFF;  // 重置上次物理值
    encoder->is_turning     = 0;           // 重置旋转状态
    encoder->last_encoder_counter =
        timer_counter_read(encoder->timer_handle);  // DEFAULT_TIMER_VALUE; // 重置上次计数值
    encoder->start_cnt       = 0;                   // 重置旋转开始计数值
    encoder->direction       = 0;                   // 重置旋转方向
    encoder->tmp_cnt         = 0;                   // 重置临时计数值
    encoder->active_duration = 0;                   // 重置激活持续时间
    encoder->active_timeout  = 0;                   // 重置激活超时时间
    encoder->pluse_gain      = CONFIG_ENCODER_PULSE_GAIN_DEFAULT;     // 重置旋转速度增益
    encoder->duration_gain   = CONFIG_ENCODER_DURATION_GAIN_DEFAULT;  // 重置旋转时间增益
    encoder->pluse_cnt       = CONFIG_ENCODER_ONE_PULSE_CNT;          // 重置旋转单次触发的个数
    // encoder->step_index           = 0;                                          // elog_i(TAG,
    // "Encoder channel %d Clear", encoder->channel);
}

static void ENCODER_TIMER_INIT_HANDLE(void *msg) {
    elog_i(TAG, "ENCODER_TIMER_INIT_HANDLE\r\n");
    // 发布事件，表示计时器已准备就绪
    // vfb_publish(ENCODER_TIMER_READY);
    vfb_send(ENCODER_TIMER_START, DECODER_CHANNEL1, NULL, 0);  // 启动计时器
    vfb_send(ENCODER_TIMER_START, DECODER_CHANNEL0, NULL, 0);  // 启动计时器
    VFBMsgDecoderStruct msg_decoder = {
        .channel   = DECODER_CHANNEL0,
        .phy_value = 0,
        .mode      = DECODER_MODE_CONTINUOUS,
    };

    vfb_send(ENCODER_TIMER_SET_MODE, 0, &msg_decoder,
             sizeof(VFBMsgDecoderStruct));  // 设置计时器模式为步进模式
    VFBMsgDecoderStruct msg_decoder1 = {
        .channel   = DECODER_CHANNEL1,
        .phy_value = 0,
        .mode      = DECODER_MODE_CONTINUOUS,
    };
    vfb_send(ENCODER_TIMER_SET_MODE, 0, &msg_decoder1,
             sizeof(VFBMsgDecoderStruct));  // 设置计时器模式为步进模式
}
// 接收消息的回调函数
static void ENCODER_TIMER_RCV_HANDLE(void *msg) {
    TaskHandle_t curTaskHandle = xTaskGetCurrentTaskHandle();
    char *taskName             = pcTaskGetName(curTaskHandle);
    vfb_message_t tmp_msg      = (vfb_message_t)msg;
    switch (tmp_msg->frame->head.event) {
        case ENCODER_TIMER_START: {
            DecoderChannelEnum channel = (DecoderChannelEnum)tmp_msg->frame->head.data;
            if (channel >= CONFIG_TIMER_DECODER_CHANNEL_MAX) {
                elog_e(TAG, "ENCODER_TIMER_START: Invalid channel %d", channel);
                return;  // 无效通道
            }
            elog_i(TAG, "ENCODER_TIMER_START Channel %d", channel);
            encoder_struct[channel].channel_ctr = 1;  // 启用通道
            encoder_struct[channel].last_encoder_counter =
                timer_counter_read(encoder_struct[channel].timer_handle);  // 读取当前计时器值
        } break;
        case ENCODER_TIMER_STOP: {
            DecoderChannelEnum channel = (DecoderChannelEnum)tmp_msg->frame->head.data;
            if (channel >= CONFIG_TIMER_DECODER_CHANNEL_MAX) {
                elog_e(TAG, "ENCODER_TIMER_STOP: Invalid channel %d", channel);
                return;  // 无效通道
            }
            if (encoder_struct[channel].channel_ctr == 0) {
                elog_w(TAG, "ENCODER_TIMER_STOP: Channel %d is already stopped", channel);
                return;  // 通道已停止
            }
            elog_i(TAG, "ENCODER_TIMER_STOP %d", channel);
            encoder_struct[channel].channel_ctr = 0;  // 停止通道
            encoder_struct[channel].last_encoder_counter =
                timer_counter_read(encoder_struct[channel].timer_handle);  // 读取当前计时器值
        } break;
        case ENCODER_TIMER_SET_MODE: {
            VFBMsgDecoderStruct *tmp_decoder = (VFBMsgDecoderStruct *)(MSG_GET_PAYLOAD(msg));
            if (tmp_decoder == NULL) {
                elog_e(TAG, "ENCODER_TIMER_SET_MODE: msg_decoder is NULL");
                return;
            }
            DecoderChannelEnum channel = (DecoderChannelEnum)tmp_decoder->channel;
            if (channel >= CONFIG_TIMER_DECODER_CHANNEL_MAX) {
                elog_e(TAG, "ENCODER_TIMER_SET_MODE: Invalid channel %d", channel);
                return;  // 无效通道
            }
            if (tmp_decoder->mode != DECODER_MODE_CONTINUOUS &&
                tmp_decoder->mode != DECODER_MODE_STEP) {
                elog_e(TAG, "ENCODER_TIMER_SET_MODE: Invalid mode %d", tmp_decoder->mode);
                return;  // 无效模式
            }
            elog_i(TAG, "ENCODER_TIMER_SET_MODE %d, mode: %d", channel, tmp_decoder->mode);
            encoder_struct[channel].mode = tmp_decoder->mode;  // 设置通道模式
            __encoder_timer_clear(&encoder_struct[channel]);   // 清除计时器状态
            if (encoder_struct[channel].mode == DECODER_MODE_STEP) {
                encoder_struct[channel].phy_value = encoder_struct[channel].step_index =
                    0;  // 步进索引重置为0
                elog_i(TAG, "Encoder channel %d set to STEP mode", channel);
            } else {
                encoder_struct[channel].phy_value = tmp_decoder->phy_value;  // 设置物理值
                elog_i(TAG, "Encoder channel %d set to CONTINUOUS mode", channel);
            }
        } break;
        case ENCODER_TIMER_SET_PHY: {
            VFBMsgDecoderStruct *tmp_decoder = (VFBMsgDecoderStruct *)(MSG_GET_PAYLOAD(msg));
            if (tmp_decoder == NULL) {
                elog_e(TAG, "ENCODER_TIMER_SET_PHY: msg_decoder is NULL");
                return;
            }
            DecoderChannelEnum channel = (DecoderChannelEnum)tmp_decoder->channel;
            if (channel >= CONFIG_TIMER_DECODER_CHANNEL_MAX) {
                elog_d(TAG, "ENCODER_TIMER_SET_PHY: Invalid channel %d", channel);
                return;  // 无效通道
            }
            if (tmp_decoder->phy_value < encoder_struct[channel].phy_value_min ||
                tmp_decoder->phy_value > encoder_struct[channel].phy_value_max) {
                elog_d(TAG, "ENCODER_TIMER_SET_PHY: Invalid phy_value %f for channel %d",
                       tmp_decoder->phy_value, channel);
                return;  // 无效物理值
            }
            elog_d(TAG, "ENCODER_TIMER_SET_PHY Channel %d, phy_value: %f", channel,
                   tmp_decoder->phy_value);
            encoder_struct[channel].phy_value = tmp_decoder->phy_value;
        } break;
        default:
            printf("TASK %s RCV: unknown event: %d\r\n", taskName, tmp_msg->frame->head.event);
            break;
    }
}

// 超时处理的回调函数
static void ENCODER_TIMER_CYC_HANDLE(void) { __decoder_handle(); }

static void __decoder_handle(void) {
    for (size_t i = 0; i < CONFIG_TIMER_DECODER_CHANNEL_MAX; i++) {
        if (!encoder_struct[i].channel_ctr) {
            continue;  // 如果通道未启用，跳过
        }
        int64_t diff_abs             = 0;
        int64_t diff                 = 0;
        uint8_t direction            = 0;  // 旋转方向
        TickType_t currentTick       = xTaskGetTickCount();
        float tmp_gain               = 0;
        TypdefEncoderStruct *encoder = &encoder_struct[i];
        // 读取当前计时器值
        uint64_t current_encoder_counter = timer_counter_read(encoder_struct[i].timer_handle);
        diff = current_encoder_counter - encoder_struct[i].last_encoder_counter;
        // if (diff == 0) {
        //     return;
        // }

        diff                   = 0 - diff;
        diff_abs               = (diff < 0) ? -diff : diff;  // 计算绝对差值
        direction              = (diff > 0) ? 0 : 1;         // 计算旋转方向，正向为0，反向为1
        uint32_t tmp_pluse_cnt = 0;                          // 当前周期有效变化
        if (diff_abs > ENCODER_MAX_DIFF) {
            // do nothing
            /* 该状态下可能是定时器越界 */
            __encoder_timer_clear(&encoder_struct[i]);  // 清除计时器
        } else if (diff_abs) {
            elog_d(TAG, "diff %ld", diff);
            // elog_i(TAG, "Encoder channel %d diff: %ld", i, diff);
            // elog_i(TAG, "current_encoder_counter: %lu", current_encoder_counter);
            // elog_i(TAG, " last_encoder_counter: %lu", encoder_struct[i].last_encoder_counter);
            encoder_struct[i].last_encoder_counter = current_encoder_counter;  // 更新上次计数值
            // elog_i(TAG, "CH0 diff: %ld", diff);
            elog_d(TAG, "CH0 Direction: %s", (diff > 0) ? "Forward" : "Backward");
            /* 更新基础状态 */
            encoder->is_turning = 1;  // 正在旋转
            encoder->active_duration += CONFIG_ENCODER_CYCLE_TIMER_MS;
            encoder->active_timeout = currentTick + CONFIG_ENCODER_ACTIVE_TIMEOUT_MS;

            /* 判断是否有完成一次旋转,和旋转的个数 */
            encoder->tmp_cnt += diff_abs;  // 累加当前周期的计数值
            tmp_pluse_cnt    = encoder->tmp_cnt / CONFIG_ENCODER_ONE_PULSE_CNT;
            encoder->tmp_cnt = encoder->tmp_cnt % CONFIG_ENCODER_ONE_PULSE_CNT;  // 更新剩余计数值

            if (encoder->direction != direction) {
                encoder->direction       = direction;
                encoder->pluse_gain      = 1;
                encoder->duration_gain   = 1;
                encoder->active_duration = 0;  // 重置激活持续时间
            } else {
                /* 计算转速增益 */
                if (tmp_pluse_cnt < 3) {
                    encoder->pluse_gain      = 1;
                    encoder->active_duration = 0;  // 如果转速过低，重置激活持续时间
                } else if (tmp_pluse_cnt < 10) {
                    encoder->pluse_gain = 20 * channel_gain[i];
                    ;
                } else {
                    encoder->pluse_gain = 40;
                }
                /* 计算时间增益 */
                // if (encoder->active_duration < CONFIG_ENCODER_CYCLE_TIMER_MS * 30) {
                //     encoder->duration_gain = 1;
                // } else {
                //     encoder->duration_gain = 10;
                // }
                encoder->duration_gain = 1;
            }
            if (encoder->mode == DECODER_MODE_STEP) {
                if (tmp_pluse_cnt) {
                    if (diff > 0) {
                        if (decoder_step_map[i][encoder->step_index + 1] == (-1)) {
                            // do nothing
                        } else {
                            encoder->step_index++;
                        }
                    } else {
                        if (encoder->step_index <= 0) {
                            encoder->step_index = 0;  // 如果步进索引小于0，保持不变
                            // donothing
                        } else {
                            encoder->step_index--;
                        }
                    }
                    encoder->phy_value = decoder_step_map[i][encoder->step_index];
                    elog_d(TAG, "Encoder channel %d step index: %d, value: %ld", i,
                           encoder->step_index, encoder->phy_value);
                } else {
                    // do nothing
                }
            } else {
                tmp_gain = encoder->pluse_gain * encoder->duration_gain;
                elog_d(TAG, "phy_value %f tmp_gain %.4f pluse_gain %u duration_gain %u",
                       encoder->phy_value, tmp_gain, encoder->pluse_gain, encoder->duration_gain);
                if (diff > 0) {
                    encoder->phy_value +=
                        tmp_pluse_cnt * tmp_gain * encoder->phy_value_step;  // 更新物理值，正向旋转

                    elog_d(
                        TAG,
                        "++++ cnt %d phy_value:%.3f tmp_gain : %.4f pluse_gain %u duration_gain %u",
                        tmp_pluse_cnt, encoder->phy_value, tmp_gain, encoder->pluse_gain,
                        encoder->duration_gain);
                } else {
                    if (encoder->phy_value <= tmp_pluse_cnt * tmp_gain * encoder->phy_value_step) {
                        encoder->phy_value = encoder->phy_value_min;
                    } else
                        encoder->phy_value -= tmp_pluse_cnt * tmp_gain *
                                              encoder->phy_value_step;  // 更新物理值，反向旋转
                    elog_d(
                        TAG,
                        "--- cnt %d phy_value:%.3f tmp_gain : %.4f pluse_gain %u duration_gain %u",
                        tmp_pluse_cnt, encoder->phy_value, tmp_gain, encoder->pluse_gain,
                        encoder->duration_gain);
                }
            }

            if (encoder->phy_value > encoder->phy_value_max) {
                encoder->phy_value = encoder->phy_value_max;  // 限制物理值最大值
            } else if (encoder->phy_value < encoder->phy_value_min) {
                encoder->phy_value = encoder->phy_value_min;  // 限制物理值最小值
            }
            if (encoder->phy_value != encoder->phy_value_last) {
                encoder->phy_value_last         = encoder->phy_value;  // 更新上次物理值
                VFBMsgDecoderStruct msg_decoder = {
                    .channel   = i,                   // 通道号
                    .phy_value = encoder->phy_value,  // 物理值
                };
                vfb_send(ENCODER_TIMER_GET_PHY, i, (void *)&msg_decoder,
                         sizeof(VFBMsgDecoderStruct));  // 发送物理值消息
                elog_d(TAG, "Encoder[%d] phy:%.3f dur:%dms gain:%d", i, encoder->phy_value,
                       encoder->active_duration, tmp_gain);
            }

        } else {
            if (encoder->is_turning) {
                if (encoder->active_timeout < currentTick) {
                    __encoder_timer_clear(
                        &encoder_struct[i]);  // 如果没有旋转，清除计时器
                                              // elog_i(TAG, "Encoder stopped after %d ms of
                                              // inactivity", encoder->active_duration);
                } else {
                    // do nothing
                }
            }
        }
    }
}
static int mode_change(int argc, char *argv[]) {
    if (argc < 3) {
        elog_e(TAG, "Invalid arguments. Usage: decoder mode <channel> <mode>");
        return -1;
    }
    int channel = atoi(argv[2]);
    int mode    = atoi(argv[3]);
    if (channel >= CONFIG_TIMER_DECODER_CHANNEL_MAX || channel < 0) {
        elog_e(TAG, "Invalid channel: %d", channel);
        return -1;
    }
    if (mode != 0 && mode != 1) {
        elog_e(TAG, "Invalid mode: %d. Use 0 for continuous mode, 1 for step mode.", mode);
        return -1;
    }
    VFBMsgDecoderStruct msg_decoder = {
        .channel = channel,
        .mode    = (mode == 0) ? DECODER_MODE_CONTINUOUS : DECODER_MODE_STEP,
    };
    vfb_send(ENCODER_TIMER_SET_MODE, 0, &msg_decoder, sizeof(VFBMsgDecoderStruct));
    elog_i(TAG, "Set channel %d to mode %d", channel, mode);
    return 0;
}
static void CmddecoderHelp(void) {
    elog_i(TAG, "\r\nUsage: decoder <state>");
    elog_i(TAG, "       decoder mode <channel 0/1> <mode 0/1> 0: CONTINUOUS 1Step mode");
}
static int CmddecoderHandle(int argc, char *argv[]) {
    if (argc < 2) {
        CmddecoderHelp();
        return 0;
    }
    if (strcmp(argv[1], "help") == 0) {
        CmddecoderHelp();
        return 0;
    } else if (strcmp(argv[1], "mode") == 0) {
        mode_change(argc, argv);
        return 0;
    }

    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), decoder,
                 CmddecoderHandle, decoder command);
#endif  // CONFIG_TIMER_DECODER_EN