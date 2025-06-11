
#include "DACSgm3533Cfg.h"
#define CONFIG_DACSgm3533_EN 1
#if CONFIG_DACSgm3533_EN
#include <stdio.h>

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
typedef struct
{
    uint32_t id;
} TypdefDACSgm3533BSPCfg;

static void __DACSgm3533CreateTaskHandle(void);
static void __DACSgm3533RcvHandle(void *msg);
static void __DACSgm3533CycHandle(void);
static void __DACSgm3533InitHandle(void *msg);

const TypdefDACSgm3533BSPCfg DACSgm3533BspCfg[DACSgm3533ChannelMax] = {

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

static void CmdDACSgm3533Help(void) {
    elog_i(TAG, "Usage: sgm3533 <state>");
    elog_i(TAG, "  <state>: 0 for off, 1 for on");
    elog_i(TAG, "Example: sgm3533 1");
}
static int CmdDACSgm3533Handle(int argc, char *argv[]) {
    if (argc != 2) {
        CmdDACSgm3533Help();
        return 0;
    }

    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), sgm3533, CmdDACSgm3533Handle, DACSgm3533 command);