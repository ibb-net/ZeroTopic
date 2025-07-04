
// #include "McuadcCfg.h"
#define CONFIG_Mcuadc_EN 1
#if CONFIG_Mcuadc_EN
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

#define TAG          "Mcuadc"
#define McuadcLogLvl ELOG_LVL_INFO

#define McuadcPriority PriorityNormalEventGroup0
#ifndef McuadcChannelMax
#define McuadcChannelMax 1
#endif
#ifndef CONFIG_Mcuadc_CYCLE_TIMER_MS
#define CONFIG_Mcuadc_CYCLE_TIMER_MS 100
#endif
/* ===================================================================================== */
typedef struct {
    uint32_t id;
} TypdefMcuadcBSPCfg;

static void __McuadcCreateTaskHandle(void);
static void __McuadcRcvHandle(void *msg);
static void __McuadcCycHandle(void);
static void __McuadcInitHandle(void *msg);
static uint32_t __adc_channel_sample(uint8_t channel);
const TypdefMcuadcBSPCfg McuadcBspCfg[McuadcChannelMax] = {

};

typedef struct {
    char device_name[DEVICE_NAME_MAX];
    double value;  // ADC value
    uint32_t id;  // ID

} TypdefMcuadcStatus;
TypdefMcuadcStatus McuadcStatus[McuadcChannelMax] = {0};

/* ===================================================================================== */

static const vfb_event_t McuadcEventList[] = {
    /*
        McuadcStart,
        McuadcStop,
        McuadcSet,
        McuadcGet,
        McuadcPrint,
        McuadcRcv,
    */

};

static const VFBTaskStruct Mcuadc_task_cfg = {
    .name         = "VFBTaskMcuadc",  // Task name
    .pvParameters = NULL,
    // .uxPriority = 10,
    // // Task parameters
    .queue_num  = 8,                                              // Number of queues to subscribe
    .event_list = McuadcEventList,                                // Event list to subscribe
    .event_num  = sizeof(McuadcEventList) / sizeof(vfb_event_t),  // Number of events to subscribe
    .startup_wait_event_list = NULL,                              // Events to wait for at startup
    .startup_wait_event_num  = 0,  // Number of startup events to wait for
    .xTicksToWait            = pdMS_TO_TICKS(CONFIG_Mcuadc_CYCLE_TIMER_MS),  // Wait indefinitely
    .init_msg_cb             = __McuadcInitHandle,  // Callback for initialization messages
    .rcv_msg_cb              = __McuadcRcvHandle,   // Callback for received messages
    .rcv_timeout_cb          = __McuadcCycHandle,   // Callback for timeout
};

/* ===================================================================================== */

void McuadcDeviceInit(void) {
    elog_i(TAG, "McuadcDeviceInit");

    for (size_t i = 0; i < McuadcChannelMax; i++) {
        TypdefMcuadcStatus *McuadcStatusHandle = &McuadcStatus[i];

        McuadcStatusHandle->id = i;
        memset(McuadcStatusHandle->device_name, 0, sizeof(McuadcStatusHandle->device_name));
        snprintf(McuadcStatusHandle->device_name, sizeof(McuadcStatusHandle->device_name),
                 "Mcuadc%d", i);
    }
    rcu_periph_clock_enable(RCU_GPIOA);

    rcu_periph_clock_enable(RCU_ADC0);
    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_3);
}
SYSTEM_REGISTER_INIT(MCUInitStage, McuadcPriority, McuadcDeviceInit, McuadcDeviceInit);

static void __McuadcCreateTaskHandle(void) {
    for (size_t i = 0; i < McuadcChannelMax; i++) {
    }
    xTaskCreate(VFBTaskFrame, "VFBTaskMcuadc", configMINIMAL_STACK_SIZE*2, (void *)&Mcuadc_task_cfg,
                McuadcPriority, NULL);
}
SYSTEM_REGISTER_INIT(ServerInitStage, McuadcPriority, __McuadcCreateTaskHandle,
                     __McuadcCreateTaskHandle init);

static void __McuadcInitHandle(void *msg) {
    elog_i(TAG, "__McuadcInitHandle");
    elog_set_filter_tag_lvl(TAG, McuadcLogLvl);
    // vfb_send(McuadcStart, 0, NULL, 0);
    /* reset ADC */

    /* reset ADC */
    adc_deinit(ADC0);
    /* ADC clock config */
    adc_clock_config(ADC0, ADC_CLK_SYNC_HCLK_DIV6);
    adc_resolution_config(ADC0, ADC_RESOLUTION_12B);
    /* ADC scan mode enable */
    adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);
    /* ADC data alignment config */
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);

    /* ADC channel length config */
    adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, 1);
    /* ADC regular channel config */
    adc_regular_channel_config(ADC0, 0, ADC_CHANNEL_15, 240);

    /* ADC trigger config */
    adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, EXTERNAL_TRIGGER_DISABLE);

    /* enable ADC interface */
    adc_enable(ADC0);

    /* wait for ADC stability */
    vTaskDelay(pdMS_TO_TICKS(1));
    adc_calibration_mode_config(ADC0, ADC_CALIBRATION_OFFSET);
    /* ADC calibration number config */
    adc_calibration_number(ADC0, ADC_CALIBRATION_NUM1);
    /* ADC calibration and reset calibration */
    adc_calibration_enable(ADC0);

    /* ADC software trigger enable */
    adc_software_trigger_enable(ADC1, ADC_REGULAR_CHANNEL);
}
// 接收消息的回调函数
static void __McuadcRcvHandle(void *msg) {
    TaskHandle_t curTaskHandle = xTaskGetCurrentTaskHandle();
    // TypdefMcuadcStatus *McuadcStatusTmp = (TypdefMcuadcStatus *)&McuadcStatus[0];
    char *taskName        = pcTaskGetName(curTaskHandle);
    vfb_message_t tmp_msg = (vfb_message_t)msg;
    switch (tmp_msg->frame->head.event) {
        // case McuadcStart: {
        //     elog_i(TAG, "McuadcStartTask %d", tmp_msg->frame->head.data);
        // } break;
        default:
            elog_e(TAG, "TASK %s RCV: unknown event: %d", taskName, tmp_msg->frame->head.event);
            break;
    }
}

static uint32_t __adc_channel_sample(uint8_t channel) {
    /* ADC regular channel config */
    adc_regular_channel_config(ADC0, 0U, channel, 240);
    /* ADC software trigger enable */
    adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
    /* wait the end of conversion flag */
    //TODO 
    while (!adc_flag_get(ADC0, ADC_FLAG_EOC));
    /* clear the end of conversion flag */
    adc_flag_clear(ADC0, ADC_FLAG_EOC);
    //todo
    /* return regular channel sample value */
    return (adc_regular_data_read(ADC0));
}

static void __McuadcCycHandle(void) {
    TypdefMcuadcStatus *McuadcStatusHandle = &McuadcStatus[0];
    static uint32_t counter                = 0;
    static double voltage_sum               = 0.0;
    if (McuadcStatusHandle == NULL) {
        elog_e(TAG, "[ERROR]McuadcStatusHandle NULL");
        return;
    }
    if (counter < 10) {
        uint32_t sample_value = 1;
        sample_value          = __adc_channel_sample(ADC_CHANNEL_15);
        double Voltage         = (double)sample_value / 4095.0 * 3.3;
        voltage_sum += Voltage;
        counter++;
        return;  // 等待10次
    }
    if (counter == 10) {
        // 第10次采样，打印平均值
        double average_voltage = voltage_sum / 10.0 * 22.0;
        elog_d(TAG, "ADC Channel 15 Average Voltage: %.2f V", average_voltage);
        voltage_sum = 0.0;  // 重置电压和计数器
        counter     = 0;
        // ADCUpdate
        McuadcStatusHandle->value = average_voltage;  // 更新ADC状态值
        // 发送ADC更新事件
        vfb_send(ADCUpdate, 0, &average_voltage, sizeof(average_voltage));
    }
}

#endif

static void CmdMcuadcPrint(void) {
    printf("\r\n===================================================\r\n");
    printf("MCU ADC Device Status:\r\n");
    printf("Power Supply: %.2f V\r\n", McuadcStatus[0].value);
    printf("===================================================\r\n");
}
static void CmdMcuadcHelp(void) {
    printf("\r\n===================================================\r\n");
    printf("MCU ADC Command Help:\r\n");
    printf(" adc power   - Print the current ADC value\r\n");
    printf("===================================================\r\n");

}
static int CmdMcuadcHandle(int argc, char *argv[]) {
    if (argc < 2) {
        CmdMcuadcHelp();
        return 0;
    }
    // value
    if (memcmp(argv[1], "power", 5) == 0) {
        CmdMcuadcPrint();
        return 0;
    }

    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), adc,
                 CmdMcuadcHandle, Mcuadc command);